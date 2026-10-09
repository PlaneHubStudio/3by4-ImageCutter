import AppKit
import ImageIO
import CoreImage
import UniformTypeIdentifiers

struct Layout {
    let count: Int, unit: Int, x: Int, y: Int
    var width: Int { count * 3 * unit }
    var height: Int { 4 * unit }
}
func layout(_ w: Int, _ h: Int) -> Layout? {
    let choices = [2, 3].map { n -> Layout in
        let u = min(w / (3 * n), h / 4)
        return Layout(count: n, unit: u, x: (w - 3 * n * u) / 2, y: (h - 4 * u) / 2)
    }
    return choices.filter { $0.unit > 0 }.max { a, b in
        a.width * a.height < b.width * b.height
    }
}
func readImage(_ url: URL) throws -> CGImage {
    guard let source = CGImageSourceCreateWithURL(url as CFURL, nil),
          let cg = CGImageSourceCreateImageAtIndex(source, 0, nil) else {
        throw NSError(domain: "Crop", code: 1, userInfo: [NSLocalizedDescriptionKey: "无法读取图片，请选择 JPG、PNG、HEIC 或 TIFF 图片。"])
    }
    let props = CGImageSourceCopyPropertiesAtIndex(source, 0, nil) as? [CFString: Any]
    let orientation = (props?[kCGImagePropertyOrientation] as? NSNumber)?.int32Value ?? 1
    let ci = CIImage(cgImage: cg).oriented(forExifOrientation: orientation)
    guard let result = CIContext().createCGImage(ci, from: ci.extent) else {
        throw NSError(domain: "Crop", code: 2, userInfo: [NSLocalizedDescriptionKey: "图片解码失败。"])
    }
    return result
}
func export(_ cg: CGImage, to folder: URL, name: String) throws -> Layout {
    guard let l = layout(cg.width, cg.height) else {
        throw NSError(domain: "Crop", code: 3, userInfo: [NSLocalizedDescriptionKey: "图片尺寸过小。"])
    }
    try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
    for i in 0..<l.count {
        let rect = CGRect(x: l.x + i * 3 * l.unit, y: l.y, width: 3 * l.unit, height: l.height)
        guard let tile = cg.cropping(to: rect),
              let dest = CGImageDestinationCreateWithURL(folder.appendingPathComponent("\(name)_\(String(format: "%02d", i + 1)).png") as CFURL, UTType.png.identifier as CFString, 1, nil) else {
            throw NSError(domain: "Crop", code: 4, userInfo: [NSLocalizedDescriptionKey: "无法写入输出文件。"])
        }
        CGImageDestinationAddImage(dest, tile, nil)
        if !CGImageDestinationFinalize(dest) { throw NSError(domain: "Crop", code: 5, userInfo: [NSLocalizedDescriptionKey: "保存失败，请检查磁盘空间及文件夹权限。"]) }
    }
    return l
}

let accent = NSColor(calibratedRed: 0.16, green: 0.48, blue: 0.34, alpha: 1)
let ink = NSColor(calibratedRed: 0.16, green: 0.20, blue: 0.18, alpha: 1)

