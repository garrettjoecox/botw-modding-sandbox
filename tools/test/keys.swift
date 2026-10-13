// keys - send macOS key events, for driving Cemu from scripts.
//   keys "d:6 w:300 t:49 u:6"
//   d:<key>  key down        u:<key>  key up
//   t:<key>  tap (120 ms)    w:<ms>   wait
// Keys are macOS virtual keycodes (see CLAUDE.md for the user's binds). The
// terminal needs Accessibility permission (System Settings > Privacy).
import ApplicationServices
import Foundation

for token in CommandLine.arguments.dropFirst().joined(separator: " ").split(separator: " ") {
    let parts = token.split(separator: ":")
    guard parts.count == 2, let value = Int(parts[1]) else { continue }
    func post(_ down: Bool) {
        CGEvent(keyboardEventSource: nil, virtualKey: CGKeyCode(value), keyDown: down)!.post(tap: .cghidEventTap)
    }
    switch parts[0] {
    case "d": post(true)
    case "u": post(false)
    case "t": post(true); usleep(120_000); post(false)
    case "w": usleep(useconds_t(value * 1000))
    default: break
    }
    usleep(30_000)
}
