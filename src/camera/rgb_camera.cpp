// Copyright (c) 2025.
// Author: EvRGB Team
// Description: Implementation of the RGB camera class for device management and control.

#include "camera/rgb_camera.h"
#include "utils/evrgb_logger.h"
#include "MvCameraControl.h"
#include <cstring>
#include <memory>
#include <sstream>

namespace evrgb
{

namespace {

// Register Hikvision factories on translation unit load so factory-based creation works without central registration.
std::vector<RgbCameraInfo> enumerateHikvisionCameras()
{
    std::vector<RgbCameraInfo> cameraList;
    MV_CC_DEVICE_INFO_LIST deviceList;
    memset(&deviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &deviceList);
    if (MV_OK != nRet) {
        LOG_ERROR("Enumerate devices failed! Error code: 0x%08X", nRet);
        return cameraList;
    }

    if (deviceList.nDeviceNum == 0) {
        LOG_INFO("No camera devices found");
        return cameraList;
    }

    for (unsigned int i = 0; i < deviceList.nDeviceNum; i++) {
        MV_CC_DEVICE_INFO* deviceInfo = deviceList.pDeviceInfo[i];
        if (!deviceInfo) continue;

        RgbCameraInfo cameraInfo;

        if (deviceInfo->nTLayerType == MV_GIGE_DEVICE) {
            MV_GIGE_DEVICE_INFO* gigeInfo = &deviceInfo->SpecialInfo.stGigEInfo;
            strncpy(cameraInfo.manufacturer, (char*)gigeInfo->chManufacturerName, sizeof(cameraInfo.manufacturer) - 1);
            strncpy(cameraInfo.serial_number, (char*)gigeInfo->chSerialNumber, sizeof(cameraInfo.serial_number) - 1);
        } else if (deviceInfo->nTLayerType == MV_USB_DEVICE) {
            MV_USB3_DEVICE_INFO* usbInfo = &deviceInfo->SpecialInfo.stUsb3VInfo;
            strncpy(cameraInfo.manufacturer, (char*)usbInfo->chManufacturerName, sizeof(cameraInfo.manufacturer) - 1);
            strncpy(cameraInfo.serial_number, (char*)usbInfo->chSerialNumber, sizeof(cameraInfo.serial_number) - 1);
        }

        void* cameraHandle = nullptr;
        nRet = MV_CC_CreateHandle(&cameraHandle, deviceInfo);
        if (MV_OK == nRet) {
            nRet = MV_CC_OpenDevice(cameraHandle);
            if (MV_OK == nRet) {
                MVCC_INTVALUE widthValue = {0};
                nRet = MV_CC_GetIntValue(cameraHandle, "Width", &widthValue);
                if (MV_OK == nRet) cameraInfo.width = widthValue.nCurValue;

                MVCC_INTVALUE heightValue = {0};
                nRet = MV_CC_GetIntValue(cameraHandle, "Height", &heightValue);
                if (MV_OK == nRet) cameraInfo.height = heightValue.nCurValue;

                MV_CC_CloseDevice(cameraHandle);
            }
            MV_CC_DestroyHandle(cameraHandle);
        }

        cameraList.push_back(cameraInfo);
        LOG_DEBUG("Camera %zu: %s (%s)", cameraList.size(), cameraInfo.manufacturer, cameraInfo.serial_number);
    }

    LOG_INFO("Found %zu camera(s)", cameraList.size());
    return cameraList;
}

static bool registerHikvisionVendors()
{
    registerRgbCameraFactory("hikvision", [] { return std::make_shared<HikvisionRgbCamera>(); });
    registerRgbCameraFactory("hikrobot", [] { return std::make_shared<HikvisionRgbCamera>(); });
    registerRgbCameraFactory("hik", [] { return std::make_shared<HikvisionRgbCamera>(); });
    registerRgbEnumerator(enumerateHikvisionCameras);
    return true;
}

static const bool kHikVendorsRegistered = registerHikvisionVendors();

CameraStatus statusFrom(const char* action, uint32_t code)
{
    if (code == static_cast<uint32_t>(MV_OK)) {
        return {0, "OK"};
    }

    std::ostringstream oss;
    oss << action << " failed, code=0x" << std::uppercase << std::hex << code;
    return {static_cast<int32_t>(code), oss.str()};
}

CameraStatus nullHandleStatus(const char* action)
{
    return {static_cast<int32_t>(MV_E_HANDLE), std::string(action) + " failed: camera handle is null"};
}

} // namespace

HikvisionRgbCamera::HikvisionRgbCamera()
    : camera_handle_(nullptr)
    , camera_state_(CameraState::UNINITIALIZED)
{
    LOG_DEBUG("HikvisionRgbCamera default constructor");
}

HikvisionRgbCamera::HikvisionRgbCamera(const std::string& serial_number)
    : camera_handle_(nullptr)
    , camera_state_(CameraState::UNINITIALIZED)
    , serial_number_(serial_number)
{
    LOG_DEBUG("HikvisionRgbCamera constructor with serial number: %s", serial_number.c_str());
    if (!initialize(serial_number))
    {
        LOG_ERROR("Failed to initialize camera with serial number: %s", serial_number.c_str());
    }
}

HikvisionRgbCamera::~HikvisionRgbCamera()
{
    LOG_DEBUG("HikvisionRgbCamera destructor");
    destroy();
}

bool HikvisionRgbCamera::initialize(const std::string& serial_number)
{
    if (camera_state_ != CameraState::UNINITIALIZED)
    {
        LOG_DEBUG("Camera already initialized, destroying first");
        destroy();
    }

    std::string target_serial = serial_number.empty() ? serial_number_ : serial_number;
    serial_number_ = target_serial;

    if (target_serial.empty())
    {
        auto cameras = enumerateAllRgbCameras();
        if (cameras.empty())
        {
            setState(CameraState::ERROR_STATUS);
            return false;
        }
        target_serial = cameras[0].serial_number;
        serial_number_ = target_serial;
        LOG_INFO("No serial number specified, using first available camera: %s", target_serial.c_str());
    }

    if (!findCameraBySerial(target_serial))
    {
        LOG_ERROR("Camera with serial number %s not found", target_serial.c_str());
        setState(CameraState::ERROR_STATUS);
        return false;
    }

    MV_CC_DEVICE_INFO_LIST deviceList;
    memset(&deviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &deviceList);
    if (MV_OK != nRet) {
        LOG_ERROR("Enumerate devices failed! Error code: 0x%08X", nRet);
        setState(CameraState::ERROR_STATUS);
        return false;
    }

    MV_CC_DEVICE_INFO* targetDevice = nullptr;
    for (unsigned int i = 0; i < deviceList.nDeviceNum; i++) {
        MV_CC_DEVICE_INFO* deviceInfo = deviceList.pDeviceInfo[i];
        if (deviceInfo == nullptr) continue;

        std::string device_serial;
        if (deviceInfo->nTLayerType == MV_GIGE_DEVICE) {
            device_serial = (char*)deviceInfo->SpecialInfo.stGigEInfo.chSerialNumber;
        } else if (deviceInfo->nTLayerType == MV_USB_DEVICE) {
            device_serial = (char*)deviceInfo->SpecialInfo.stUsb3VInfo.chSerialNumber;
        }

        if (device_serial == target_serial) {
            targetDevice = deviceInfo;
            break;
        }
    }

    if (targetDevice == nullptr) {
        LOG_ERROR("Target device with serial %s not found in enumeration", target_serial.c_str());
        setState(CameraState::ERROR_STATUS);
        return false;
    }

    nRet = MV_CC_CreateHandle(&camera_handle_, targetDevice);
    if (MV_OK != nRet) {
        LOG_ERROR("Create handle failed, error code: 0x%08X", nRet);
        setState(CameraState::ERROR_STATUS);
        return false;
    }

    nRet = MV_CC_OpenDevice(camera_handle_);
    if (MV_OK != nRet) {
        LOG_ERROR("Open device failed, error code: 0x%08X", nRet);
        MV_CC_DestroyHandle(camera_handle_);
        camera_handle_ = nullptr;
        setState(CameraState::ERROR_STATUS);
        return false;
    }

    // Get Width and Height
    MVCC_INTVALUE widthValue = {0};
    if (MV_CC_GetIntValue(camera_handle_, "Width", &widthValue) == MV_OK) {
        width_ = widthValue.nCurValue;
    }
    MVCC_INTVALUE heightValue = {0};
    if (MV_CC_GetIntValue(camera_handle_, "Height", &heightValue) == MV_OK) {
        height_ = heightValue.nCurValue;
    }

    // Default settings
    nRet = MV_CC_SetEnumValue(camera_handle_, "TriggerMode", MV_TRIGGER_MODE_OFF);
    
    uint8_t lineSelector = 1; 
    nRet = MV_CC_SetEnumValue(camera_handle_, "LineSelector", lineSelector);
    
    if (lineSelector == 2){
         nRet = MV_CC_SetEnumValue(camera_handle_, "LineMode", 8);
    }
   
    nRet = MV_CC_SetBoolValue(camera_handle_, "LineInverter", true);
    nRet = MV_CC_SetEnumValueByString(camera_handle_, "LineSource", "ExposureStartActive");
    nRet = MV_CC_SetBoolValue(camera_handle_, "StrobeEnable", true);

    LOG_INFO("Camera initialized successfully with serial: %s", serial_number_.c_str());
    setState(CameraState::INITIALIZED);
    return true;
}

bool HikvisionRgbCamera::start()
{
    if (camera_state_ != CameraState::INITIALIZED && camera_state_ != CameraState::STOPPED) {
        LOG_ERROR("Camera must be initialized or stopped before starting");
        return false;
    }

    if (camera_handle_ == nullptr) {
        LOG_ERROR("Invalid camera handle");
        setState(CameraState::ERROR_STATUS);
        return false;
    }

    int nRet = MV_CC_StartGrabbing(camera_handle_);
    if (MV_OK != nRet) {
        LOG_ERROR("Start grabbing failed, error code: 0x%08X", nRet);
        setState(CameraState::ERROR_STATUS);
        return false;
    }

    setState(CameraState::STARTED);
    LOG_INFO("Camera started successfully");
    return true;
}

bool HikvisionRgbCamera::stop()
{
    if (camera_state_ != CameraState::STARTED) {
        LOG_ERROR("Camera is not started");
        return false;
    }

    if (camera_handle_ == nullptr) {
        LOG_ERROR("Invalid camera handle");
        setState(CameraState::ERROR_STATUS);
        return false;
    }

    int nRet = MV_CC_StopGrabbing(camera_handle_);
    if (MV_OK != nRet) {
        LOG_ERROR("Stop grabbing failed, error code: 0x%08X", nRet);
        setState(CameraState::ERROR_STATUS);
        return false;
    }

    setState(CameraState::STOPPED);
    LOG_INFO("Camera stopped successfully");
    return true;
}

void HikvisionRgbCamera::destroy()
{
    if (camera_state_ == CameraState::STARTED) {
        stop();
    }

    if (camera_handle_ != nullptr) {
        MV_CC_CloseDevice(camera_handle_);
        MV_CC_DestroyHandle(camera_handle_);
        camera_handle_ = nullptr;
    }

    setState(CameraState::UNINITIALIZED);
    LOG_DEBUG("Camera destroyed successfully");
}

bool HikvisionRgbCamera::getLatestImage(cv::Mat& output_image)
{
    if (camera_state_ != CameraState::STARTED) {
        LOG_ERROR("Camera is not started, cannot get frame");
        return false;
    }
    if (!camera_handle_) {
        LOG_ERROR("Invalid camera handle");
        setState(CameraState::ERROR_STATUS);
        return false;
    }

    MV_FRAME_OUT frameOut{};
    int nRet = MV_CC_GetImageBuffer(camera_handle_, &frameOut, 1000);
    if (MV_OK != nRet) {
        if (nRet != MV_E_NODATA) {
            LOG_ERROR("GetImageBuffer failed, code=0x%08X", nRet);
        }
        return false;
    }

    unsigned int width = frameOut.stFrameInfo.nWidth;
    unsigned int height = frameOut.stFrameInfo.nHeight;
    auto pixelType = frameOut.stFrameInfo.enPixelType;

    const uint8_t* srcPtr = static_cast<const uint8_t*>(frameOut.pBufAddr);

    bool success = false;
    if (pixelType == PixelType_Gvsp_BGR8_Packed) {
        output_image = cv::Mat(height, width, CV_8UC3, (void*)srcPtr).clone();
        success = true;
    } else if (pixelType == PixelType_Gvsp_RGB8_Packed) {
        cv::Mat rgb(height, width, CV_8UC3, (void*)srcPtr);
        cv::Mat bgr; cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
        output_image = bgr.clone();
        success = true;
    } else if (pixelType == PixelType_Gvsp_Mono8) {
        cv::Mat mono(height, width, CV_8UC1, (void*)srcPtr);
        cv::Mat bgr; cv::cvtColor(mono, bgr, cv::COLOR_GRAY2BGR);
        output_image = bgr.clone();
        success = true;
    } else {
        MV_CC_PIXEL_CONVERT_PARAM cvt{};
        cvt.nWidth = width;
        cvt.nHeight = height;
        cvt.pSrcData = const_cast<uint8_t*>(srcPtr);
        cvt.nSrcDataLen = frameOut.stFrameInfo.nFrameLen;
        cvt.enSrcPixelType = pixelType;
        cvt.enDstPixelType = PixelType_Gvsp_BGR8_Packed;
        std::vector<uint8_t> converted((size_t)width * height * 3);
        cvt.pDstBuffer = converted.data();
        cvt.nDstBufferSize = (unsigned)converted.size();
        int cvtRet = MV_CC_ConvertPixelType(camera_handle_, &cvt);
        if (MV_OK == cvtRet) {
            output_image = cv::Mat(height, width, CV_8UC3, converted.data()).clone();
            success = true;
        } else {
            LOG_ERROR("Pixel conversion failed, code=0x%08X", cvtRet);
            success = false;
        }
    }

    MV_CC_FreeImageBuffer(camera_handle_, &frameOut);
    return success;
}

unsigned int HikvisionRgbCamera::getWidth() const { return width_; }
unsigned int HikvisionRgbCamera::getHeight() const { return height_; }

CameraStatus HikvisionRgbCamera::getInt(const std::string& key, IntProperty& out)
{
    if (!camera_handle_) return nullHandleStatus("GetIntValueEx");

    MVCC_INTVALUE_EX val{};
    int ret = MV_CC_GetIntValueEx(camera_handle_, key.c_str(), &val);
    if (ret == MV_OK) {
        out.value = static_cast<int64_t>(val.nCurValue);
        out.min = static_cast<int64_t>(val.nMin);
        out.max = static_cast<int64_t>(val.nMax);
        out.inc = static_cast<int64_t>(val.nInc);
    }
    return statusFrom("GetIntValueEx", ret);
}

void HikvisionRgbCamera::setIntrinsics(const CameraIntrinsics& intrinsics)
{
    intrinsics_ = intrinsics;
}

std::optional<CameraIntrinsics> HikvisionRgbCamera::getIntrinsics() const
{
    return intrinsics_;
}

CameraStatus HikvisionRgbCamera::setInt(const std::string& key, int64_t value)
{
    if (!camera_handle_) return nullHandleStatus("SetIntValue");
    int ret = MV_CC_SetIntValue(camera_handle_, key.c_str(), value);
    return statusFrom("SetIntValue", ret);
}

CameraStatus HikvisionRgbCamera::getEnum(const std::string& key, EnumProperty& out)
{
    if (!camera_handle_) return nullHandleStatus("GetEnumValueEx");

    MVCC_ENUMVALUE_EX val{};
    int ret = MV_CC_GetEnumValueEx(camera_handle_, key.c_str(), &val);
    if (ret == MV_OK) {
        out.value = val.nCurValue;
        out.entries.clear();

        for (unsigned int i = 0; i < val.nSupportedNum; ++i) {
            MVCC_ENUMENTRY entry{};
            entry.nValue = val.nSupportValue[i];
            int eRet = MV_CC_GetEnumEntrySymbolic(camera_handle_, key.c_str(), &entry);
            if (eRet == MV_OK) {
                EnumEntry e;
                e.value = entry.nValue;
                e.name = entry.chSymbolic;
                e.available = true;
                out.entries.push_back(e);
            }
        }
    }
    return statusFrom("GetEnumValueEx", ret);
}

CameraStatus HikvisionRgbCamera::setEnum(const std::string& key, uint32_t value)
{
    if (!camera_handle_) return nullHandleStatus("SetEnumValue");
    int ret = MV_CC_SetEnumValue(camera_handle_, key.c_str(), value);
    return statusFrom("SetEnumValue", ret);
}

CameraStatus HikvisionRgbCamera::setEnumByName(const std::string& key, const std::string& name)
{
    if (!camera_handle_) return nullHandleStatus("SetEnumValueByString");
    int ret = MV_CC_SetEnumValueByString(camera_handle_, key.c_str(), name.c_str());
    return statusFrom("SetEnumValueByString", ret);
}

CameraStatus HikvisionRgbCamera::getFloat(const std::string& key, FloatProperty& out)
{
    if (!camera_handle_) return nullHandleStatus("GetFloatValue");

    MVCC_FLOATVALUE val{};
    int ret = MV_CC_GetFloatValue(camera_handle_, key.c_str(), &val);
    if (ret == MV_OK) {
        out.value = val.fCurValue;
        out.min = val.fMin;
        out.max = val.fMax;
        out.inc = 0.0;
    }
    return statusFrom("GetFloatValue", ret);
}

CameraStatus HikvisionRgbCamera::setFloat(const std::string& key, double value)
{
    if (!camera_handle_) return nullHandleStatus("SetFloatValue");
    int ret = MV_CC_SetFloatValue(camera_handle_, key.c_str(), static_cast<float>(value));
    return statusFrom("SetFloatValue", ret);
}

CameraStatus HikvisionRgbCamera::getBool(const std::string& key, bool& out)
{
    if (!camera_handle_) return nullHandleStatus("GetBoolValue");
    bool b = false;
    int ret = MV_CC_GetBoolValue(camera_handle_, key.c_str(), &b);
    if (ret == MV_OK) {
        out = b;
    }
    return statusFrom("GetBoolValue", ret);
}

CameraStatus HikvisionRgbCamera::setBool(const std::string& key, bool value)
{
    if (!camera_handle_) return nullHandleStatus("SetBoolValue");
    int ret = MV_CC_SetBoolValue(camera_handle_, key.c_str(), value);
    return statusFrom("SetBoolValue", ret);
}

CameraStatus HikvisionRgbCamera::getString(const std::string& key, StringProperty& out)
{
    if (!camera_handle_) return nullHandleStatus("GetStringValue");

    MVCC_STRINGVALUE val{};
    int ret = MV_CC_GetStringValue(camera_handle_, key.c_str(), &val);
    if (ret == MV_OK) {
        out.value = val.chCurValue;
        out.max_len = val.nMaxLength;
    }
    return statusFrom("GetStringValue", ret);
}

CameraStatus HikvisionRgbCamera::setString(const std::string& key, const std::string& value)
{
    if (!camera_handle_) return nullHandleStatus("SetStringValue");
    int ret = MV_CC_SetStringValue(camera_handle_, key.c_str(), value.c_str());
    return statusFrom("SetStringValue", ret);
}

CameraStatus HikvisionRgbCamera::getNodeAccessMode(const std::string& key, NodeAccessMode& mode)
{
    if (!camera_handle_) return nullHandleStatus("GetNodeAccessMode");
    MV_XML_AccessMode raw{};
    int ret = MV_XML_GetNodeAccessMode(camera_handle_, key.c_str(), &raw);
    if (ret == MV_OK) {
        mode = NodeAccessMode::Unknown; // Mapping can be added with SDK constants if needed
    }
    return statusFrom("GetNodeAccessMode", ret);
}

CameraStatus HikvisionRgbCamera::getNodeInterfaceType(const std::string& key, NodeInterfaceType& type)
{
    if (!camera_handle_) return nullHandleStatus("GetNodeInterfaceType");
    MV_XML_InterfaceType raw{};
    int ret = MV_XML_GetNodeInterfaceType(camera_handle_, key.c_str(), &raw);
    if (ret == MV_OK) {
        type = NodeInterfaceType::Unknown; // Mapping can be added with SDK constants if needed
    }
    return statusFrom("GetNodeInterfaceType", ret);
}

CameraStatus HikvisionRgbCamera::loadFeatureFile(const std::string& file_path)
{
    if (!camera_handle_) return nullHandleStatus("FeatureLoad");
    int ret = MV_CC_FeatureLoad(camera_handle_, file_path.c_str());
    return statusFrom("FeatureLoad", ret);
}

CameraStatus HikvisionRgbCamera::saveFeatureFile(const std::string& file_path)
{
    if (!camera_handle_) return nullHandleStatus("FeatureSave");
    int ret = MV_CC_FeatureSave(camera_handle_, file_path.c_str());
    return statusFrom("FeatureSave", ret);
}

CameraStatus HikvisionRgbCamera::getDeviceModelName (StringProperty& out)
{
    if (!camera_handle_) return nullHandleStatus("GetDeviceModelName");

    MVCC_STRINGVALUE val{};
    int ret = MV_CC_GetStringValue(camera_handle_, "DeviceModelName", &val);
    if (ret == MV_OK) {
        out.value = val.chCurValue;
        out.max_len = val.nMaxLength;
    }
    return statusFrom("GetDeviceModelName", ret);
}

void* HikvisionRgbCamera::getNativeHandle()
{
    return camera_handle_;
}

bool HikvisionRgbCamera::findCameraBySerial(const std::string& serialNumber)
{
    auto cameras = enumerateAllRgbCameras();
    for (const auto& camera : cameras) {
        if (std::string(camera.serial_number) == serialNumber) {
            LOG_DEBUG("Found camera with serial number: %s", serialNumber.c_str());
            return true;
        }
    }
    LOG_DEBUG("Camera with serial number %s not found", serialNumber.c_str());
    return false;
}

void HikvisionRgbCamera::setState(CameraState newState)
{
    if (camera_state_ != newState) {
        LOG_DEBUG("Camera state changed: %d -> %d", 
                  static_cast<int>(camera_state_), static_cast<int>(newState));
        camera_state_ = newState;
    }
}

} // namespace evrgb
