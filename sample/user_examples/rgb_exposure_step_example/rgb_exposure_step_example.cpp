#include "camera/rgb_camera.h"
#include <iostream>
#include <iomanip>
#include <optional>

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

    auto cameras = evrgb::enumerateAllRgbCameras();
    if (cameras.empty()) {
        std::cerr << "No RGB camera found." << std::endl;
        return 1;
    }

    const std::string serial = cameras[0].serial_number;
    std::cout << "Opening RGB camera: " << serial << std::endl;

    evrgb::HikvisionRgbCamera camera;
    if (!camera.initialize(serial)) {
        std::cerr << "Initialize failed." << std::endl;
        return 1;
    }

    // Disable auto exposure so manual writes succeed on most Hikrobot models
    auto st_auto = camera.setEnumByName("ExposureAuto", "Off");
    printStatus("Set ExposureAuto=Off", st_auto);

    // Some devices require ExposureMode to be Timed for manual control
    auto st_mode = camera.setEnumByName("ExposureMode", "Timed");
    printStatus("Set ExposureMode=Timed", st_mode);

    // Query current exposure
    evrgb::FloatProperty exposure{};
    auto st = camera.getFloat("ExposureTime", exposure);
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
    st = camera.setFloat("ExposureTime", target_exposure_us);
    if (!st.success()) {
        // Fallback: some cameras expose ExposureTime as integer
        st = camera.setInt("ExposureTime", static_cast<int64_t>(target_exposure_us));
    }
    printStatus("Set ExposureTime", st);

    // Re-read to show actual applied value
    st = camera.getFloat("ExposureTime", exposure);
    if (st.success()) {
        std::cout << "Applied exposure: " << exposure.value << " us" << std::endl;
    }

    if (!camera.start()) {
        std::cerr << "Start failed." << std::endl;
        return 1;
    }

    std::cout << "Streaming... press +/- to adjust exposure step, q or ESC to quit." << std::endl;

    cv::namedWindow("RGB Exposure", cv::WINDOW_AUTOSIZE);

    double step = (exposure.inc > 0.0) ? exposure.inc : 500.0; // fallback 0.5 ms

    while (true) {
        cv::Mat frame;
        if (camera.getLatestImage(frame)) {
            cv::imshow("RGB Exposure", frame);
        }

        int key = cv::waitKey(1);
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }

        if (key == '+' || key == '=') {
            target_exposure_us = std::min(exposure.max, exposure.value + step);
            st = camera.setFloat("ExposureTime", target_exposure_us);
            if (!st.success()) {
                st = camera.setInt("ExposureTime", static_cast<int64_t>(target_exposure_us));
            }
            printStatus("Set ExposureTime (+)", st);
            camera.getFloat("ExposureTime", exposure);
            std::cout << "Exposure now: " << exposure.value << " us" << std::endl;
        }

        if (key == '-' || key == '_') {
            target_exposure_us = std::max(exposure.min, exposure.value - step);
            st = camera.setFloat("ExposureTime", target_exposure_us);
            if (!st.success()) {
                st = camera.setInt("ExposureTime", static_cast<int64_t>(target_exposure_us));
            }
            camera.getFloat("ExposureTime", exposure);
            std::cout << "Exposure now: " << exposure.value << " us" << std::endl;
        }
    }

    camera.stop();
    camera.destroy();
    cv::destroyAllWindows();
    return 0;
}
