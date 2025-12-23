@page samples_guide Samples Guide

This page summarizes the samples under sample/user_examples and how to run them. Samples build when CMake is configured with BUILD_SAMPLES=ON (default).

## Build and Run Quickly
- Configure: cmake -B build -S . -DBUILD_SAMPLES=ON
- Build a sample (Release config recommended):
  - Linux/macOS: cmake --build build --config Release --target <sample_target>
  - Windows: cmake --build build --config Release --target <sample_target>
- Binaries land in build/bin (Linux) or build/bin/Release (Windows).

## Samples
### camera_enum_example
- Purpose: enumerate RGB and DVS cameras, printing manufacturer, serial, and resolution.
- Dependencies: Hikrobot MVS SDK.
- Run:
  - Linux/macOS: cd build && ./bin/camera_enum_example
  - Windows: cd build && .\bin\Release\camera_enum_example.exe

### combo_sync_display_example
- Purpose: live synchronized preview; overlays DVS events onto RGB, optional recording to recordings/.
- Dependencies: Hikrobot MVS SDK, OpenCV, Dvsense driver, display server.
- Run:
  - Linux/macOS: cd build && ./bin/combo_sync_display_example
  - Windows: cd build && .\bin\Release\combo_sync_display_example.exe
- Controls: SPACE toggle recording; m toggle overlay/side-by-side; arrow keys shift event overlay; q or ESC to quit.
- Notes: uses the first detected RGB and DVS cameras; recordings are written under recordings/ with mp4 video and event files.

### recorded_replay_example
- Purpose: replay a synchronized recording produced by combo_sync_display_example or SyncedDataRecorder.
- Dependencies: OpenCV (runtime only); no cameras required.
- Run:
  - Linux/macOS: cd build && ./bin/recorded_replay_example [recording_dir]
  - Windows: cd build && .\bin\Release\recorded_replay_example.exe [recording_dir]
- Controls: SPACE toggle slow motion; q or ESC to quit.
- Notes: default recording_dir is recordings; overlays ON/OFF events in red/blue on the RGB frames.

### rgb_exposure_step_example
- Purpose: manually set and step the RGB exposure time on a Hikvision camera.
- Dependencies: Hikrobot MVS SDK, OpenCV (for display).
- Run:
  - Linux/macOS: cd build && ./bin/rgb_exposure_step_example [exposure_us]
  - Windows: cd build && .\bin\Release\rgb_exposure_step_example.exe [exposure_us]
- Controls: '+'/'=' increases exposure; '-'/'_' decreases exposure; q or ESC to quit.
- Notes: disables auto exposure, sets ExposureMode=Timed, then applies the requested exposure (clamped to camera limits). Uses the first detected RGB camera.

## Tips
- Keep at least one RGB and one DVS camera connected before running streaming samples; they pick the first detected device of each type.
- If you use a custom build directory, adjust the binary paths accordingly (cmake --build <dir> --config Release --target <sample_target>).
- Enable BUILD_DEV_SAMPLES if you also need the internal dev_testing examples (off by default).

