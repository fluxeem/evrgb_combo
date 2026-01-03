#include "core/combo.h"
#include <iostream>
#include <iomanip>
#include <mutex>

#ifdef _WIN32
    #include "opencv2/opencv.hpp"
#else
    #include "opencv4/opencv2/opencv.hpp"
#endif

namespace {

void printStatus(const char* action, const evrgb::CameraStatus& status) {
    if (status.success()) {
        std::cout << action << " -> OK" << std::endl;
    } else {
        std::cout << action << " -> " << status.message
                  << " (code=0x" << std::hex << std::uppercase << status.code << std::dec << ")" << std::endl;
    }
}

} // namespace

int main(int argc, char* argv[]) {
    double target_exposure_us = (argc > 1) ? std::stod(argv[1]) : 10000.0; // default 10 ms

    std::cout << "EvRGB Combo SDK Sample - RGB Exposure Step Example" << std::endl;
    std::cout << "Usage: rgb_exposure_step_example [exposure_us]" << std::endl;

    auto [rgb_cameras, dvs_cameras] = evrgb::enumerateAllCameras();
    if (rgb_cameras.empty()) {
        std::cerr << "No RGB camera found." << std::endl;
        return 1;
    }

    const std::string rgb_serial = rgb_cameras[0].serial_number;
    const std::string dvs_serial = dvs_cameras.empty() ? "" : dvs_cameras[0].serial;
    std::cout << "Opening Combo with RGB serial: " << rgb_serial << std::endl;
    if (!dvs_serial.empty()) {
        std::cout << "Using DVS serial (optional): " << dvs_serial << std::endl;
    } else {
        std::cout << "No DVS selected (RGB-only)." << std::endl;
    }

    evrgb::Combo combo(rgb_serial, dvs_serial);
    if (!combo.init()) {
        std::cerr << "Combo initialization failed." << std::endl;
        return 1;
    }

    auto rgb_camera = combo.getRgbCamera();
    if (!rgb_camera) {
        std::cerr << "RGB camera handle unavailable." << std::endl;
        return 1;
    }

    evrgb::CameraStatus ret_status;

    // Disable auto exposure so manual writes succeed on most Hikrobot models
    ret_status = rgb_camera->setEnumByName("ExposureAuto", "Off");
    printStatus("Set ExposureAuto=Off", ret_status);

    // Some devices require ExposureMode to be Timed for manual control
    ret_status = rgb_camera->setEnumByName("ExposureMode", "Timed");
    printStatus("Set ExposureMode=Timed", ret_status);

    ret_status = rgb_camera->setInt("AutoExposureTimeUpperLimit", static_cast<int64_t>(500000)); // 500 ms
    printStatus("Set AutoExposureTimeUpperLimit=500000", ret_status);

    // Query current exposure
    evrgb::FloatProperty exposure{};
    auto st = rgb_camera->getFloat("ExposureTime", exposure);
    printStatus("Get ExposureTime", st);
    if (st.success()) {
        std::cout << "Current exposure: " << exposure.value << " us"
                  << " (min=" << exposure.min << ", max=" << exposure.max
                  << ", inc=" << exposure.inc << ")" << std::endl;
    }

    // Apply requested exposure (clamp to camera limits when known)
    if (st.success()) {
        if (target_exposure_us < exposure.min) target_exposure_us = exposure.min;
        if (target_exposure_us > exposure.max) target_exposure_us = exposure.max;
    }
    st = rgb_camera->setFloat("ExposureTime", target_exposure_us);
    if (!st.success()) {
        // Fallback: some cameras expose ExposureTime as integer
        st = rgb_camera->setInt("ExposureTime", static_cast<int64_t>(target_exposure_us));
    }
    printStatus("Set ExposureTime", st);

    // Re-read to show actual applied value
    st = rgb_camera->getFloat("ExposureTime", exposure);
    if (st.success()) {
        std::cout << "Applied exposure: " << exposure.value << " us" << std::endl;
    }

    cv::namedWindow("RGB Exposure", cv::WINDOW_AUTOSIZE);

    // Pull frames from Combo via callback to demonstrate Combo usage
    cv::Mat latest_frame;
    std::mutex frame_mutex;
    combo.setRgbImageCallback([&](const cv::Mat& frame) {
        std::lock_guard<std::mutex> lock(frame_mutex);
        frame.copyTo(latest_frame);
    });

    if (!combo.start()) {
        std::cerr << "Combo start failed." << std::endl;
        return 1;
    }

    std::cout << "Streaming... press +/- to adjust exposure step, q or ESC to quit." << std::endl;

    double step = (exposure.inc > 0.0) ? exposure.inc : 500.0; // fallback 0.5 ms

    while (true) {
        cv::Mat frame;
        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            if (!latest_frame.empty()) {
                frame = latest_frame.clone();
            }
        }

        if (!frame.empty()) {
            cv::imshow("RGB Exposure", frame);
        }

        int key = cv::waitKey(1);
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }

        if (key == '+' || key == '=') {
            target_exposure_us = std::min(exposure.max, exposure.value + step);
            st = rgb_camera->setFloat("ExposureTime", target_exposure_us);
            if (!st.success()) {
                st = rgb_camera->setInt("ExposureTime", static_cast<int64_t>(target_exposure_us));
            }
            printStatus("Set ExposureTime (+)", st);
            rgb_camera->getFloat("ExposureTime", exposure);
            std::cout << "Exposure now: " << exposure.value << " us" << std::endl;
        }

        if (key == '-' || key == '_') {
            target_exposure_us = std::max(exposure.min, exposure.value - step);
            st = rgb_camera->setFloat("ExposureTime", target_exposure_us);
            if (!st.success()) {
                st = rgb_camera->setInt("ExposureTime", static_cast<int64_t>(target_exposure_us));
            }
            rgb_camera->getFloat("ExposureTime", exposure);
            std::cout << "Exposure now: " << exposure.value << " us" << std::endl;
        }
    }

    combo.stop();
    combo.destroy();
    cv::destroyAllWindows();
    return 0;
}
