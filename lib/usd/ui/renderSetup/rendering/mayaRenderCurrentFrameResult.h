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

#ifndef MAYAUSDUI_USD_RENDERSETUP_MAYARENDERCURRENTFRAMERESULT_H
#define MAYAUSDUI_USD_RENDERSETUP_MAYARENDERCURRENTFRAMERESULT_H

#include <AdskUsdRenderSetup/IRenderResult.h>

namespace MayaUsdRenderSetup {

//! MayaUSD concrete class implementation of AdskUsdRenderSetup::IRenderResult
//! for Maya batch render.
class MayaRenderCurrentFrameResult : public AdskUsdRenderSetup::IRenderResult
{
public:
    // Render current frame synchronously renders a single frame, so
    // progress and success is simplistic.  We assume success means
    // progress is 1, failure means progress is 0.  An empty error
    // message means success.
    MayaRenderCurrentFrameResult(const std::string& errorMsg = {});

    ~MayaRenderCurrentFrameResult() = default;

    bool isAsync() const override { return false; }

    bool isDone() const override { return true; }

    float progress() const override { return _errorMsg.empty() ? 1.0f : 0.0f; }

    std::string errorMsg() const override { return _errorMsg; }

    explicit operator bool() const override { return _errorMsg.empty(); }

private:
    const std::string _errorMsg;
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_MAYARENDERCURRENTFRAMERESULT_H
