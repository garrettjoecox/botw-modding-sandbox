// screen.cpp - print(), draw_text() and draw_box().
//
// Behind-the-scenes code. botw.gui only lets a mod draw from inside its own
// per-frame callback (Draw, below), but your code runs in every_frame(), which is
// a different callback. So drawing is recorded: draw_text() and draw_box()
// write into a list during every_frame(), and Draw() paints the latest list.
// print() works the same way, with lines that fade out after a few seconds.

#include "engine/engine.h"

namespace Gui {
WXL_USE_botw_gui(PushAlpha);
WXL_USE_botw_gui(PopAlpha);
}

namespace {

// --- styles ------------------------------------------------------------------
//
// A botw.gui style is a font size + colour + shadow, made once and reused.
// The GUI has 32 for every mod together, so keep a few and share them.

constexpr float kTextScale = 0.75f;
constexpr float kMessageScale = 0.7f;

struct StyleEntry {
    Color color;
    float scale;
    uint32_t id;
};

StyleEntry g_Styles[8];
uint32_t g_StyleCount;

uint32_t StyleFor(Color color, float scale) {
    for (uint32_t i = 0; i < g_StyleCount; ++i) {
        if (g_Styles[i].color == color && g_Styles[i].scale == scale) return g_Styles[i].id;
    }
    if (g_StyleCount >= sizeof(g_Styles) / sizeof(g_Styles[0])) return g_Styles[0].id;
    const uint32_t id = Gui::StyleCreate();
    if (!id) return 0;
    Gui::StyleScale(id, scale);
    Gui::StyleColor(id, color, color);
    Gui::StyleShadow(id, 1, 0x00000080, 2.0f, 2.0f);
    g_Styles[g_StyleCount++] = {color, scale, id};
    return id;
}

// --- shapes from every_frame() -----------------------------------------------

enum ShapeKind : uint8_t { kText, kBox };

struct Shape {
    ShapeKind kind;
    float x, y, w, h;
    Color color;
    char text[96];
};

constexpr uint32_t kMaxShapes = 48;

// Two lists: every_frame() fills one while Draw() paints the other.
Shape g_Shapes[2][kMaxShapes];
uint32_t g_ShapeCount[2];
uint32_t volatile g_Filling;          // the list every_frame() is writing
uint32_t volatile g_Finished = 2;     // the last complete list (2 = none yet)
uint32_t volatile g_FinishedSerial;   // goes up each time a list is finished
uint32_t g_SerialSeen;
uint32_t g_FramesWithoutNewList;
bool g_WarnedFull;

Shape* NewShape() {
    const uint32_t list = g_Filling;
    if (g_ShapeCount[list] >= kMaxShapes) {
        if (!g_WarnedFull) {
            g_WarnedFull = true;
            print("Too many draw calls in one frame - only the first %u are shown", kMaxShapes);
        }
        return nullptr;
    }
    return &g_Shapes[list][g_ShapeCount[list]++];
}

void AddText(float x, float y, Color color, const char* format, va_list args) {
    Shape* shape = NewShape();
    if (!shape) return;
    shape->kind = kText;
    shape->x = x;
    shape->y = y;
    shape->color = color;
    Engine::Format(shape->text, sizeof(shape->text), format, args);
}

void DrawShapes() {
    // Every_frame() stops while the game is paused or loading. A list that
    // hasn't been replaced for a few frames is stale, so stop showing it.
    if (g_FinishedSerial != g_SerialSeen) {
        g_SerialSeen = g_FinishedSerial;
        g_FramesWithoutNewList = 0;
    } else if (g_FramesWithoutNewList < 1000) {
        ++g_FramesWithoutNewList;
    }
    const uint32_t list = g_Finished;
    if (list > 1 || g_FramesWithoutNewList > 3) return;

    for (uint32_t i = 0; i < g_ShapeCount[list]; ++i) {
        const Shape& s = g_Shapes[list][i];
        if (s.kind == kBox) {
            Gui::RoundedBox(s.x, s.y, s.w, s.h, s.color, 12.0f);
        } else {
            Gui::Text(s.x, s.y, s.text, StyleFor(s.color, kTextScale));
        }
    }
}

// --- print() messages ----------------------------------------------------------

constexpr uint32_t kMaxMessages = 5;
constexpr float kMessageSeconds = 6.0f;

struct Message {
    char text[100];
    uint32_t repeats;    // 1 = shown once; more when the same text is printed again
    float shownAt;       // GUI clock when first drawn; < 0 = not drawn yet
};

Message g_Messages[kMaxMessages];    // oldest first
uint32_t g_MessageCount;

void DrawMessages(float now) {
    // Drop the ones that have been up long enough.
    while (g_MessageCount > 0 && g_Messages[0].shownAt >= 0.0f &&
           now - g_Messages[0].shownAt > kMessageSeconds) {
        for (uint32_t i = 1; i < g_MessageCount; ++i) g_Messages[i - 1] = g_Messages[i];
        --g_MessageCount;
    }

    const uint32_t style = StyleFor(CREAM, kMessageScale);
    float y = 24.0f;
    for (uint32_t i = 0; i < g_MessageCount; ++i) {
        Message& m = g_Messages[i];
        if (m.shownAt < 0.0f) m.shownAt = now;

        char line[120];
        if (m.repeats > 1) {
            Engine::FormatTo(line, sizeof(line), "%s  (x%u)", m.text, m.repeats);
        } else {
            Engine::CopyText(line, sizeof(line), m.text);
        }

        float w = 0.0f, h = 0.0f;
        Gui::MeasureText(line, style, &w, &h, 0.0f);
        if (h <= 0.0f) h = 24.0f;
        const float x = 640.0f - w / 2.0f;   // centred

        // Fade out over the last second.
        const float left = kMessageSeconds - (now - m.shownAt);
        Gui::PushAlpha(left < 1.0f ? (left > 0.0f ? left : 0.0f) : 1.0f);
        Gui::RoundedBox(x - 14.0f, y - 4.0f, w + 28.0f, h + 8.0f, 0x00000090, 10.0f);
        Gui::Text(x, y, line, style);
        Gui::PopAlpha();

        y += h + 14.0f;
    }
}

// The log gets each message once; a run of the same text is summed up when it
// ends, so printing every frame doesn't bury everything else.
char g_LastLogged[256];
uint32_t g_LastLoggedRepeats;

void Log(const char* text) {
    if (g_LastLoggedRepeats > 0 && same_text(text, g_LastLogged)) {
        ++g_LastLoggedRepeats;
        return;
    }
    if (g_LastLoggedRepeats > 1) {
        WIIXL_LOG("my_mod: (that message repeated %u more times)", g_LastLoggedRepeats - 1);
    }
    Engine::CopyText(g_LastLogged, sizeof(g_LastLogged), text);
    g_LastLoggedRepeats = 1;
    WIIXL_LOG("my_mod: %s", text);
}

} // namespace

