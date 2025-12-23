@page recording_tutorial EvRGB Combo SDK 录制教程

本指南介绍如何使用 SDK API 和示例程序录制同步的 RGB 帧与 DVS 事件。参考示例代码 [sample/user_examples/combo_sync_display_example.cpp](../../sample/user_examples/combo_sync_display_example.cpp) 以及接口声明 [include/combo.h](../../include/combo.h)。

## 录制内容
- RGB 视频：通过 OpenCV `VideoWriter` 写出 MP4（文件名 `combo_rgb.mp4`）。
- 帧时间戳：CSV，包含曝光开始/结束时间与帧序号（`combo_timestamps.csv`）。
- DVS 原始流：二进制事件数据（`combo_events.raw`）。
- 输出目录由 [include/synced_data_recorder.h](../../include/synced_data_recorder.h) 中的 `SyncedRecorderConfig::output_dir` 控制。

## 前置条件
- RGB 和 DVS 驱动已正确安装并可用。
- 如需运行示例程序，请在构建时启用 `BUILD_SAMPLES=ON`。
- OpenCV 已在编译环境中可用（录制依赖 OpenCV）。

## 快速开始：示例程序
1. 配置并构建（项目根目录执行）：
   ```bash
   mkdir -p build && cd build
   cmake .. -DBUILD_SAMPLES=ON
   cmake --build . --config Release
   ```
2. 运行同步显示示例（目标名随生成器而异；Makefile 生成器下为 `./sample/user_examples/combo_sync_display_example`）。
3. 运行时控制：
   - 空格：开始/暂停录制。
   - `m`：叠加显示与左右分屏切换。
   - 方向键：调整 DVS 叠加偏移。
   - `q` 或 ESC：退出。
4. 默认输出路径为 `recordings/`（参见示例代码中的 `recorder_cfg`）。

## 集成步骤（代码模式）
1. 枚举并选择设备：
   ```cpp
   auto [rgbs, dvss] = evrgb::enumerateAllCameras();
   evrgb::Combo combo(rgbs[0].serial_number, dvss[0].serial, /*max_buffer=*/100);
   ```
2. 创建录制器与配置：
   ```cpp
   evrgb::SyncedRecorderConfig cfg;
   cfg.output_dir = "recordings";
   cfg.fps = 30.0;
   cfg.fourcc = "mp4v";

   auto recorder = std::make_shared<evrgb::SyncedDataRecorder>();
   combo.setSyncedDataRecorder(recorder);
   ```
3. （可选，仅用于实时显示）挂载同步回调。录制本身不需要回调；只要绑定了 recorder 并调用 `startRecording` 即会写文件。需要屏幕显示或额外处理时再添加：
   ```cpp
   combo.setSyncedCallback([](const evrgb::RgbImageWithTimestamp& rgb,
                       const std::vector<dvsense::Event2D>& events) {
      // 例如在此渲染或检查数据
   });
   ```
4. 启动采集：
   ```cpp
   if (!combo.init()) return -1;
   if (!combo.start()) return -1;
   ```
5. 按需录制：
   ```cpp
   combo.startRecording(cfg);   // 开始写入 RGB MP4 + CSV + DVS 原始数据
   // ... 主循环 ...
   combo.stopRecording();
   ```
6. 清理退出：
   ```cpp
   combo.stop();
   // combo.destroy(); // 需要时可显式释放
   ```

## 录制配置要点
- `output_dir`：若目录不存在会自动创建；确保可写。
- `fps`：用于 MP4 写出；建议设为接近 RGB 实际帧率。
- `fourcc`：编码标签；使用你的 OpenCV 构建支持的值（如 `mp4v`、`avc1`）。
- 文件名固定：`combo_rgb.mp4`、`combo_timestamps.csv`、`combo_events.raw` 位于 `output_dir` 下。

## 提示与排查
- 录制未开始：检查 `output_dir` 路径及 OpenCV 对应编码支持。
- 无效或不支持的 RGB 帧会被跳过并警告；确保 RGB 回调返回有效图像。
- 较大的缓冲区（`Combo` 构造参数中的 `max_buffer_size`）可在录制时吸收处理抖动。
- 录制 DVS 原始数据无需额外操作；绑定 recorder 后 `startRecording` 会自动开启。

## 下一步
- 将示例作为模板，根据数据命名与存储需求调整 `recorder_cfg`。
- 如需后处理，可扩展 CSV 字段以记录更多元数据。
- 针对慢动作或高码率场景，考虑提高 `fps` 或更换 `fourcc`。

*/
