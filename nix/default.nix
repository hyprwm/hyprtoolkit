{
  lib,
  stdenv,
  cmake,
  pkg-config,
  aquamarine,
  cairo,
  gtest,
  hyprgraphics,
  hyprlang,
  hyprutils,
  hyprwayland-scanner,
  iniparser,
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
  doCheck ? false,
}:
let
  inherit (lib.attrsets) mapAttrsToList;
  inherit (lib.strings) cmakeBool;
in
stdenv.mkDerivation {
  pname = "hyprtoolkit";
  inherit version doCheck;

  src = ../.;

  nativeBuildInputs = [
    cmake
    pkg-config
    hyprwayland-scanner
    wayland-scanner
  ];

  buildInputs = [
    aquamarine
    cairo
    gtest
    hyprgraphics
    hyprlang
    hyprutils
    iniparser
    libGL
    libdrm
    libgbm
    libxkbcommon
    pango
    pixman
    wayland
    wayland-protocols
  ];

  cmakeFlags = mapAttrsToList cmakeBool {
    "DISABLE_TESTING" = !doCheck;
  };

  meta = {
    homepage = "https://github.com/hyprwm/hyprtoolkit";
    description = "A modern C++ Wayland-native GUI toolkit";
    license = lib.licenses.bsd3;
    platforms = lib.platforms.linux;
  };
}
