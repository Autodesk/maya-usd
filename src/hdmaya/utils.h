//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef __HDMAYA_UTILS_H__
#define __HDMAYA_UTILS_H__

#include <pxr/pxr.h>

#include <pxr/base/gf/matrix4d.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MMatrix.h>
#include <maya/MRenderUtil.h>

#include <hdmaya/adapters/mayaAttrs.h>

PXR_NAMESPACE_OPEN_SCOPE

inline GfMatrix4d GetGfMatrixFromMaya(const MMatrix& mayaMat) {
    GfMatrix4d mat;
    memcpy(mat.GetArray(), mayaMat[0], sizeof(double) * 16);
    return mat;
}

inline MString GetTextureFilePath(
    const MFnDependencyNode& fileNode, bool fullPath = false) {
    MString ret;
    if (fileNode.findPlug(MayaAttrs::file::uvTilingMode, true).asShort() != 0) {
        ret = fileNode.findPlug(MayaAttrs::file::fileTextureNamePattern, true)
                  .asString();
        if (ret.length() == 0) {
            ret = fileNode
                      .findPlug(
                          MayaAttrs::file::computedFileTextureNamePattern, true)
                      .asString();
        }
    } else {
        if (fullPath) {
            ret = MRenderUtil::exactFileTextureName(fileNode.object());
        }
        if (ret.length() == 0) {
            ret = fileNode.findPlug(MayaAttrs::file::fileTextureName, true)
                      .asString();
        }
    }
    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_UTILS_H_
