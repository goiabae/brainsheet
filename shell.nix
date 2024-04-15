{ pkgs ? import <nixpkgs> {} }:
with pkgs; mkShell {
  nativeBuildInputs = [
    stdenv.cc
    cmake
  ];
}
