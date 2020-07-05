# Tabla de contenidos

* [Plataformas](#plataformas)
* [Descarga el código fuente](#descarga-el-código-fuente)
* [Dependencias](#dependencias)
  * [Dependencias de Windows](#dependendias-de-windows)
  * [Dependencias de Mac OS X](#dependencies-de-mac-os-x)
  * [Dependencias de Linux ](#dependencias-de-linux)
* [Compilar](#compilar)
  * [Detalles de Windows](#detalles-de-windows)
  * [Detalles de Mac OS X](#detalles-de-mac-os-x)
    * [Problemas con Retina displays](#problemas-con-retina-displays)
  * [Detalles de Linux](#detalles-de-linux)
* [Usando librerías compartidas de terceros](#usando-librerias-compartidas-de-terceros)
  * [Problemas en Linux](#problemas-en-linux)
* [Compilar Skia dependencia](#compilar-skia-dependencia)
  * [Skia en Windows](#skia-en-windows)
  * [Skia en Mac OS X](#skia-en-mac-os-x)

# Plataformas

Deberías poder compilar LibreSprite sin problemas en las siguientes plataformas:

* Windows 10 + VS2015 Community Edition + Windows 10 SDK
* Mac OS X 10.11.4 El Capitan + Xcode 7.3 + OS X 10.11 SDK + Skia (Sin GPU)
* Linux + gcc 4.8 con soporte de C++11

# Descarga el código fuente

Recuerda que ya no es necesario que compiles el codigo tu mismo, puedes descargar ya los instaladores desde [aqui](https://libresprite.github.io/)

Puedes obtener el código fuente descargándolo como archivo zip o tar.gz desde el repositorio principal de LibreSprite:

https://github.com/LibreSprite/LibreSprite

O puedes clonar el repositorio y todos sus sub-módulos usando el siguiente comando:

    git clone --recursive https://github.com/LibreSprite/LibreSprite

Para actualizar un repositorio clonado ya existente puedes usar los siguientes comandos 

    cd LibreSprite
    git pull
    git submodule update --init --recursive

Puedes usar [Git para Windows](https://git-for-windows.github.io/) para clonar el repositorio en Windows.

# Dependencias

Para compilar LibreSprite necesitaras

* La ultima version de [CMake](http://www.cmake.org/) (3.4 or superior)
* [Ninja](https://ninja-build.org) build system

LibreSprite puede ser compilado con dos diferentes back-ends:

1. Allegro back-end (Windows, Linux): No necesitaras ninguna librería extra por que el repositorio ya contiene una version modificada de la librería Allegro. Este back-end ya esta obsoleto y sera removido en futuras versiones. Todo el nuevo desarrollo sera hecho con el nuevo Skia back-end.

2. Skia back-end (Windows, Mac OS X): Necesitaras una version compilada de la librería Skia. Por favor revisa los detalles de [Como compilar Skia](#compilar-skia-dependencia) en tu plataforma.

## Dependencias de Windows

Primero que nada necesitaras una pequeña herramienta llamada: `awk`, esta se usa para compilar la librería libpng. Puedes descargar esta herramienta desde distribuciones MSYS2 como [MozillaBuild](https://wiki.mozilla.org/MozillaBuild).

Después de eso tendrás que elegir un back-end:
1. Si eliges Allegro back-end, puedes pasar directamente a la sección de [Compilar](#compilar). 

Nota: Este back-end ya esta obsoleto y sera removido en futuras versiones. Todo el nuevo desarrollo sera hecho con el nuevo Skia back-end.

2. Si eliges Skia back-end necesitaras [compilar Skia](#compilar-skia-dependencia) antes de continuar a la sección de [Compilar](#compilar), recuerda revisar la sección [detalles de Windows](#detalles-de-windows) para saber como usar correctamente `cmake`.

## Dependencias de Mac OS X

En OS X necesitaras el SDK de Mac OS X 10.11 y Xcode 7.3 (Quizá versiones anteriores funcionen, si es asi háznoslo saber).

También, necesitaras compilar [Skia en Mac OS X](#skia-en-mac-os-x) antes de empezar con la [compilación](#compilar).

## Dependencias de Linux

Necesitaras las siguientes dependencias:
(Ubuntu, Debian):

    sudo apt-get update -qq
    sudo apt-get install -y g++ libx11-dev libxcursor-dev cmake ninja-build

(Manjaro, Archlinux)
En ArchLinux y derivadas algunas de estas dependencias ya vienen incluida con la distribución tal vez solo necesites:  
    
    sudo pacman -S cmake

Si hace falta alguna dependencia extra por favor revisa el gestor de paquetes de tu distribución y descargala.

El paquete `libxcursor-dev` es necesario [esconder el cursor](https://github.com/aseprite/aseprite/issues/913).

## Compilar

1. [Descarga el código fuente](#descarga-el-código-fuente), (si lo descargaste como archivo zip o tar.gz descomprimirlo), colocalo en una carpeta como `C:\LibreSprite`, y crea un nuevo directorio dentro de esta carpeta llamado `build`, donde irán todos los archivos generados durante la compilación (`.exe`,`.lib`, `.obj`, `.a`, `.o`, etc).

        cd C:\LibreSprite
        mkdir build

De esta manera si quieres empezar con una copia nueva del código fuente de LibreSprite, puedes solo eliminar el directorio `build` y empezar de nuevo.

2. Entra al directorio y ejecuta `cmake`:

        cd C:\LibreSprite\build
        cmake -G Ninja ..

Aquí `cmake` necesitara diferentes opciones dependiendo de tu sistema operativo, Revisa los detalles para las diferentes plataformas  [Windows](#detalles-de-windows), [Mac OS X](#detalles-de-mac-os-x) y [Linux](#detalles-de-linux). Algunas opciones de `cmake` pueden ser modificadas usando herramientas como [`ccmake`](https://cmake.org/cmake/help/latest/manual/ccmake.1.html)
   o [`cmake-gui`](https://cmake.org/cmake/help/latest/manual/cmake-gui.1.html).

3. Una vez ejecutado el comando `cmake`, tendrás que compilar el proyecto:

        cd C:\LibreSprite\build
        ninja libresprite

4. Cuando `ninja` haya terminado la compilación, podrás encontrar los ejecutables dentro de `C:\LibreSprite\build\bin\libresprite.exe`.

## Detalles de Windows

Para elegir Skia back-end ([Despues de que lo hayas compilado](#skia-en-windows)), puedes ejecutar el comando `cmake` con los siguientes argumentos:

    cd LibreSprite
    mkdir build
    cd build
    cmake -DUSE_ALLEG4_BACKEND=OFF -DUSE_SKIA_BACKEND=ON -DSKIA_DIR=C:\deps\skia -G Ninja ..
    ninja libresprite

En este caso `C:\deps\skia` es el directorio donde Skia fue compilado como lo describe la sección [Skia en Windows](#skia-en-windows)

## Detalles de Mac OS X

Despues de compilar [Skia en Mac OS X](#skia-en-mac-os-x) deberás ejecutar el comando `cmake` con los siguientes parámetros y luego `ninja`:

    cd LibreSprite
    mkdir build
    cd build
    cmake \
      -DCMAKE_OSX_ARCHITECTURES=x86_64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=10.7 \
      -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk \
      -DUSE_ALLEG4_BACKEND=OFF \
      -DUSE_SKIA_BACKEND=ON \
      -DSKIA_DIR=$HOME/deps/skia \
      -DWITH_HarfBuzz=OFF \
      -G Ninja \
      ..
    ninja libresprite

En este caso `$HOME/deps/skia` es el directorio donde Skia fue compilado como lo describe la sección [Skia en Mac OS X](#skia-en-mac-os-x).

### Problemas con Retina displays

Si tienes una Retina display, revisa el siguiente hilo: 

  https://github.com/aseprite/aseprite/issues/589

## Detalles de Linux

En linux puedes especificar el directorio donde deseas instalar LibreSprite antes de usar el comando `ninja install` como por ejemplo: 

    cd LibreSprite
    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=~/software -G Ninja ..
    ninja libresprite

Entonces puedes usar el comando `ninja install` y este copiara el programa en el directorio que especificaste(ejemplo  `~/software/bin/libresprite` en linux)

# Usando librerías compartidas de terceros

Si no quieres usar librerías de terceros embebidas en el código (como por ejemplo en el caso que quieras usar las que tienes instaladas en tu sistema) puedes deshabilitar los enlaces estáticos de configuración en cada `USE_SHARED_` opción.

Después de ejecutar el comando `cmake -G`, puedes editar el archivo `build/CMakeCache.txt` y habilitar la opción `USE_SHARED_` (coloca el valor en `ON`) de cada librería que quieras que se enlace dinamicamente.

## Problemas en Linux

Si usas la version oficial de la librería Allegro 4.4 (Por ejemplo compilas com `USE_SHARED_ALLEGRO4=ON`) experimentaras un par de problemas ya resueltos en [nuestra version parchada de la libreria Allegro 4.4](https://github.com/LibreSprite/LibreSprite/tree/master/src/allegro):

* Podrías [no ser capaz de reajustar el tamaño de la ventana](https://github.com/aseprite/aseprite/issues/192)
  ([Parche](https://github.com/LibreSprite/LibreSprite/commit/920f6275d55113507121afcbcda80adb44cc0563)).
* Tendras problemas
  [añadiendo colores HSV en sistemas no ingleses](https://github.com/LibreSprite/LibreSprite/commit/27b55030e26e93c5e8d9e7e21206c8709d46ff22)
  usando el icono de advertencia.

# Compilar Skia dependencia

Cuando compilas LibreSprite con [Skia](https://skia.org) como back-end en windows o os X, necesitaras copilar una version especifica de skia. En la siguiente sección encontraras paso a paso como compilar Skia.

Siempre puedes revisar las [instrucciones oficiales de Skia](https://skia.org/user/quick) y seleccionar el SO para el que vas a compilar, LibreSprite usa la rama de Skia `aseprite-m53` desde `https://github.com/aseprite/skia`.

## Skia en Windows 

Descarga [Google depot tools](https://storage.googleapis.com/chrome-infra/depot_tools.zip) descomprimelo y colocalo en un algun directorio como por ejemplo: `C:\deps\depot_tools`.

Abre la linea de comandos y sigue los siguientes pasos (para VS2015):

    call "%VS140COMNTOOLS%\vsvars32.bat"
    set PATH=C:\deps\depot_tools;%PATH%
    cd C:\deps\depot_tools
    gclient sync

(El comando `gclient` pueda que de un error como:
`Error: client not configured; see 'gclient config'`.
Solo ignoralo.)

    cd C:\deps
    git clone https://github.com/aseprite/skia.git
    cd skia
    git checkout aseprite-m53
    python bin/sync-and-gyp

(El comando `bin/sync-and-gyp` tomara algunos minutos ya que descarga muchos paquetes, por favor espera y vuelve a ejecutar el comando en caso de que este falle.)

    ninja -C out/Release dm

Para mas información acerca de estos pasos puedes revisar la [documentacion oficial de Skia](https://skia.org/user/quick/windows).

## Skia en Mac OS X

Estos pasos crearan una carpeta llamada `deps` en tu directorio Home con algunos sub directorios necesarios para la compilación de Skia (puedes cambiar `$HOME/deps` con otro directorio). Algunos de estos comandos tomaran varios minutos en finalizar, asi que ten paciencia: 

    mkdir $HOME/deps
    cd $HOME/deps
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
    git clone https://github.com/aseprite/skia.git
    export PATH="${PWD}/depot_tools:${PATH}"
    cd skia
    git checkout aseprite-m53
    python bin/sync-and-gyp
    ninja -C out/Release dm

Después de esto tendrás todas las librerias de Skia compiladas, cuando [compiles LibreSprite](#compilar) recuerda añadir el parametro `-DSKIA_DIR=$HOME/deps/skia` cuando ejecutes `cmake`, tal como lo describe la seccion [Detalles de Mac OS X](#detalles-de-mac-os-x).

Para mas información acerca de estos pasos puedes revisar la [documentacion oficial de Skia](https://skia.org/user/quick/macos).