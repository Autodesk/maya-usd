#pragma once

#include "AL/maya/utils/Api.h"

#include <maya/MDagPath.h>
#include <maya/MString.h>
#include <maya/MUuid.h>

#include <string>

namespace AL {
namespace maya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
// code to speed up comparisons of MObject guids
//----------------------------------------------------------------------------------------------------------------------

/// \brief  A type to store a UUID from a maya node
/// \ingroup usdmaya
struct guid
{
    uint8_t uuid[16]; ///< the UUID for a Maya node
};
#if AL_UTILS_ENABLE_SIMD

/// \brief  Less than comparison utility for sorting via 128bit guid.
/// \ingroup usdmaya
struct guid_compare
{
    inline bool operator()(const i128 a, const i128 b) const
    {
        const uint32_t lt_mask = movemask16i8(cmplt16i8(a, b));
        const uint32_t eq_mask = 0xFFFF & (~movemask16i8(cmpeq16i8(a, b)));
        if (!eq_mask)
            return false;

        // find first bit value that is not equal
        const uint32_t index = __builtin_ctz(eq_mask);
        // now see whether that bit has been set
        return (lt_mask & (1 << index)) != 0;
    }
};

#else

/// \brief  Less than comparison utility for sorting via 128bit guid.
/// \ingroup usdmaya
struct guid_compare
{
    /// \brief  performs a less than comparison between two UUIDs. Used to sort the entries in an
    /// MObjectMap \param  a first UUID to compare \param  b second UUID to compare \return true if
    /// a < b
    inline bool operator()(const guid& a, const guid& b) const
    {
        for (int i = 0; i < 16; ++i) {
            if (a.uuid[i] < b.uuid[i])
                return true;
            if (a.uuid[i] > b.uuid[i])
                return false;
        }
        return false;
    }
};
#endif

/// \brief  convert string types
/// \param  str the MString to convert to a std::string
/// \return the std::string
/// \ingroup usdmaya
inline std::string convert(const MString& str) { return std::string(str.asChar(), str.length()); }

/// \brief  convert string types
/// \param  str the std::string to convert to an MString
/// \return the MString
/// \ingroup usdmaya
inline MString convert(const std::string& str) { return MString(str.data(), str.size()); }

/// \brief  returns the dag path for the specified maya object
AL_MAYA_UTILS_PUBLIC
MDagPath getDagPath(const MObject&);

/// \brief  checks to see if the maya plugin is loaded. If it isn't, it will attempt to load the
/// plugin. \param  pluginName the name of the plugin to check \return if the plugin is avalable and
/// loaded, it will return true. If the plugin is not found, false is returned.
AL_MAYA_UTILS_PUBLIC
bool ensureMayaPluginIsLoaded(const MString& pluginName);

/// \brief  return maya object with specified name
AL_MAYA_UTILS_PUBLIC
MObject findMayaObject(const MString& objectName);

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
