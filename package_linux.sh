#!/bin/sh

add()
{
  echo "adding $1"

  if [ -e "/usr/lib/x86_64-linux-gnu/$1" ]; then
      src="/usr/lib/x86_64-linux-gnu"
  elif [ -e "/usr/lib/x86_64-linux-gnu/pulseaudio/$1" ]; then
      src="/usr/lib/x86_64-linux-gnu/pulseaudio"
  elif [ -e "/usr/lib64/$1" ]; then
      src="/usr/lib64"
  fi

  if [ $src != $out ]; then
      cp "$src/$1" "$out/$1"
  fi

  dependencies=$(readelf -d "$src/$1" | grep NEEDED | sed -En "s/[^\[]*\[([^]]*)\S*/\1/gp")

  for dependency in $dependencies
  do
    if [ ! -f "$out/$dependency" ]; then
        add $dependency
    fi
  done
}

out=$(pwd)
src=$(pwd)

add "libresprite"

cp /usr/lib/libpthread.so* ./
cp /usr/lib/librt.so* ./
cp /usr/lib/libstdc++.so* ./

rm libc.so*
rm libm.so*
# rm libpthread.so*
# rm librt.so*
# rm libstdc++.so*

chmod +x libresprite

mkdir LibreSprite
mkdir LibreSprite/usr
mkdir LibreSprite/usr/bin
mkdir LibreSprite/usr/lib

mv ../../desktop/libresprite.desktop LibreSprite/
cp ../../desktop/icons/hicolor/16x16/apps/libresprite.png LibreSprite/libresprite.png

mv libresprite LibreSprite/usr/bin
mv data LibreSprite/usr/bin
mv *.so* LibreSprite/usr/lib

wget https://github.com/AppImage/AppImageKit/releases/download/13/AppRun-x86_64 -O LibreSprite/AppRun
chmod +x LibreSprite/AppRun

wget https://github.com/AppImage/AppImageKit/releases/download/13/appimagetool-x86_64.AppImage -O appimagetool
chmod +x appimagetool

./appimagetool LibreSprite
