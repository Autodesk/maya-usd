//
// Copyright 2017 Animal Logic
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
#include "test_usdmaya.h"

#include "AL/maya/test/testHelpers.h"

#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>

using AL::maya::test::buildTempPath;

AL::usdmaya::nodes::ProxyShape* CreateMayaProxyShape(
    std::function<UsdStageRefPtr()> buildUsdStage,
    const std::string&              tempPath,
    MObject*                        shapeParent)
{
    if (buildUsdStage != nullptr) {
        UsdStageRefPtr stage = buildUsdStage();
        stage->Export(tempPath, false);
    }

    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);

    if (shapeParent)
        *shapeParent = shape;

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
    proxy->filePathPlug().setString(tempPath.c_str());
    return proxy;
}

AL::usdmaya::nodes::ProxyShape* CreateMayaProxyShape(const std::string& rootLayerPath)
{
    MFnDagNode                      fn;
    MObject                         xform = fn.create("transform");
    MObject                         shape = fn.create("AL_usdmaya_ProxyShape", xform);
    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
    proxy->filePathPlug().setString(rootLayerPath.c_str());
    return proxy;
}

AL::usdmaya::nodes::ProxyShape* SetupProxyShapeWithMesh()
{
    MFileIO::newFile(true);
    MGlobal::executeCommand("polySphere");
    const MString scene = buildTempPath("AL_USDMayaTests_SceneWithMesh.usda");
    MString       command;
    command.format(
        "file -force -typ \"AL usdmaya export\" -options \"Merge_Transforms=0;Meshes=1;\" -pr -ea "
        "\"^1s\"",
        scene.asChar());

    MGlobal::executeCommand(command, true);

    // clear scene then create ProxyShape
    MFileIO::newFile(true);
    AL::usdmaya::nodes::ProxyShape* proxyShape = CreateMayaProxyShape(scene.asChar());
    return proxyShape;
}

AL::usdmaya::nodes::ProxyShape* SetupProxyShapeWithMergedMesh()
{
    MFileIO::newFile(true);
    MGlobal::executeCommand("polySphere");
    const MString scene = buildTempPath("AL_USDMayaTests_SceneWithMergedMesh.usda");
    MString       command;
    command.format(
        "file -force -typ \"AL usdmaya export\" -options \"Merge_Transforms=1;Meshes=1;\" -pr -ea "
        "\"^1s\"",
        scene.asChar());

    MGlobal::executeCommand(command, true);

    // clear scene then create ProxyShape
    MFileIO::newFile(true);
    AL::usdmaya::nodes::ProxyShape* proxyShape = CreateMayaProxyShape(scene.asChar());
    return proxyShape;
}

AL::usdmaya::nodes::ProxyShape* SetupProxyShapeWithMultipleMeshes()
{
    MFileIO::newFile(true);
    MGlobal::executeCommand("polySphere"); // pSphere1
    MGlobal::executeCommand("polySphere"); // pSphere2
    MGlobal::executeCommand("polySphere"); // pSphere3
    const MString scene = buildTempPath("AL_USDMayaTests_SceneWithMultipleMeshs.usda");
    MString       command;
    command.format("file -force -typ \"AL usdmaya export\" -pr -ea \"^1s\"", scene.asChar());

    MGlobal::executeCommand(command, true);

    // clear scene then create ProxyShape
    MFileIO::newFile(true);
    AL::usdmaya::nodes::ProxyShape* proxyShape = CreateMayaProxyShape(scene.asChar());
    return proxyShape;
}
