# Development shell for NixOS / Nix on Linux.
#
#   nix-shell                      # PC host build: cmake, ninja, GCC 16, Python + Pillow
#   p3p-get-devkitpro              # once: copy devkitPro (3ds-dev) from the official
#                                  # devkitpro/devkitarm Docker image into .cache/devkitpro
#   p3p-3ds                        # FHS shell where devkitPro's prebuilt binaries run
#
# devkitPro is not packaged in nixpkgs and ships glibc binaries for ordinary
# Linux distributions; p3p-3ds is a buildFHSEnv so they run unmodified.
# DEVKITPRO defaults to .cache/devkitpro (workspace containment, CLAUDE.md
# section 2); export DEVKITPRO before nix-shell to use another install.
{ pkgs ? import <nixpkgs> { } }:

let
  python = pkgs.python3.withPackages (ps: [ ps.pillow ]);

  # Tools for configuring and driving the 3DS build inside the FHS env.
  fhsTools = p: [
    p.bashInteractive
    p.coreutils
    p.gnumake
    p.cmake
    p.ninja
    p.git
    p.which
    p.file
    (p.python3.withPackages (ps: [ ps.pillow ]))
    # Runtime libraries the devkitARM / picasso / tex3ds / 3dsxtool binaries link against.
    p.stdenv.cc.cc.lib
    p.zlib
    p.zstd
    p.xz
    p.bzip2
    p.expat
    p.ncurses
  ];

  p3p-3ds = pkgs.buildFHSEnv {
    name = "p3p-3ds";
    targetPkgs = fhsTools;
    profile = ''
      export DEVKITPRO="''${DEVKITPRO:-$PWD/.cache/devkitpro}"
      export DEVKITARM="$DEVKITPRO/devkitARM"
      export PATH="$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH"
    '';
    runScript = "bash";
  };

  p3p-get-devkitpro = pkgs.writeShellScriptBin "p3p-get-devkitpro" ''
    set -euo pipefail
    dest="''${DEVKITPRO:-$PWD/.cache/devkitpro}"
    image="''${P3P_DEVKITPRO_IMAGE:-devkitpro/devkitarm:latest}"
    if [ -d "$dest/devkitARM" ]; then
      echo "devkitPro already present in $dest"; exit 0
    fi
    engine="$(command -v docker || command -v podman || true)"
    if [ -z "$engine" ]; then
      echo "docker or podman is required to fetch $image" >&2; exit 1
    fi
    "$engine" pull "$image"
    cid="$("$engine" create "$image")"
    trap '"$engine" rm -f "$cid" >/dev/null' EXIT
    mkdir -p "$(dirname "$dest")"
    "$engine" cp "$cid:/opt/devkitpro" "$dest"
    echo "devkitPro copied to $dest"
  '';
in
(pkgs.mkShell.override { stdenv = pkgs.gcc16Stdenv; }) {
  name = "p3p3ds";

  packages = [
    pkgs.cmake
    pkgs.ninja
    pkgs.git
    python
    pkgs.glibc.static # the host targets link with -static
    p3p-3ds
    p3p-get-devkitpro
  ];

  # Keep temporary files inside the workspace (CLAUDE.md section 2).
  shellHook = ''
    mkdir -p .tmp
    export TMPDIR="$PWD/.tmp" TEMP="$PWD/.tmp" TMP="$PWD/.tmp"
    export DEVKITPRO="''${DEVKITPRO:-$PWD/.cache/devkitpro}"
    export DEVKITARM="$DEVKITPRO/devkitARM"
  '';
}
