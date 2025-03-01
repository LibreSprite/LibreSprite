#!/bin/sh


out=$(pwd)
src=$(pwd)

chmod +x libresprite

mkdir -p LibreSprite

mv ../../desktop/libresprite.desktop LibreSprite/
cp ../../desktop/icons/hicolor/256x256/apps/libresprite.png LibreSprite/libresprite.png

# Create AppImage with lib4bin and Sharun
(
export ARCH="$(uname -m)" # Just to be double sure
cd LibreSprite
wget "https://raw.githubusercontent.com/VHSgunzo/sharun/refs/heads/main/lib4bin" -O ./lib4bin
chmod +x ./lib4bin
xvfb-run -a -- ./lib4bin -p -v -e -r -k -w \
  /usr/bin/libresprite \
  /usr/bin/data \
  /usr/lib/libpthread.so* \
  /usr/lib/librt.so* \
  /usr/lib/libstdc++.so* 
ln ./sharun ./AppRun 
./sharun -g
)