#ifndef __HDMAYA_DELEGATE_H__
#define __HDMAYA_DELEGATE_H__

#include <pxr/pxr.h>

#include <pxr/usd/sdf/path.h>

#include <maya/MDagPath.h>
#include <maya/MSelectionList.h>

#include <memory>

#include <hdmaya/api.h>
#include <hdmaya/delegates/params.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegate {
public:
    HDMAYA_API
    HdMayaDelegate() = default;
    HDMAYA_API
    virtual ~HdMayaDelegate() = default;

    virtual void Populate() = 0;
    virtual void PreFrame() { }
    virtual void PostFrame() { }

    HDMAYA_API
    virtual void SetParams(const HdMayaParams& params);
    const HdMayaParams& GetParams() { return _params; }

    void SetPreferSimpleLight(bool v) { _preferSimpleLight = v; }
    bool GetPreferSimpleLight() { return _preferSimpleLight; }

    virtual void PopulateSelectedPaths(const MSelectionList& mayaSelection,
            SdfPathVector& selectedSdfPaths)
    { }
private:
    HdMayaParams _params;
    bool _preferSimpleLight = false;
};

using HdMayaDelegatePtr = std::shared_ptr<HdMayaDelegate>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_H__
