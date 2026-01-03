// Copyright (c) 2025.
// Author: tonywangziteng
// Description: Version information for EvRGB Combo SDK

#pragma once

#define EVRGB_VERSION_MAJOR 1
#define EVRGB_VERSION_MINOR 0
#define EVRGB_VERSION_PATCH 1

// Auto-generate version string from version numbers
#define EVRGB_VERSION_STRING_HELPER(major, minor, patch) #major "." #minor "." #patch
#define EVRGB_VERSION_STRING(major, minor, patch) EVRGB_VERSION_STRING_HELPER(major, minor, patch)
#define EVRGB_VERSION EVRGB_VERSION_STRING(EVRGB_VERSION_MAJOR, EVRGB_VERSION_MINOR, EVRGB_VERSION_PATCH)

namespace evrgb {

/// Version information structure
struct Version {
    /// Major version number
    static constexpr int major = EVRGB_VERSION_MAJOR;
    
    /// Minor version number
    static constexpr int minor = EVRGB_VERSION_MINOR;
    
    /// Patch version number
    static constexpr int patch = EVRGB_VERSION_PATCH;
    
    /// Complete version string
    static constexpr const char* string = EVRGB_VERSION;
    
    /// Check if version is compatible with given major.minor version
    static constexpr bool isCompatible(int reqMajor, int reqMinor) {
        return major == reqMajor && minor >= reqMinor;
    }
    
    /// Get version as integer in format MMmmpp (Major*10000 + Minor*100 + Patch)
    static constexpr int asInteger() {
        return major * 10000 + minor * 100 + patch;
    }
};

} // namespace evrgb