namespace Engine {

void Show(const char* full) {
    Log(full);
    char text[sizeof(Message::text)];
    CopyText(text, sizeof(text), full);
    if (g_MessageCount > 0 && same_text(g_Messages[g_MessageCount - 1].text, text)) {
        Message& last = g_Messages[g_MessageCount - 1];
        ++last.repeats;
        if (last.shownAt >= 0.0f) last.shownAt = -1.0f;   // restart its timer
        return;
    }
    if (g_MessageCount == kMaxMessages) {
        for (uint32_t i = 1; i < kMaxMessages; ++i) g_Messages[i - 1] = g_Messages[i];
        --g_MessageCount;
    }
    Message& m = g_Messages[g_MessageCount];
    CopyText(m.text, sizeof(m.text), text);
    m.repeats = 1;
    m.shownAt = -1.0f;
    ++g_MessageCount;
}

void BeginDrawing() {
    g_ShapeCount[g_Filling] = 0;
}

void EndDrawing() {
    g_Finished = g_Filling;
    g_Filling = g_Filling ^ 1u;
    g_FinishedSerial = g_FinishedSerial + 1;
}

void Draw() {
    if (!Gui::IsReady()) return;   // fonts are still loading
    float now = 0.0f;
    Gui::TimeSeconds(&now);
    DrawShapes();
    DrawMessages(now);
}

} // namespace Engine

// --- botw.h ------------------------------------------------------------------

void print(const char* format, ...) {
    char text[256];   // the log gets all of it; the screen shows the first 99 characters
    va_list args;
    va_start(args, format);
    Engine::Format(text, sizeof(text), format, args);
    va_end(args);
    Engine::Show(text);
}

void draw_text(float x, float y, const char* format, ...) {
    va_list args;
    va_start(args, format);
    AddText(x, y, CREAM, format, args);
    va_end(args);
}

void draw_colored_text(float x, float y, Color color, const char* format, ...) {
    va_list args;
    va_start(args, format);
    AddText(x, y, color, format, args);
    va_end(args);
}

void draw_box(float x, float y, float width, float height, Color color) {
    Shape* shape = NewShape();
    if (!shape) return;
    shape->kind = kBox;
    shape->x = x;
    shape->y = y;
    shape->w = width;
    shape->h = height;
    shape->color = color;
}
