#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

import os
ASSET_BASE = os.path.join(os.getcwd(), 'models')

def main():
    shotFilePath = 'shots/s00_01/s00_01.usd'
    animLayerFilePath = 'shots/s00_01/s00_01_anim.usd'

    from pxr import Usd, Sdf
    stage = Usd.Stage.Open(shotFilePath)

    # set the timeCode range for the shot that other applications can use.
    stage.SetStartTimeCode(1)
    stage.SetEndTimeCode(10)

    # we use Sdf, a lower level library, to obtain the 'layout' layer.
    workingLayer = Sdf.Layer.FindOrOpen(animLayerFilePath)
    assert stage.HasLocalLayer(workingLayer)

    # this makes the workingLayer the target for authoring operations by the
    # stage.
    stage.SetEditTarget(workingLayer)

    _AddCueRigAndCacheVariants(stage)

    stage.GetEditTarget().GetLayer().Save()

    print '==='
    print 'usdview %s' % shotFilePath
    print 'usdcat %s' % animLayerFilePath

def _AddCueRigAndCacheVariants(stage):
    from pxr import Kind, Sdf, Usd, UsdGeom
    from AL.usd import schemas

    # Make sure the model-parents we need are well-specified
    Usd.ModelAPI(UsdGeom.Xform.Define(stage, '/World')).SetKind(Kind.Tokens.group)
    animModel = Usd.ModelAPI(UsdGeom.Xform.Define(stage, '/World/anim'))
    animModel.SetKind(Kind.Tokens.group)
    
    animVariant = animModel.GetPrim().GetVariantSets().AddVariantSet('animVariant')
    animVariant.AddVariant('rig')
    animVariant.SetVariantSelection('rig')
    with animVariant.GetVariantEditContext():
        # Creating maya rig for CueBall
        cueBallRig = schemas.MayaReference.Define(stage, '/World/anim/CueBall')
        cueBallRig.CreateMayaNamespaceAttr().Set('cueball_rig')
        cueBallRig.CreateMayaReferenceAttr().Set(os.path.join(ASSET_BASE, 'Ball/Ball_rig.ma'))
    
    animVariant.AddVariant('cache')
    animVariant.SetVariantSelection('cache')
    with animVariant.GetVariantEditContext():
        # Creating cache layer and bring in as payload
        cueCacheLayerPath = 'shots/s00_01/s00_01_animcue.usd'
        if not os.path.exists(cueCacheLayerPath):
            payload = Usd.Stage.CreateNew(cueCacheLayerPath)
            payload.DefinePrim("/Ball")
            payload.Save()
        cueBallCache = UsdGeom.Xform.Define(stage, '/World/anim/CueBall')
        cueBallCache.GetPrim().SetPayload(cueCacheLayerPath, '/Ball')

if __name__ == '__main__':
    main()

