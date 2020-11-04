//
// Copyright 2017 Pixar
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

/// \file glslProgram.h

#ifndef __PX_VP20_GLSL_PROGRAM_H__
#define __PX_VP20_GLSL_PROGRAM_H__

#include <mayaUsd/base/api.h>

#include <pxr/imaging/garch/gl.h>
#include <pxr/pxr.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \class PxrMayaGLSLProgram
///
/// A convenience class that abstracts away the OpenGL API details of
/// compiling and linking GLSL shaders into a program.
///
class PxrMayaGLSLProgram
{
public:
    MAYAUSD_CORE_PUBLIC
    PxrMayaGLSLProgram();
    MAYAUSD_CORE_PUBLIC
    virtual ~PxrMayaGLSLProgram();

    /// Compile a shader of type \p type with the given \p source.
    MAYAUSD_CORE_PUBLIC
    bool CompileShader(const GLenum type, const std::string& source);

    /// Link the compiled shaders together.
    MAYAUSD_CORE_PUBLIC
    bool Link();

    /// Validate whether this program is valid in the current context.
    MAYAUSD_CORE_PUBLIC
    bool Validate() const;

    /// Get the ID of the OpenGL program object.
    GLuint GetProgramId() const { return mProgramId; };

private:
    GLuint mProgramId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __PX_VP20_GLSL_PROGRAM_H__
