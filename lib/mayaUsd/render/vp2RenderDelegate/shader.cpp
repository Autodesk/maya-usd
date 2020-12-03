//
// Copyright 2020 Autodesk
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
#include "shader.h"

#include <pxr/base/tf/diagnostic.h>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  Releases the reference to the shader owned by a smart pointer.
 */
void HdVP2ShaderDeleter::operator()(MHWRender::MShaderInstance* shader)
{
    MRenderer* const renderer = MRenderer::theRenderer();

    const MShaderManager* const shaderMgr = renderer ? renderer->getShaderManager() : nullptr;
    if (TF_VERIFY(shaderMgr)) {
        shaderMgr->releaseShader(shader);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
