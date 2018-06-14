#include "adapter.h"

PXR_NAMESPACE_OPEN_SCOPE

HdMayaAdapter::HdMayaAdapter(const SdfPath& id, HdSceneDelegate* delegate) :
    _id(id), _delegate(delegate) { }

PXR_NAMESPACE_CLOSE_SCOPE
