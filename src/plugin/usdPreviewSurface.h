#ifndef __HDMAYA_USD_PREVIEW_SURFACE_H__
#define __HDMAYA_USD_PREVIEW_SURFACE_H__

#include <pxr/pxr.h>

#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaUsdPreviewSurface : public MPxNode {
public:
    static const MString classification;
    static const MString name;
    static const MTypeId typeId;
    static void* Creator() { return new HdMayaUsdPreviewSurface(); }
    static MStatus Initialize();

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_USD_PREVIEW_SURFACE_H__
