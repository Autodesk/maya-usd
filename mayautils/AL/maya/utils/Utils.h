#pragma once

#include <string>
#include <maya/MString.h>
#include <maya/MDagPath.h>
#include <maya/MObject.h>

namespace AL {
namespace maya {
namespace utils {

  /// \brief  convert string types
  /// \param  str the MString to convert to a std::string
  /// \return the std::string
  /// \ingroup usdmaya
  inline std::string convert(const MString& str)
  {
    return std::string(str.asChar(), str.length());
  }

  /// \brief  convert string types
  /// \param  str the std::string to convert to an MString
  /// \return the MString
  /// \ingroup usdmaya
  inline MString convert(const std::string& str)
  {
    return MString(str.data(), str.size());
  }

  /// \brief  returns the dag path for the specified maya object
  static MDagPath getDagPath(const MObject&);

  /// \brief  checks to see if the maya plugin is loaded. If it isn't, it will attempt to load the plugin.
  /// \param  pluginName the name of the plugin to check
  /// \return if the plugin is avalable and loaded, it will return true. If the plugin is not found, false is returned.
  static bool ensureMayaPluginIsLoaded(const MString& pluginName);

  /// \brief  return maya object with specified name
  static MObject findMayaObject(const MString& objectName);


//----------------------------------------------------------------------------------------------------------------------
} // utils
} // maya
} // AL
//----------------------------------------------------------------------------------------------------------------------
