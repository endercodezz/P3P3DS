***

# Persona 3 Dual — Asset Pipeline (OUTDATED)

> Everything in `assets/` is source. Everything in `source/` or `data/` are generated.
> **Never hand-edit generated files** — run `make assets` instead.

---

## Quick Start

```bash
# First time only — set up venv (No external dependencies like Pillow required!)
python3 -m venv ~/.venv
source ~/.venv/bin/activate

# Full build (assets + ROM)
make

# Asset conversion only
make assets

# One category at a time
make dialogue
make music
make video
make environments
make models
make maps

# See all targets and flags
make help
```

---

## Configuration (`.build.json`)

The pipeline uses a unified Python wrapper (`build_asset.py`) to process assets. Instead of cluttering filenames with crop coordinates or overriding Makefile variables, **all asset configuration is now handled via `.build.json` sidecar files.**

Place a `.build.json` file next to your asset with the same base name (e.g., `tartarus.png` and `tartarus.build.json`). For grouped assets like environments or models, you can also place a directory-level `.build.json` to apply settings to everything inside that folder.

---

## Directory Layout

```text
assets/
  dialogue/       .dlg scripts          → source/dialogue/
  music/          .mp3 music            → data/music/
  sfx/            .wav SFX              → soundbank.bin (via mmutil)
  video/          .mp4 cutscenes        → data/video/
  environments/   .obj static rooms     → data/environments/ & source/environments/
  models/         .json animated models → data/models/ & source/models/
  maps/           .png collision maps   → source/maps/

source/
  dialogue/       *_dialogue.h + .cpp   (auto-generated)
  environments/   *_env.h               (auto-generated wrappers)
  models/         *.h                   (auto-generated animation enums)
  maps/           *.h                   (auto-generated collision arrays)

data/
  music/          *.pcm                 (auto-generated)
  video/          *.vid                 (auto-generated)
  environments/   *.bin                 (auto-generated display lists)
  models/         *.bin                 (auto-generated rigs/tracks)

tools/
  build_asset.py          (Unified configuration wrapper)
  dlg2dialogue.py
  build_environment.py
  obj2environment.py
  obj2model.py
  video2vid.py
```

---

## Tools Reference

---

### `build_environment.py` — 3D Static Environment Pipeline

Converts an `.obj` + `.mtl` + `.png` collection into a zero-boilerplate C++ header, a raw `.bin` display list file, and auto-generated `.grit` sidecar files for native Make compilation.

*(See `ENVIRONMENT_README.md` for full details).*

| | |
|---|---|
| **Input** | `assets/environments/<name>/<name>.obj` (plus textures) |
| **Output** | `data/environments/<name>.bin`, `source/environments/<name>_env.h`, and `.grit` sidecars |
| **Config** | `assets/environments/<name>/<name>.build.json` (e.g., `{"no_center": true, "source_blender": true}`) |

---

### `obj2model.py` — 3D Animated Model Converter

Compiles a hierarchical `.json` skeleton and per-bone `.obj` parts into a highly optimized binary format and a C++ header containing animation enums. Strips redundant keyframes automatically to save NDS memory.

*(See `MODEL_README.md` for full details on Blockbench/Blender plugin usage).*

| | |
|---|---|
| **Input** | `assets/models/<name>/<name>.json` (plus isolated `.obj`s) |
| **Output** | `data/models/<name>.bin` and `source/models/<name>.h` |
| **Config** | `assets/models/<name>/<name>.build.json` (e.g., `{"texsize": [128, 128], "source_blender": true}`) |

### `dlg2dialogue.py` — Dialogue Compiler

Compiles a `.dlg` scene script into a C++ header + source pair for the `DialogueController`. Generates robust background loading arrays, choice branching, and label-based jump matrices.

| | |
|---|---|
| **Input** | `assets/dialogue/<name>.dlg` |
| **Output** | `source/dialogue/<name>_dialogue.h` + `.cpp` |
| **Config** | `<name>.build.json` (e.g., `{"stdout": true}` for debugging) |

---

### `ffmpeg` — Music Converter (PCM)

Converts MP3s to raw 16-bit stereo PCM at 32 kHz for `maxmod`.

| | |
|---|---|
| **Input** | `assets/music/<name>.mp3` |
| **Output** | `data/music/<name>.pcm` |

---

### `video2vid.py` — Video Encoder

Encodes an MP4 video into a custom, interleaved `.vid` format containing raw video frames (pal8 or bgr555) perfectly synced with 16-bit PCM audio chunks.

| | |
|---|---|
| **Input** | `assets/video/<name>.mp4` |
| **Output** | `data/video/<name>.vid` |
| **Config** | `<name>.build.json` (e.g., `{"fps": 24, "bits": 16, "size": "256x192"}`) |

---

## Incremental Builds

Make tracks input → output dependencies. Re-running `make assets` only reconverts files whose source is **newer** than the output (this includes changes to `.build.json` files). To force a full reconvert:

```bash
make clean && make assets
```

To force a single category without cleaning:

```bash
touch assets/environments/dorm/dorm.obj && make environments
```
