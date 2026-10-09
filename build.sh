#!/bin/zsh
set -eu
cd "${0:A:h}"
mkdir -p dist/staging/'3比4图片快切.app'/Contents/MacOS
swiftc -module-cache-path /private/tmp/rednotecrop-module-cache Sources/main.swift Sources/PlaneHubLogo.swift -target arm64-apple-macosx11.0 -o dist/crop-arm64 -framework AppKit -framework ImageIO -framework CoreImage -O
swiftc -module-cache-path /private/tmp/rednotecrop-module-cache Sources/main.swift Sources/PlaneHubLogo.swift -target x86_64-apple-macosx11.0 -o dist/crop-x86_64 -framework AppKit -framework ImageIO -framework CoreImage -O
lipo -create dist/crop-arm64 dist/crop-x86_64 -output 'dist/staging/3比4图片快切.app/Contents/MacOS/RedNoteCrop'
cat > 'dist/staging/3比4图片快切.app/Contents/Info.plist' <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleExecutable</key><string>RedNoteCrop</string>
<key>CFBundleIconFile</key><string>AppIcon.icns</string>
<key>CFBundleIdentifier</key><string>local.photocut34.mac</string>
<key>CFBundleName</key><string>3比4图片快切</string>
<key>CFBundleDisplayName</key><string>3比4图片快切</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleShortVersionString</key><string>1.4</string>
<key>CFBundleVersion</key><string>9</string>
<key>LSMinimumSystemVersion</key><string>11.0</string>
<key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST
mkdir -p 'dist/staging/3比4图片快切.app/Contents/Resources'
cp Assets/AppIcon.icns Assets/PlaneHub-logo.svg 'dist/staging/3比4图片快切.app/Contents/Resources/'
codesign --force --sign - 'dist/staging/3比4图片快切.app'
'dist/staging/3比4图片快切.app/Contents/MacOS/RedNoteCrop' --self-test
ln -sfn /Applications dist/staging/Applications
cp README.txt dist/staging/使用说明.txt
if [[ "${1:-}" != "--app-only" ]]; then
hdiutil create -volname '3比4图片快切' -srcfolder dist/staging -ov -format UDZO 'dist/3比4图片快切.dmg'
fi
rm -f dist/crop-arm64 dist/crop-x86_64 dist/RedNoteCrop
