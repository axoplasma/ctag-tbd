# tapp Terminal Application

A simple application to route TBD web ui activity through serial.

# Build for mac
first compile SDL and deploy locally:
```
cd SDL
./configure --prefix `pwd`/dist --disable-shared --disable-video-x11 --disable-audio --disable-sensor --disable-haptic --disable-video-metal --disable-video-vulkan --disable-video-opengles --disable-video-opengles1 --disable-video-opengles2
cd ..
```
the compile and package tapp
```
mkdir build
cd build
cmake ..
make
cpack
```

# Licenses
Packaged tapp application uses [https://github.com/espressif/esptool](https://github.com/espressif/esptool) for flashing,
which is [GPL](which is GPL licensed) licensed.
esptool has been packaged in binary form using [https://realpython.com/pyinstaller-python/](https://realpython.com/pyinstaller-python/) 
to be usable as executable without having a Python environment set up.