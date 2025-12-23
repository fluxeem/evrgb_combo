@page recording_tutorial EvRGB Combo SDK Recording Tutorial

This guide shows how to record synchronized RGB frames and DVS events using the SDK APIs and the sample app. See the reference code in [sample/user_examples/combo_sync_display_example.cpp](../../sample/user_examples/combo_sync_display_example.cpp) and the API declarations in [include/combo.h](../../include/combo.h).

## What gets recorded
- RGB video: MP4 written via OpenCV `VideoWriter` (`combo_rgb.mp4`).
- Frame timestamps: CSV with exposure start/end and frame index (`combo_timestamps.csv`).
- DVS raw stream: binary event dump (`combo_events.raw`).
- Output directory is controlled by `SyncedRecorderConfig::output_dir` in [include/synced_data_recorder.h](../../include/synced_data_recorder.h).

## Prerequisites
- RGB and DVS drivers installed and working.
- Build with `BUILD_SAMPLES=ON` if you want the sample app.
- OpenCV available to your toolchain (used by the recorder).

## Quick start: sample app
1. Configure & build (from repo root):
   ```bash
   mkdir -p build && cd build
   cmake .. -DBUILD_SAMPLES=ON
   cmake --build . --config Release
   ```
2. Run the synced display sample (target name may vary by generator; on Makefiles: `./sample/user_examples/combo_sync_display_example`).
3. Controls while running:
   - SPACE: toggle recording on/off.
   - `m`: toggle overlay vs side-by-side display.
   - Arrow keys: shift DVS overlay.
   - `q` or ESC: quit.
4. Files are written under `recordings/` by default (see `recorder_cfg` in the sample).

## Integration steps (code pattern)
1. Enumerate and pick devices:
   ```cpp
   auto [rgbs, dvss] = evrgb::enumerateAllCameras();
   evrgb::Combo combo(rgbs[0].serial_number, dvss[0].serial, /*max_buffer=*/100);
   ```
2. Create recorder + config:
   ```cpp
   evrgb::SyncedRecorderConfig cfg;
   cfg.output_dir = "recordings";
   cfg.fps = 30.0;
   cfg.fourcc = "mp4v";

   auto recorder = std::make_shared<evrgb::SyncedDataRecorder>();
   combo.setSyncedDataRecorder(recorder);
   ```
3. (Optional, for live display only) attach a synced callback. Recording does **not** require this; `startRecording` handles writing once a recorder is attached. Use a callback only if you want on-screen visualization or extra processing:
   ```cpp
   combo.setSyncedCallback([](const evrgb::RgbImageWithTimestamp& rgb,
                       const std::vector<dvsense::Event2D>& events) {
      // e.g., render or inspect data here
   });
   ```
4. Start streaming:
   ```cpp
   if (!combo.init()) return -1;
   if (!combo.start()) return -1;
   ```
5. Record on demand:
   ```cpp
   combo.startRecording(cfg);   // begins RGB MP4 + CSV + DVS raw
   // ... run your loop ...
   combo.stopRecording();
   ```
6. Shutdown cleanly:
   ```cpp
   combo.stop();
   // combo.destroy(); // optional explicit teardown
   ```

## Recorder configuration notes
- `output_dir`: directory will be created if missing; ensure it is writable.
- `fps`: used for the MP4 writer; choose a value close to your RGB stream rate.
- `fourcc`: codec tag; use one supported by your OpenCV build (e.g., `mp4v`, `avc1`).
- File names are fixed: `combo_rgb.mp4`, `combo_timestamps.csv`, `combo_events.raw` under `output_dir`.

## Tips and troubleshooting
- If recording does not start, check `output_dir` path and codec support for your OpenCV build.
- Empty or unsupported RGB frames are skipped with a warning; ensure your RGB callback delivers valid images.
- Large buffers (`max_buffer_size` in `Combo` constructor) help absorb processing spikes when recording.
- To capture DVS raw data, `startRecording` automatically enables it when a recorder is attached.

## Next steps
- Use the sample as a template and adapt `recorder_cfg` for your dataset naming and storage layout.
- Extend the CSV to include more metadata if needed by post-processing pipelines.
- Consider a higher `fps` or different `fourcc` when targeting slow-motion or higher-bitrate output.

*/
