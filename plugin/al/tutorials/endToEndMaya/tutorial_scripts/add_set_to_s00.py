#!/usr/bin/env mayapy
#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import os
ASSET_BASE = os.path.join('../../', 'models')

def main():
    sequenceFilePath = 'shots/s00/s00.usd'
    setsLayoutLayerFilePath = 'shots/s00/s00_sets.usd'

    from pxr import Kind, Usd, UsdGeom, Sdf
    stage = Usd.Stage.Open(sequenceFilePath)

    # we use Sdf, a lower level library, to obtain the 'sets' layer.
    workingLayer = Sdf.Layer.FindOrOpen(setsLayoutLayerFilePath)
    assert stage.HasLocalLayer(workingLayer)

    # this makes the workingLayer the target for authoring operations by the
    # stage.
    stage.SetEditTarget(workingLayer)

    # Make sure the model-parents we need are well-specified
    Usd.ModelAPI(UsdGeom.Xform.Define(stage, '/World')).SetKind(Kind.Tokens.group)
    Usd.ModelAPI(UsdGeom.Xform.Define(stage, '/World/sets')).SetKind(Kind.Tokens.group)
    
    # in previous examples, we've been using GetReferences().AppendReference(...).  The
    # following uses .SetItems() instead which lets us explicitly set (replace)
    # the references at once instead of adding.
    stage.DefinePrim('/World/sets/Room_set').GetReferences().SetReferences([
        Sdf.Reference(os.path.join(ASSET_BASE, 'Room_set/Room_set.usd'))])

    stage.GetEditTarget().GetLayer().Save()

    print '==='
    print 'usdview %s' % sequenceFilePath
    print 'usdcat %s' % setsLayoutLayerFilePath

if __name__ == '__main__':
    main()

