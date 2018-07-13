#ifndef __HDMAYA_AL_PROXY_DELEGATE_H__
#define __HDMAYA_AL_PROXY_DELEGATE_H__

#include <pxr/pxr.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <AL/usdmaya/nodes/ProxyShape.h>

#include <hdmaya/delegates/delegate.h>

#include <maya/MMessage.h>

#include <memory>
#include <unordered_map>


using AL::usdmaya::nodes::ProxyShape;

PXR_NAMESPACE_OPEN_SCOPE

struct HdMayaALProxyData {
	std::vector<AL::event::CallbackId> proxyShapeCallbacks;
	std::unique_ptr<UsdImagingDelegate> delegate;
	bool populated = false;
};

class HdMayaALProxyDelegate : public HdMayaDelegate {
public:
    HdMayaALProxyDelegate(
        HdRenderIndex* renderIndex,
        const SdfPath& delegateID);

    ~HdMayaALProxyDelegate() override;

    void Populate() override;
    void PreFrame() override;

    HdMayaALProxyData& addProxy(ProxyShape* proxy);
    void removeProxy(ProxyShape* proxy);
    void setUsdImagingDelegate(ProxyShape* proxy);

private:
    bool _populateSingleProxy(ProxyShape* proxy,
    		HdMayaALProxyData& proxyData);
    void _setUsdImagingDelegate(ProxyShape* proxy,
    		HdMayaALProxyData& proxyData);

    std::unordered_map<ProxyShape*, HdMayaALProxyData> _proxiesData;
    SdfPath const _delegateID;
    HdRenderIndex* _renderIndex;
    MCallbackId _nodeAddedCBId;
};

PXR_NAMESPACE_CLOSE_SCOPE


#endif // __HDMAYA_AL_PROXY_DELEGATE_H__
