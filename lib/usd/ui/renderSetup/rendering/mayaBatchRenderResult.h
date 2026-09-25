//
// Copyright 2026 Autodesk
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

#ifndef MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDERRESULT_H
#define MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDERRESULT_H

#include <AdskUsdRenderSetup/IRenderResult.h>

namespace MayaUsdRenderSetup {

//! MayaUSD concrete class implementation of AdskUsdRenderSetup::IRenderResult
//! for Maya batch render.
class MayaBatchRenderResult : public AdskUsdRenderSetup::IRenderResult
{
public:
    ~MayaBatchRenderResult() = default;

    bool isAsync() const override { return true; }

    //! Placeholder.
    bool isDone() const override;

    //! Placeholder.
    float progress() const override;

    std::string errorMsg() const override { return _errorMsg; }
    void        setErrorMsg(const std::string& errorMsg) { _errorMsg = errorMsg; }

    //! Placeholder.
    explicit operator bool() const override;

private:
    std::string _errorMsg {};
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDERRESULT_H
