
#include "utilFileSystem.h"
#include "../base/debugCodes.h"

#include <maya/MFileIO.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MFnReference.h>

#include "pxr/usd/ar/resolver.h"

PXR_NAMESPACE_USING_DIRECTIVE

std::string
UsdMayaUtilFileSystem::resolvePath(const std::string& filePath)
{
    ArResolver& resolver = ArGetResolver();
    return resolver.Resolve(filePath);
}

std::string
UsdMayaUtilFileSystem::getDir(const std::string &fullFilePath)
{
    return boost::filesystem::path(fullFilePath).parent_path().string();
}

std::string
UsdMayaUtilFileSystem::getMayaReferencedFileDir(const MObject &proxyShapeNode)
{
    // Can not use MFnDependencyNode(proxyShapeNode).isFromReferencedFile() to test if it is reference node or not,
    // which always return false even the proxyShape node is referenced...

    MStatus stat;
    MFnReference refFn;
    MItDependencyNodes dgIter(MFn::kReference, &stat);
    for (; !dgIter.isDone(); dgIter.next())
    {
        MObject cRefNode = dgIter.thisNode();
        refFn.setObject(cRefNode);
        if(refFn.containsNodeExactly(proxyShapeNode, &stat))
        {
            // According to Maya API document, the second argument is 'includePath' and set it to true to include the file path.
            // However, I have to set it to false to return the full file path otherwise I get a file name only...
            MString refFilePath = refFn.fileName(true, false, false, &stat);
            if(!refFilePath.length())
            return std::string();

            std::string referencedFilePath = refFilePath.asChar();
            TF_DEBUG(USDMAYA_PROXYSHAPEBASE).Msg("getMayaReferencedFileDir: The reference file that contains the proxyShape node is : %s\n", referencedFilePath.c_str());

            return getDir(referencedFilePath);
        }
    }

    return std::string();
}

std::string
UsdMayaUtilFileSystem::getMayaSceneFileDir()
{
    std::string currentFile = std::string(MFileIO::currentFile().asChar(), MFileIO::currentFile().length());
    size_t filePathSize = currentFile.size();
    if(filePathSize < 4)
        return std::string();

    // If scene is untitled, the maya file will be MayaWorkspaceDir/untitled :
    constexpr char ma_ext[] = ".ma";
    constexpr char mb_ext[] = ".mb";
    auto ext_start = currentFile.end() - 3;
    if(std::equal(ma_ext, ma_ext + 3, ext_start) || std::equal(mb_ext, mb_ext + 3, ext_start))
        return getDir(currentFile);

    return std::string();
}

std::string
UsdMayaUtilFileSystem::resolveRelativePathWithinMayaContext(const MObject &proxyShape, const std::string& relativeFilePath)
{
    if (relativeFilePath.length() < 3)
        return relativeFilePath;

    std::string currentFileDir = getMayaReferencedFileDir(proxyShape);

    if(currentFileDir.empty())
        currentFileDir = getMayaSceneFileDir();

    if(currentFileDir.empty())
        return relativeFilePath;

    boost::system::error_code errorCode;
    auto path = boost::filesystem::canonical(relativeFilePath, currentFileDir, errorCode);
    if (errorCode)
    {
        // file does not exist
        return std::string();
    }

    return path.string();
}

