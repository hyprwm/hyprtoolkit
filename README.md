## hyprtoolkit
A modern C++ Wayland-native GUI toolkit

![](./assets/preview.png)

## What Hyprtoolkit is

Hyprtoolkit is designed to be a small, simple, and modern C++ toolkit for making wayland GUI apps, with
a few goals:

- Simple C++ API for making a GUI app
- Smooth animations
- Easy usage
- Simple system theming

## Building

Standard CMake build:
```sh
cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr -S . -B ./build
cmake --build ./build --config Release --target all -j`nproc 2>/dev/null || getconf NPROCESSORS_CONF`
```

### What Hyprtoolkit is not

Hyprtoolkit is not:
- cross-platform
- packed with crazy features
