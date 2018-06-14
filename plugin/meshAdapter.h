#ifndef __HDMAYA_MESH_ADAPTER_H__
#define __HDMAYA_MESH_ADAPTER_H__

#include <pxr/pxr.h>

#include "dagAdapter.h"
#include "adapterRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaMeshAdapter : public HdMayaDagAdapter {
public:
    explicit HdMayaMeshAdapter(const MDagPath& dag);

    void Populate(
        HdRenderIndex& renderIndex,
        HdSceneDelegate* delegate,
        const SdfPath& id) override;

    VtValue Get(const TfToken& key) override;
    HdMeshTopology GetMeshTopology() override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_MESH_ADAPTER_H__
