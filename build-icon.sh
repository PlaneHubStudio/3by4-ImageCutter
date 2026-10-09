#!/bin/zsh
set -eu
cd "${0:A:h}"
mkdir -p dist/AppIcon.iconset
for size in 16 32 128 256 512; do
  sips -z "$size" "$size" Assets/AppIcon.png --out "dist/AppIcon.iconset/icon_${size}x${size}.png" >/dev/null
  retina=$((size * 2))
  sips -z "$retina" "$retina" Assets/AppIcon.png --out "dist/AppIcon.iconset/icon_${size}x${size}@2x.png" >/dev/null
done
iconutil -c icns dist/AppIcon.iconset -o Assets/AppIcon.icns
mkdir -p 'dist/staging/3比4图片快切.app/Contents/Resources'
cp Assets/AppIcon.icns 'dist/staging/3比4图片快切.app/Contents/Resources/AppIcon.icns'
