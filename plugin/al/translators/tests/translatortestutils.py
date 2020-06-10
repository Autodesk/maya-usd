
import tempfile
import maya.cmds as mc
from AL import usdmaya

def getStage():
    """
    Grabs the first stage from the cache
    """
    stageCache = usdmaya.StageCache.Get()
    stage = stageCache.GetAllStages()[0]
    return stage


class SceneWithSphere:
    stage = None
    sphereXformName = ""
    sphereShapeName = ""

def importStageWithSphere(proxyShapeName='AL_usdmaya_Proxy'):
    """
    Creates a USD scene containing a single sphere.
    """

    # Create sphere in Maya and export a .usda file
    sphereXformName, sphereShapeName = mc.polySphere()
    mc.select(sphereXformName)
    tempFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="test_MeshTranslator_", delete=False)
    tempFile.close()
    mc.file(tempFile.name, exportSelected=True, force=True, type="AL usdmaya export")
    dir(tempFile)
    print("tempFile ", tempFile.name)

    # clear scene
    mc.file(f=True, new=True)
    mc.AL_usdmaya_ProxyShapeImport(file=tempFile.name, name=proxyShapeName)

    data = SceneWithSphere()
    data.stage = getStage()
    data.sphereXformName = sphereXformName
    data.sphereShapeName = sphereShapeName
    return data

def importStageWithNurbsCircle():
    # Create nurbs circle in Maya and export a .usda file
    mc.CreateNURBSCircle()
    mc.select("nurbsCircle1")
    tempFile = tempfile.NamedTemporaryFile(suffix=".usda", prefix="test_NurbsCurveTranslator_", delete=True)
    mc.file(tempFile.name, exportSelected=True, force=True, type="AL usdmaya export")

    # clear scene
    mc.file(f=True, new=True)
    mc.AL_usdmaya_ProxyShapeImport(file=tempFile.name)
    return getStage()
