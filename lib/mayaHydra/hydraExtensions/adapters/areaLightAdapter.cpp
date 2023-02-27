//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include <mayaHydraLib/adapters/adapterDebugCodes.h>
#include <mayaHydraLib/adapters/adapterRegistry.h>
#include <mayaHydraLib/adapters/lightAdapter.h>

#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

/**
 * \brief MayaHydraAreaLightAdapter is used to handle the translation from a Maya area light to hydra.
 */
class MayaHydraAreaLightAdapter : public MayaHydraLightAdapter
{
public:
    MayaHydraAreaLightAdapter(MayaHydraDelegateCtx* delegate, const MDagPath& dag)
        : MayaHydraLightAdapter(delegate, dag)
    {
    }

    void _CalculateLightParams(GlfSimpleLight& light) override { light.SetSpotCutoff(90.0f); }

    const TfToken& LightType() const override
    {
        if (GetDelegate()->IsHdSt()) {
            return HdPrimTypeTokens->simpleLight;
        } else {
            return HdPrimTypeTokens->rectLight;
        }
    }

    VtValue GetLightParamValue(const TfToken& paramName) override
    {
        TF_DEBUG(MAYAHYDRALIB_ADAPTER_GET_LIGHT_PARAM_VALUE)
            .Msg(
                "Called MayaHydraAreaLightAdapter::GetLightParamValue(%s) - %s\n",
                paramName.GetText(),
                GetDagPath().partialPathName().asChar());

        if (paramName == HdLightTokens->width) {
            return VtValue(2.0f);
        } else if (paramName == HdLightTokens->height) {
            return VtValue(2.0f);
        }
        return MayaHydraLightAdapter::GetLightParamValue(paramName);
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<MayaHydraAreaLightAdapter, TfType::Bases<MayaHydraLightAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(MayaHydraAdapterRegistry, pointLight)
{
    MayaHydraAdapterRegistry::RegisterLightAdapter(
        TfToken("areaLight"),
        [](MayaHydraDelegateCtx* delegate, const MDagPath& dag) -> MayaHydraLightAdapterPtr {
            return MayaHydraLightAdapterPtr(new MayaHydraAreaLightAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
