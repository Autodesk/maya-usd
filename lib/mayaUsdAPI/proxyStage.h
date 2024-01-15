//
// Copyright 2023 Autodesk
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
#ifndef MAYAUSDAPI_MAYAUSD_PROXY_STAGE_H
#define MAYAUSDAPI_MAYAUSD_PROXY_STAGE_H

#include <mayaUsdAPI/api.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MPxNode.h>

#include <memory>

namespace MAYAUSDAPI_NS_DEF {

//! Forward declaration of ProxyStageImp
struct ProxyStageImp;
//! Custom deleter for ProxyStageImp
struct ProxyStageImpDeleter
{
    void operator()(ProxyStageImp* ptr) const;
};

/*! \brief  Class to access Usd time and Usd stage from a maya MObject which comes from a mayaUsd
   ProxyStageProvider subclass (usually a MayaUsdProxyShapeBase node) 
   \class  ProxyStage
   This class is an accessor for a UsdTime and a UsdStageRefPtr from a MObject which comes from a
   mayaUsd ProxyStageProvider subclass. Usually a MayaUsdProxyShapeBase node. 
   Example usage: \code
    {
        MObject dagNode;//is the MObject of a MayaUsdProxyShape node for example
        MayaUsdAPI::ProxyStage proxyStage(dagNode);
        if (TF_VERIFY(proxyStage.isValid(), "Error getting MayaUsdAPIProxyStage")) {
            auto stage = proxyStage.getUsdStage();
            auto time = proxyStage.getTime();
        }
    }
    \endcode
*/
class MAYAUSD_API_PUBLIC ProxyStage
{
public:
    /*! Constructor from a MObject which is a the MObject of a subclass of mayaUsd
     * ProxyStageProvider, such as a MayaUsdProxyShapeBase node
     * @param[in] obj is a MObject from a subclass of mayaUsd ProxyStageProvider, such as a
     * MayaUsdProxyShapeBase node
     */
    ProxyStage(const MObject& obj);
    /*! Copy constructor
     * @param[in] other is another ProxyStage
     */
    ProxyStage(const ProxyStage& other);
    //! Destructor
    ~ProxyStage();
    /*! Returns true if the ProxyStage is valid, it can only be invalid if it was constructed from a
     * MpxNode which was not a subclass of mayaUsd ProxyStageProvider \return true if the ProxyStage
     * is valid
     */
    bool isValid() const;
    /*! Returns a UsdTimeCode
     *  \return a UsdTimeCode, if the ProxyStage is not valid, it returns a default constructed
     * UsdTimeCode()
     */
    PXR_NS::UsdTimeCode getTime() const;
    /*! Returns a UsdStageRefPtr
     *  \return a UsdStageRefPtr, if the ProxyStage is not valid, it returns nullptr
     */
    PXR_NS::UsdStageRefPtr getUsdStage() const;

private:
    //! Private constructor
    ProxyStage() = delete;

    //! Private implementation member
    std::unique_ptr<struct ProxyStageImp, ProxyStageImpDeleter> _imp {};
};

} // namespace MAYAUSDAPI_NS_DEF

#endif // MAYAUSDAPI_MAYAUSD_PROXY_STAGE_H
