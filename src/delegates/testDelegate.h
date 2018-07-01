#ifndef __HDMAYA_TEST_DELEGATE_H__
#define __HDMAYA_TEST_DELEGATE_H__

#include <pxr/pxr.h>

#include <pxr/imaging/hd/renderIndex.h>

#include <pxr/usd/sdf/path.h>

#include <pxr/usdImaging/usdImaging/delegate.h>
#include <pxr/usd/usd/stage.h>

#include <memory>

#include "delegate.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaTestDelegate : public HdMayaDelegate {
public:
    HdMayaTestDelegate(
        HdRenderIndex* renderIndex,
        const SdfPath& delegateID);

    void Populate() override;

private:
    std::unique_ptr<UsdImagingDelegate> _delegate;
    UsdStageRefPtr _stage;
};

PXR_NAMESPACE_CLOSE_SCOPE


#endif // __HDMAYA_TEST_DELEGATE_H__
