#pragma once

#include <string>
#include <maya/MString.h>

namespace AL {
namespace maya {
namespace utils {

  //----------------------------------------------------------------------------------------------------------------------
  /// \brief  convert string types
  /// \param  str the MString to convert to a std::string
  /// \return the std::string
  /// \ingroup usdmaya
  //----------------------------------------------------------------------------------------------------------------------
  inline std::string convert(const MString& str)
  {
    return std::string(str.asChar(), str.length());
  }

  //----------------------------------------------------------------------------------------------------------------------
  /// \brief  convert string types
  /// \param  str the std::string to convert to an MString
  /// \return the MString
  /// \ingroup usdmaya
  //----------------------------------------------------------------------------------------------------------------------
  inline MString convert(const std::string& str)
  {
    return MString(str.data(), str.size());
  }

//----------------------------------------------------------------------------------------------------------------------
} // utils
} // maya
} // AL
//----------------------------------------------------------------------------------------------------------------------