final class CutPreview: NSView {
    var tiles: [NSImage] = []
    var progress: CGFloat = 1
    var timer: Timer?
    func show(_ cg: CGImage, layout l: Layout, animated: Bool = true) {
        timer?.invalidate()
        tiles = (0..<l.count).compactMap { i in
            cg.cropping(to: CGRect(x: l.x + i * 3 * l.unit, y: l.y, width: 3 * l.unit, height: l.height)).map { NSImage(cgImage: $0, size: NSSize(width: 3*l.unit, height: l.height)) }
        }
        progress = animated && !NSWorkspace.shared.accessibilityDisplayShouldReduceMotion ? 0 : 1
        needsDisplay = true
        guard progress == 0 else { return }
        let start = Date()
        timer = Timer.scheduledTimer(withTimeInterval: 1.0/60, repeats: true) { [weak self] timer in
            guard let self = self else { timer.invalidate(); return }
            let t = min(1, max(0, (Date().timeIntervalSince(start) - 0.18) / 0.85))
            self.progress = CGFloat(1 - pow(1-t, 3)); self.needsDisplay = true
            if t >= 1 { timer.invalidate() }
        }
    }
    override func draw(_ dirtyRect: NSRect) {
        guard !tiles.isEmpty else {
            let box = NSBezierPath(roundedRect: bounds.insetBy(dx: 16, dy: 24), xRadius: 22, yRadius: 22)
            NSColor(calibratedWhite: 0.5, alpha: 0.18).setStroke(); box.setLineDash([6,6], count: 2, phase: 0); box.lineWidth = 1; box.stroke()
            let symbol = NSImage(systemSymbolName: "photo.on.rectangle.angled", accessibilityDescription: nil)!
            symbol.draw(in: NSRect(x: bounds.midX-24, y: bounds.midY+12, width: 48, height: 42))
            let text = "把图片拖到这里"
            let attrs: [NSAttributedString.Key: Any] = [.font: NSFont.systemFont(ofSize: 16, weight: .medium), .foregroundColor: ink]
            let size = text.size(withAttributes: attrs)
            text.draw(at: NSPoint(x: bounds.midX-size.width/2, y: bounds.midY-28), withAttributes: attrs)
            return
        }
        let n = CGFloat(tiles.count)
        let h = min(300, (bounds.width-58)/(n*0.75)), w = h*0.75
        let step = w*(1-0.11*progress), total = w+step*(n-1)
        let angles: [CGFloat] = tiles.count == 3 ? [-0.045,0.022,0.047] : [-0.035,0.035]
        let offsets: [CGFloat] = tiles.count == 3 ? [10,-9,5] : [9,-9]
        for (i, tile) in tiles.enumerated() {
            NSGraphicsContext.saveGraphicsState()
            let ctx = NSGraphicsContext.current!.cgContext
            ctx.translateBy(x: (bounds.width-total)/2+w/2+CGFloat(i)*step, y: bounds.midY+offsets[i]*progress)
            ctx.rotate(by: angles[i]*progress)
            let r = NSRect(x: -w/2, y: -h/2, width: w, height: h)
            if progress > 0.01 {
                let shadow = NSShadow(); shadow.shadowColor = NSColor.black.withAlphaComponent(0.16*progress); shadow.shadowBlurRadius = 17*progress; shadow.shadowOffset = NSSize(width: 0, height: -7*progress); shadow.set()
                NSColor.white.setFill(); NSBezierPath(roundedRect: r, xRadius: 5*progress, yRadius: 5*progress).fill()
                NSShadow().set()
            }
            NSBezierPath(roundedRect: r, xRadius: 5*progress, yRadius: 5*progress).addClip()
            tile.draw(in: r, from: .zero, operation: .sourceOver, fraction: 1)
            NSGraphicsContext.restoreGraphicsState()
        }
    }
}
final class CutMark: NSView {
    var timer: Timer?
    var phase: CGFloat = 0
    override init(frame: NSRect) {
        super.init(frame: frame)
        if !NSWorkspace.shared.accessibilityDisplayShouldReduceMotion {
            let start = Date()
            timer = Timer.scheduledTimer(withTimeInterval: 1.0/30, repeats: true) { [weak self] _ in
                let t = Date().timeIntervalSince(start).truncatingRemainder(dividingBy: 7)
                if t < 0.9 { self?.phase = 0 }
                else if t < 3.2 { self?.phase = CGFloat((t-0.9)/2.3) }
                else if t < 5.3 { self?.phase = 1 }
                else { self?.phase = CGFloat(max(0, 1-(t-5.3)/1.5)) }
                self?.needsDisplay = true
            }
        }
    }
    required init?(coder: NSCoder) { fatalError() }
    override func draw(_ dirtyRect: NSRect) {
        let border = min(1, max(0, (phase-0.15)/0.25))
        let raw = min(1, max(0, (phase-0.46)/0.44))
        let spread = raw*raw*(3-2*raw)
        let width: CGFloat = 120, height: CGFloat = width*4/9
        let x: CGFloat = 12, y: CGFloat = 7
        let ctx = NSGraphicsContext.current!.cgContext
        for i in 0..<3 {
            ctx.saveGState()
            let dx = CGFloat(i-1)*5*spread, dy = CGFloat([3,-3,1][i])*spread
            ctx.translateBy(x: dx,y: dy)
            ctx.clip(to: CGRect(x:x+CGFloat(i)*width/3,y:y,width:width/3,height:height))
            ctx.translateBy(x:x,y:y+(height+56*width/164)/2)
            ctx.scaleBy(x:width/164,y:-width/164)
            PlaneHubLogo.draw(in:ctx)
            ctx.restoreGState()
        }
        if border > 0 {
            for i in 1...2 {
                ctx.saveGState(); ctx.setStrokeColor(NSColor.white.withAlphaComponent(0.85*border*(1-spread)).cgColor);ctx.setLineWidth(1)
                let cut = x+CGFloat(i)*width/3
                ctx.move(to:CGPoint(x:cut,y:y+7));ctx.addLine(to:CGPoint(x:cut,y:y+height-7));ctx.strokePath();ctx.restoreGState()
            }
        }

    }
}
final class GreenButton: NSButton {
    var secondary = false
    var hovered = false
    var tracking: NSTrackingArea?
    override var isEnabled: Bool { didSet { needsDisplay = true } }
    override func updateTrackingAreas() {
        super.updateTrackingAreas()
        if let tracking = tracking { removeTrackingArea(tracking) }
        let area = NSTrackingArea(rect: .zero, options: [.mouseEnteredAndExited, .activeInKeyWindow, .inVisibleRect], owner: self, userInfo: nil)
        addTrackingArea(area); tracking = area
    }
    override func mouseEntered(with event: NSEvent) { hovered = true; needsDisplay = true }
    override func mouseExited(with event: NSEvent) { hovered = false; needsDisplay = true }
    override func resetCursorRects() { if isEnabled { addCursorRect(bounds, cursor: .pointingHand) } }
    override func draw(_ dirtyRect: NSRect) {
        let active = isEnabled && (hovered || isHighlighted)
        let fill: NSColor
        if secondary { fill = active ? accent.withAlphaComponent(0.055) : NSColor.clear }
        else { fill = isEnabled ? (active ? NSColor(calibratedRed: 0.10, green: 0.39, blue: 0.26, alpha: 1) : accent) : accent.withAlphaComponent(0.35) }
        fill.setFill()
        let path = NSBezierPath(roundedRect: bounds.insetBy(dx: 1, dy: 1), xRadius: 12, yRadius: 12); path.fill()
        if secondary && active { accent.withAlphaComponent(0.35).setStroke(); path.lineWidth = 1; path.stroke() }
        let color = secondary ? accent : NSColor.white
        let label = secondary ? "上传图片" : "导出"
        let attrs: [NSAttributedString.Key: Any] = [.font: NSFont.systemFont(ofSize: 14, weight: .semibold), .foregroundColor: color]
        let textSize = label.size(withAttributes: attrs)
        if secondary {
            label.draw(at: NSPoint(x: (bounds.width-textSize.width)/2, y: (bounds.height-textSize.height)/2), withAttributes: attrs)
        } else if let base = NSImage(systemSymbolName: "square.and.arrow.up", accessibilityDescription: label) {
            let icon = base.withSymbolConfiguration(NSImage.SymbolConfiguration(pointSize: 17, weight: .medium)) ?? base
            let iconHeight: CGFloat = 20
            let iconWidth = iconHeight * icon.size.width / icon.size.height
            let total = iconWidth + 9 + textSize.width
            let left = (bounds.width-total)/2
            let tinted = NSImage(size: NSSize(width: iconWidth,height: iconHeight), flipped: false) { r in
                icon.draw(in: r); color.setFill(); r.fill(using: .sourceAtop); return true
            }
            tinted.draw(in: NSRect(x: left,y: (bounds.height-iconHeight)/2,width: iconWidth,height: iconHeight))
            label.draw(at: NSPoint(x: left+iconWidth+9,y: (bounds.height-textSize.height)/2), withAttributes: attrs)
        }

    }
}

