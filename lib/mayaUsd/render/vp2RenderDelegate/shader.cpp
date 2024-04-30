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

#include <maya/MGlobal.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
using DeadShaders = std::set<MHWRender::MShaderInstance*>;
std::mutex deadShaderMutex;

DeadShaders& getDeadShaders()
{
    static DeadShaders dead;
    return dead;
}

void addDeadShader(MHWRender::MShaderInstance* shader)
{
    if (!shader)
        return;

    std::lock_guard<std::mutex> mutexGuard(deadShaderMutex);
    getDeadShaders().insert(shader);
}

} // namespace

void HdVP2ShaderUniquePtr::cleanupDeadShaders()
{
    MRenderer* const renderer = MRenderer::theRenderer();
    if (!renderer)
        return;

    const MShaderManager* const shaderMgr = renderer->getShaderManager();
    if (!shaderMgr)
        return;

    std::lock_guard<std::mutex> mutexGuard(deadShaderMutex);

    for (MHWRender::MShaderInstance* shader : getDeadShaders()) {
        if (shader)
            shaderMgr->releaseShader(shader);
    }

    getDeadShaders().clear();
}

HdVP2ShaderUniquePtr::HdVP2ShaderUniquePtr() { }

HdVP2ShaderUniquePtr::HdVP2ShaderUniquePtr(MHWRender::MShaderInstance* shader) { reset(shader); }

HdVP2ShaderUniquePtr::HdVP2ShaderUniquePtr(const HdVP2ShaderUniquePtr& other)
{
    if (!other._data)
        return;

    _data = other._data;
    _data->_count.fetch_add(1);
}

HdVP2ShaderUniquePtr::HdVP2ShaderUniquePtr(HdVP2ShaderUniquePtr&& other)
{
    _data = other._data;
    other._data = nullptr;
}

HdVP2ShaderUniquePtr::~HdVP2ShaderUniquePtr() { clear(); }

HdVP2ShaderUniquePtr& HdVP2ShaderUniquePtr::operator=(const HdVP2ShaderUniquePtr& other)
{
    if (other._data == _data)
        return *this;

    if (_data && other._data && _data->_shader == other._data->_shader)
        return *this;

    clear();

    if (!other._data)
        return *this;

    _data = other._data;
    _data->_count.fetch_add(1);

    return *this;
}

HdVP2ShaderUniquePtr& HdVP2ShaderUniquePtr::operator=(HdVP2ShaderUniquePtr&& other)
{
    if (other._data == _data)
        return *this;

    _data = other._data;
    other._data = nullptr;

    return *this;
}

MHWRender::MShaderInstance* HdVP2ShaderUniquePtr::operator->() const
{
    return _data ? _data->_shader : nullptr;
}

MHWRender::MShaderInstance* HdVP2ShaderUniquePtr::get() const
{
    return _data ? _data->_shader : nullptr;
}

HdVP2ShaderUniquePtr::operator bool() const
{
    return _data != nullptr && _data->_shader != nullptr;
}

void HdVP2ShaderUniquePtr::reset(MHWRender::MShaderInstance* shader)
{
    if (_data && _data->_shader == shader)
        return;

    clear();

    if (!shader)
        return;

    _data = new Data;
    _data->_count.fetch_add(1);
    _data->_shader = shader;
}

void HdVP2ShaderUniquePtr::clear()
{
    if (!_data)
        return;

    const int prevCount = _data->_count.fetch_sub(1);
    if (prevCount != 1)
        return;

    MHWRender::MShaderInstance* shader = _data->_shader;
    addDeadShader(shader);

    _data->_shader = nullptr;
    delete _data;
    _data = nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
