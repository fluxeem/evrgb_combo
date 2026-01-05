@page samples_guide 示例程序指南

本页汇总 sample/user_examples 下的示例及运行方式。启用示例需在 CMake 配置时设置 `BUILD_SAMPLES=ON`（默认开启）。

## 快速构建与运行
- 配置：`cmake -B build -S . -DBUILD_SAMPLES=ON`
- 构建（推荐 Release）：
  - Linux/macOS: `cmake --build build --config Release --target <sample_target>`
  - Windows: `cmake --build build --config Release --target <sample_target>`
- 可执行文件路径：Linux 在 `build/bin`，Windows 在 `build/bin/Release`。

## 示例列表
### camera_enum_example
- 作用：枚举 RGB 和 DVS 相机，打印厂商、序列号、分辨率。
- 依赖：Hikrobot MVS SDK。
- 运行：
  - Linux/macOS: `cd build && ./bin/camera_enum_example`
  - Windows: `cd build && .\bin\Release\camera_enum_example.exe`

### combo_sync_display_example
- 作用：实时同步预览；将 DVS 事件叠加到 RGB，可选录制到 `recordings/`。
- 依赖：Hikrobot MVS SDK、OpenCV、Dvsense 驱动、显示环境。
- 运行：
  - Linux/macOS: `cd build && ./bin/combo_sync_display_example`
  - Windows: `cd build && .\bin\Release\combo_sync_display_example.exe`
- 控制：空格 开/关录制；`m` 叠加/分屏；方向键 调整事件叠加偏移；`q` 或 ESC 退出。
- 说明：默认使用首个检测到的 RGB 与 DVS；录制输出在 `recordings/`，含 mp4 与事件文件。

### recorded_replay_example
- 作用：回放由 combo_sync_display_example 或 SyncedDataRecorder 生成的同步录制。
- 依赖：OpenCV（运行时）；无需相机。
- 运行：
  - Linux/macOS: `cd build && ./bin/recorded_replay_example [recording_dir]`
  - Windows: `cd build && .\bin\Release\recorded_replay_example.exe [recording_dir]`
- 控制：空格 慢动作切换；`q` 或 ESC 退出。
- 说明：默认 recording_dir 为 recordings；在 RGB 帧上用红/蓝叠加 ON/OFF 事件。

### rgb_exposure_step_example
- 作用：手动设置并步进 RGB 曝光时间。
- 依赖：Hikrobot MVS SDK，OpenCV（显示）。
- 运行：
  - Linux/macOS: `cd build && ./bin/rgb_exposure_step_example [exposure_us]`
  - Windows: `cd build && .\bin\Release\rgb_exposure_step_example.exe [exposure_us]`
- 控制：`+`/`=` 增加曝光；`-`/`_` 减少曝光；`q` 或 ESC 退出。
- 说明：关闭自动曝光，设置 ExposureMode=Timed，然后应用请求的曝光（按相机范围裁剪）；默认使用首个 RGB 相机。

### beam_splitter_align
- 作用：分束镜对齐工具；实时同步预览，支持交互式调整仿射变换（平移/缩放）来对齐 DVS 和 RGB 图像。
- 依赖：Hikrobot MVS SDK、OpenCV、Dvsense 驱动、显示环境。
- 运行：
  - Linux/macOS: `cd build && ./bin/beam_splitter_align [metadata_path]`
  - Windows: `cd build && .\bin\Release\beam_splitter_align.exe [metadata_path]`
- 控制：方向键 调整仿射平移；`+`/`=` 放大仿射；`-`/`_` 缩小仿射；空格 开/关录制；`q` 或 ESC 退出。
- 说明：
  - 默认使用 BEAM_SPLITTER 排列方式，同步延迟 100ms。
  - 支持加载/保存元数据（包括内参和校准信息），默认路径为 `combo_metadata.json`。
  - 录制输出在 `recordings/`，含 mp4 视频和事件文件。
  - 适用于分束镜光学系统的精确对齐和校准。

## 提示
- 运行流式示例前，至少连接一台 RGB 与一台 DVS，相机默认取首个检测到的设备。
- 若使用自定义构建目录，请对应调整可执行文件路径（示例：`cmake --build <dir> --config Release --target <sample_target>`）。
- 如需内部 dev_testing 示例，启用 `BUILD_DEV_SAMPLES`（默认关闭）。
