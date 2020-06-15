#!/pxrpythonsubst
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

'''
This script assumes you have models at

  models/
    Ball/
    Table/

It will create a new "set" (aggregate) model, Room_set at

  models/Room_set

'''

import os
ASSET_BASE = os.path.join(os.getcwd(), 'models')
TABLE_HEIGHT = 74.5
BALL_RADIUS = 5.715 * 0.5

def main():
    from pxr import Kind, Usd, UsdGeom

    setFilePath = os.path.join(ASSET_BASE, 'Room_set/Room_set.usd')

    # Create the model stage, the model prim, and also make the modelPrim the
    # default prim so that the layer can be referenced without specifying a
    # root path.
    stage = Usd.Stage.CreateNew(setFilePath)
    setModelPrim = UsdGeom.Xform.Define(stage, '/Room_set').GetPrim()
    stage.SetDefaultPrim(setModelPrim)

    # Lets viewing applications know how to orient a free camera properly
    UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)

    # Models can have a "kind" which can be used to categorize models into
    # different types.  This is useful for other applications to reason about
    # the model hierarchy.
    Usd.ModelAPI(setModelPrim).SetKind(Kind.Tokens.assembly)

    # add child models to our set.
    _AddModel(stage, '/Room_set/Furniture/Table', 'Table/Table.usd')

    ballNumber = 0

    import math
    for row in range(5):

        tableX = 10 + row * 2 * BALL_RADIUS * math.cos(math.pi/6)
        tableRowZ = -row * BALL_RADIUS
        numBallsInRow = row + 1

        for i in range(numBallsInRow):
            ballNumber += 1

            b = _AddModel(
                stage, 
                '/Room_set/Props/Ball_%d' % ballNumber, 
                'Ball/Ball.usd')

            # tableX and tableZ will arrange the balls into a triangle.
            tableZ = tableRowZ + i * 2 * BALL_RADIUS
            ballXlate = (tableX, TABLE_HEIGHT + BALL_RADIUS, tableZ)

            # Apply the UsdGeom.Xformable schema to the model, and then set the
            # transformation.
            # We are only going to translate the balls, but we still use
            # the helper XformCommonAPI schema to  instantiate a translate op.
            # XformCommonAPI helps ensure transform ops are interchangeable 
            # between applications, when you can make do with SRT + pivot
            UsdGeom.XformCommonAPI(b).SetTranslate(ballXlate)

            # we conveniently named the shadingVariant the same as the name of
            # the ball.
            shadingVariantName = b.GetName()

            # get the variantSet "shadingVariant" and then set it accordingly.
            vs = b.GetVariantSets().GetVariantSet('shadingVariant')
            vs.SetVariantSelection(shadingVariantName)

    stage.GetRootLayer().Save()
    
def _AddModel(stage, path, refPath):
    '''
    Adds a reference to refPath at the given path in the stage.  This will make
    sure the model hierarchy is maintained by ensuring that all ancestors of
    the path have "kind" set to "group".

    returns the referenced model.
    '''

    from pxr import Kind, Sdf, Usd, UsdGeom

    # convert path to an Sdf.Path which has several methods that are useful
    # when working with paths.  We use GetPrefixes() here which returns a list
    # of all the prim prefixes for a given path.  
    path = Sdf.Path(path)
    for prefixPath in path.GetPrefixes()[1:-1]:
        parentPrim = stage.GetPrimAtPath(prefixPath)
        if not parentPrim:
            parentPrim = UsdGeom.Xform.Define(stage, prefixPath).GetPrim()
            Usd.ModelAPI(parentPrim).SetKind(Kind.Tokens.group)
    
    # typeless def here because we'll be getting the type from the prim that
    # we're referencing.
    m = stage.DefinePrim(path)
    m.GetReferences().AddReference(os.path.join(ASSET_BASE, refPath))
    return m

if __name__ == '__main__':
    main()

