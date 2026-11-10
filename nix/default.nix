{
  lib,
  stdenv,
  cmake,
  pkg-config,
  abseil-cpp,
  aquamarine,
  gtest,
  hyprgraphics,
  hyprlang,
  hyprutils,
  hyprwayland-scanner,
  iniparser,
  libgbm,
  libxkbcommon,
  wayland,
  wayland-protocols,
  wayland-scanner,
  version ? "git",
  doCheck ? false,
}:
let
  inherit (lib.strings) optionalString;
in
stdenv.mkDerivation {
  pname = "hyprtoolkit" + optionalString doCheck "-with-tests";
  inherit version doCheck;

  src = ../.;

  nativeBuildInputs = [
    cmake
    pkg-config
    hyprwayland-scanner
    wayland-scanner
  ];

  propagatedBuildInputs = [
    aquamarine
    hyprgraphics
    hyprutils
  ];

  buildInputs = [
    abseil-cpp
    gtest
    hyprlang
    iniparser
    libgbm
    libxkbcommon
    wayland
    wayland-protocols
    wayland-scanner
  ];

  env.XDG_RUNTIME_DIR = "/tmp/runtime";

  strictDeps = true;

  cmakeBuildType = if doCheck then "Debug" else "RelWithDebInfo";

  preCheck = ''
    mkdir -p /tmp/runtime
  '';

  meta = {
    homepage = "https://github.com/hyprwm/hyprtoolkit";
    description = "A modern C++ Wayland-native GUI toolkit";
    license = lib.licenses.bsd3;
    platforms = lib.platforms.linux;
  };
}
