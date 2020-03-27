#include "pxr/pxr.h"

#include "../base/api.h"

#include <maya/MObject.h>
#include <string>

#include <boost/filesystem.hpp>

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdMayaUtilFileSystem
{
    /*! \brief returns the resolved filesystem path for the file identified by the given path
    */
    MAYAUSD_CORE_PUBLIC
    std::string resolvePath(const std::string& filePath);

    /*! \brief returns the path to the
    */
    MAYAUSD_CORE_PUBLIC
    std::string getDir(const std::string &fullFilePath);

    /*! \brief returns parent directory of a maya scene file opened by reference
    */
    MAYAUSD_CORE_PUBLIC
    std::string getMayaReferencedFileDir(const MObject& proxyShapeNode);

    /*! \brief returns parent directory of opened maya scene file 
    */
    MAYAUSD_CORE_PUBLIC
    std::string getMayaSceneFileDir();

    /*! \brief returns the aboluste path relative to the maya file
    */
    MAYAUSD_CORE_PUBLIC
    std::string resolveRelativePathWithinMayaContext(const MObject& proxyShape, 
                                                     const std::string& relativeFilePath);

} // namespace UsdMayaUtilFileSystem

PXR_NAMESPACE_CLOSE_SCOPE
