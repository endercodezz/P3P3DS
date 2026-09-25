# Android packaging

What the Android app is made of besides the native code. `scripts/release_android.sh` packs a release from these; [`docs/RELEASING.md`](../../../../docs/RELEASING.md#android) says how.

| File | What it is |
| --- | --- |
| `AndroidManifest.xml` | The app: `io.github.teamgdb.yakumo`, Android 10 (API 29) and Vulkan 1.1 required, landscape, the libraries extracted so the overlay loader can list them, the backup rules |
| `java/…/YakumoActivity.java` | SDL's activity plus what the host needs from Android: the display cutout, the document picker for folders and files, restarting the app |
| `res/` | The name, the theme without a title bar, the launcher icons (made by `make_icons.sh` from `docs/images/emblem.svg`) and the backup rules (saves and settings only) |
| `build_apk.sh` | Packs and signs an APK without Gradle, with the SDK's own tools |
| `make_icons.sh` | Makes the launcher icons again after the emblem changes; needs `rsvg-convert` and ImageMagick |

SDL's own Java classes come from the SDL3 source the release pins (`packaging/linux/sources.sh`), not from this directory.
