{
  lib,
  stdenv,
  cmake,
  pkg-config,
  aquamarine,
  hyprgraphics,
  hyprlang,
  hyprutils,
  hyprwayland-scanner,
  libGL,
  libdrm,
  libgbm,
  libxkbcommon,
  pango,
  pixman,
  wayland,
  wayland-protocols,
  wayland-scanner,
  version ? "git",
}:
stdenv.mkDerivation {
  pname = "hyprtoolkit";
  inherit version;

  src = ../.;

  nativeBuildInputs = [
    cmake
    pkg-config
    hyprwayland-scanner
    wayland-scanner
  ];

  buildInputs = [
    aquamarine
    hyprgraphics
    hyprlang
    hyprutils
    libGL
    libdrm
    libgbm
    libxkbcommon
    pango
    pixman
    wayland
    wayland-protocols
  ];

  meta = {
    homepage = "https://github.com/hyprwm/hyprtoolkit";
    description = "A modern C++ Wayland-native GUI toolkit";
    license = lib.licenses.bsd3;
    platforms = lib.platforms.linux;
  };
}
