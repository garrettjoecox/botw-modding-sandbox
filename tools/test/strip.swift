// strip - stack images vertically into one PNG, to look at several screenshots
// at once:  strip out.png a.png b.png c.png
import AppKit

let args = CommandLine.arguments
guard args.count >= 3 else { print("usage: strip out.png in1.png [in2.png ...]"); exit(1) }
let images = args.dropFirst(2).map { NSImage(contentsOfFile: $0)! }
let width = images.map { $0.size.width }.max()!
let height = images.map { $0.size.height }.reduce(0, +)
let out = NSImage(size: NSSize(width: width, height: height))
out.lockFocus()
var y = height
for image in images {
    y -= image.size.height
    image.draw(at: NSPoint(x: 0, y: y), from: .zero, operation: .copy, fraction: 1)
}
out.unlockFocus()
let rep = NSBitmapImageRep(data: out.tiffRepresentation!)!
try! rep.representation(using: .png, properties: [:])!.write(to: URL(fileURLWithPath: args[1]))
