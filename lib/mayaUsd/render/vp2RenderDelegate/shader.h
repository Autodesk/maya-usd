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
#ifndef HD_VP2_SHADER
#define HD_VP2_SHADER

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

#include <maya/MShaderManager.h>

#include <tbb/spin_rw_mutex.h>

#include <memory>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

struct HdVP2ShaderUniquePtr
{
    HdVP2ShaderUniquePtr();
    explicit HdVP2ShaderUniquePtr(MHWRender::MShaderInstance*);
    HdVP2ShaderUniquePtr(const HdVP2ShaderUniquePtr&);
    HdVP2ShaderUniquePtr(HdVP2ShaderUniquePtr&&);

    ~HdVP2ShaderUniquePtr();

    HdVP2ShaderUniquePtr& operator=(const HdVP2ShaderUniquePtr&);
    HdVP2ShaderUniquePtr& operator=(HdVP2ShaderUniquePtr&&);

    MHWRender::MShaderInstance* operator->() const;
    MHWRender::MShaderInstance* get() const;
    explicit                    operator bool() const;

    void reset(MHWRender::MShaderInstance* shader);
    void clear();

    static void cleanupDeadShaders();

private:
    struct Data
    {
        std::atomic<int>            _count { 0 };
        MHWRender::MShaderInstance* _shader { nullptr };
    };

    Data* _data { nullptr };
};

/*! \brief  Thread-safe cache of named shaders.
 */
struct HdVP2ShaderCache
{
    //! Shader registry
    std::unordered_map<TfToken, HdVP2ShaderUniquePtr, TfToken::HashFunctor> _map;

#ifdef WANT_MATERIALX_BUILD
    //! Primvars registry
    std::unordered_map<TfToken, TfTokenVector, TfToken::HashFunctor> _primvars;
#endif

    //! Synchronization used to protect concurrent read from serial writes
    tbb::spin_rw_mutex _mutex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_SHADER