final class DropView: NSView {
    override func draw(_ dirtyRect: NSRect) { NSColor(calibratedRed: 0.965, green: 0.974, blue: 0.957, alpha: 1).setFill(); bounds.fill() }
    var onDrop: ((URL) -> Void)?
    override init(frame: NSRect) { super.init(frame: frame); registerForDraggedTypes([.fileURL]) }
    required init?(coder: NSCoder) { fatalError() }
    override func draggingEntered(_ sender: NSDraggingInfo) -> NSDragOperation { .copy }
    override func performDragOperation(_ sender: NSDraggingInfo) -> Bool {
        guard let urls = sender.draggingPasteboard.readObjects(forClasses: [NSURL.self], options: [.urlReadingFileURLsOnly: true]) as? [URL], let url = urls.first else { return false }
        onDrop?(url); return true
    }
}
final class App: NSObject, NSApplicationDelegate {
    var window: NSWindow!
    var image: CGImage?
    var source: URL?
    let preview = CutPreview()
    let status = NSTextField(labelWithString: "一张长图，连成一组。")
    let detail = NSTextField(labelWithString: "自动选择两张或三张 · 每张 3:4")
    let save = GreenButton(title: "导出", target: nil, action: nil)
    func applicationDidFinishLaunching(_ notification: Notification) {
        let menu = NSMenu(); let item = NSMenuItem(); menu.addItem(item)
        let appMenu = NSMenu(); appMenu.addItem(withTitle: "退出 3:4图片快切", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q"); item.submenu = appMenu; NSApp.mainMenu = menu
        window = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 700, height: 570), styleMask: [.titled, .closable, .miniaturizable, .fullSizeContentView], backing: .buffered, defer: false)
        window.title = "3:4图片快切"; window.titleVisibility = .hidden; window.titlebarAppearsTransparent = true; window.center()
        window.appearance = NSAppearance(named: .aqua)
        window.backgroundColor = NSColor(calibratedRed: 0.965, green: 0.974, blue: 0.957, alpha: 1)
        let root = DropView(frame: NSRect(x: 0, y: 0, width: 700, height: 570)); window.contentView = root
        root.onDrop = { [weak self] url in self?.load(url) }
        let title = NSTextField(labelWithString: "3:4图片快切")
        title.textColor = ink; title.font = .systemFont(ofSize: 25, weight: .semibold); title.frame = NSRect(x: 36, y: 492, width: 440, height: 36); root.addSubview(title)
        let hint = NSTextField(labelWithString: "让长图，自然连起来。")
        hint.textColor = NSColor(calibratedWhite: 0.48, alpha: 1); hint.font = .systemFont(ofSize: 13); hint.frame = NSRect(x: 37, y: 466, width: 440, height: 22); root.addSubview(hint)
        root.addSubview(CutMark(frame: NSRect(x: 525,y: 481,width: 150,height: 66)))
        preview.frame = NSRect(x: 22, y: 110, width: 656, height: 330); root.addSubview(preview)
        status.frame = NSRect(x: 36, y: 86, width: 628, height: 26); status.alignment = .center; status.textColor = ink; status.font = .systemFont(ofSize: 15, weight: .medium); root.addSubview(status)
        detail.frame = NSRect(x: 36,y: 62,width: 628,height: 22); detail.alignment = .center; detail.textColor = NSColor(calibratedWhite: 0.48, alpha: 1); detail.font = .systemFont(ofSize: 11); root.addSubview(detail)
        let choose = GreenButton(title: "上传图片", target: self, action: #selector(pick)); choose.secondary = true; choose.isBordered = false; choose.frame = NSRect(x: 36, y: 15, width: 120, height: 46); root.addSubview(choose)
        save.target = self; save.action = #selector(write); save.isBordered = false; save.frame = NSRect(x: 544, y: 15, width: 120, height: 46); save.isEnabled = false; root.addSubview(save)
        if !CommandLine.arguments.contains("--snapshot") { window.makeKeyAndOrderFront(nil); NSApp.activate(ignoringOtherApps: true) }
    }
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }
    func application(_ sender: NSApplication, openFiles filenames: [String]) { if let first = filenames.first { load(URL(fileURLWithPath: first)) }; sender.reply(toOpenOrPrint: .success) }
    @objc func pick() {
        let p = NSOpenPanel(); p.allowedContentTypes = [.jpeg, .png, .heic, .tiff, .bmp]; p.canChooseDirectories = false
        if p.runModal() == .OK, let url = p.url { load(url) }
    }
    func load(_ url: URL) {
        do {
            let cg = try readImage(url)
            guard let l = layout(cg.width, cg.height) else { throw NSError(domain: "Crop", code: 3, userInfo: [NSLocalizedDescriptionKey: "图片尺寸过小。"]) }
            image = cg; source = url; preview.show(cg, layout: l); save.isEnabled = true
            let loss = 100 * (1 - Double(l.width * l.height) / Double(cg.width * cg.height))
            status.stringValue = "将切分成 \(l.count) 张 3:4 图片"
            detail.stringValue = "每张 \(3*l.unit) × \(l.height) · " + (loss < 0.05 ? "无需裁剪" : "居中裁去约 \(String(format: "%.1f", loss))%")

        } catch { alert(error) }
    }
    @objc func write() {
        guard let cg = image, let url = source else { return }
        let p = NSOpenPanel(); p.title = "选择导出位置"; p.prompt = "导出"; p.canChooseFiles = false; p.canChooseDirectories = true; p.canCreateDirectories = true
        guard p.runModal() == .OK, let parent = p.url else { return }
        let name = url.deletingPathExtension().lastPathComponent
        var folder = parent.appendingPathComponent(name + "_连页")
        var suffix = 2
        while FileManager.default.fileExists(atPath: folder.path) { folder = parent.appendingPathComponent(name + "_连页_\(suffix)"); suffix += 1 }
        do { let l = try export(cg, to: folder, name: name); status.stringValue = "已导出 \(l.count) 张 3:4 图片"; detail.stringValue = "按 01、02、03 的顺序，连起来发布。"; NSWorkspace.shared.open(folder) } catch { alert(error) }
    }
    func alert(_ error: Error) { let a = NSAlert(error: error); a.runModal() }
}
if CommandLine.arguments.contains("--self-test") {
    for (w,h,n) in [(1500,1000,2),(2250,1000,3),(1600,1000,2),(2100,1000,3),(1000,1500,2)] {
        let l = layout(w,h)!; precondition(l.count == n); precondition(l.width <= w && l.height <= h); precondition(l.height * 3 == l.unit * 3 * 4)
    }
    let space = CGColorSpaceCreateDeviceRGB()
    let ctx = CGContext(data: nil, width: 2250, height: 1000, bitsPerComponent: 8, bytesPerRow: 0, space: space, bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue)!
    for i in 0..<3 { ctx.setFillColor([NSColor.red, .green, .blue][i].cgColor); ctx.fill(CGRect(x:i*750,y:0,width:750,height:1000)) }
    let folder = URL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent(UUID().uuidString)
    let l = try export(ctx.makeImage()!, to: folder, name: "test")
    for i in 0..<l.count { let cg = try readImage(folder.appendingPathComponent("test_\(String(format: "%02d", i+1)).png")); precondition(cg.width == 750 && cg.height == 1000) }
    try FileManager.default.removeItem(at: folder)
    print("PASS: ratio selection, centered bounds, PNG export and readback")
} else if CommandLine.arguments.contains("--snapshot") {
    _ = NSApplication.shared
    let delegate = App()
    delegate.applicationDidFinishLaunching(Notification(name: NSApplication.didFinishLaunchingNotification))
    if CommandLine.arguments.count > 3 {
        delegate.load(URL(fileURLWithPath: CommandLine.arguments[3]))
        delegate.preview.timer?.invalidate(); delegate.preview.progress = 1
    }
    let view = delegate.window.contentView!
    if CommandLine.arguments.count > 4, let phase = Double(CommandLine.arguments[4]) {
        for case let mark as CutMark in view.subviews { mark.timer?.invalidate(); mark.phase = CGFloat(phase) }
    }
    view.displayIfNeeded()
    let bitmap = view.bitmapImageRepForCachingDisplay(in: view.bounds)!
    view.cacheDisplay(in: view.bounds, to: bitmap)
    try bitmap.representation(using: .png, properties: [:])!.write(to: URL(fileURLWithPath: CommandLine.arguments[2]))
    print("Saved UI snapshot")
} else {
    let app = NSApplication.shared; let delegate = App(); app.delegate = delegate; app.setActivationPolicy(.regular); app.run()
}
