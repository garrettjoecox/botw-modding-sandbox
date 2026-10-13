// winid - print the window number of Cemu's biggest on-screen window (the game),
// for `screencapture -l <id>`. Exits 1 if Cemu has no window.
import CoreGraphics
import Foundation

let windows = CGWindowListCopyWindowInfo([.optionOnScreenOnly, .excludeDesktopElements], kCGNullWindowID)
    as? [[String: Any]] ?? []
var best: (number: Int, area: Double)?
for window in windows {
    guard (window[kCGWindowOwnerName as String] as? String) == "Cemu",
          let number = window[kCGWindowNumber as String] as? Int,
          let bounds = window[kCGWindowBounds as String] as? [String: Double] else { continue }
    let area = (bounds["Width"] ?? 0) * (bounds["Height"] ?? 0)
    if best == nil || area > best!.area { best = (number, area) }
}
guard let found = best else {
    FileHandle.standardError.write("winid: no Cemu window\n".data(using: .utf8)!)
    exit(1)
}
print(found.number)
