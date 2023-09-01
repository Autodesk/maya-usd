
#include "interfaceImp.h"

PXR_NAMESPACE_OPEN_SCOPE

MayaHydraLibInterface& GetMayaHydraLibInterface()
{
    static MayaHydraLibInterfaceImp libInterface;
    return libInterface;
}

void MayaHydraLibInterfaceImp::RegisterTerminalSceneIndex(HdSceneIndexBasePtr sceneIndex)
{
    auto foundSceneIndex = std::find(_sceneIndices.begin(), _sceneIndices.end(), sceneIndex);
    if (foundSceneIndex == _sceneIndices.end()) {
        _sceneIndices.push_back(sceneIndex);
    }
}

void MayaHydraLibInterfaceImp::UnregisterTerminalSceneIndex(HdSceneIndexBasePtr sceneIndex)
{
    auto foundSceneIndex = std::find(_sceneIndices.begin(), _sceneIndices.end(), sceneIndex);
    if (foundSceneIndex != _sceneIndices.end()) {
        _sceneIndices.erase(foundSceneIndex);
    }
}

void MayaHydraLibInterfaceImp::ClearTerminalSceneIndices() { _sceneIndices.clear(); }

const SceneIndicesVector& MayaHydraLibInterfaceImp::GetTerminalSceneIndices() const
{
    return _sceneIndices;
}

PXR_NAMESPACE_CLOSE_SCOPE
