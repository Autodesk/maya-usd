import AL.usd
import AL.usd.utils
from AL.usd.utils import MayaTransformAPI, RotationOrder
from pxr import Usd, Sdf, Gf, UsdGeom
import unittest


class TestMayaTransformAPI(unittest.TestCase):

    def test_scale(self):
        stage = Usd.Stage.CreateInMemory()
        xform = UsdGeom.Xform.Define(stage, "/pCube1")
        api = MayaTransformAPI(xform.GetPrim(), False)
        self.assertTrue(api.isValid())
        api.scale(Gf.Vec3f(2,3,4), Usd.TimeCode.Default())
        ops = xform.GetOrderedXformOps()
        self.assertTrue(len(ops) == 1)
        self.assertTrue(ops[0].GetOpType() == UsdGeom.XformOp.TypeScale)

    def test_translate(self):
        stage = Usd.Stage.CreateInMemory()
        xform = UsdGeom.Xform.Define(stage, "/pCube1")
        api = MayaTransformAPI(xform.GetPrim(), False)
        self.assertTrue(api.isValid())
        api.scale(Gf.Vec3d(2,3,4), Usd.TimeCode.Default())
        ops = xform.GetOrderedXformOps()
        self.assertTrue(len(ops) == 1)
        self.assertTrue(ops[0].GetOpType() == UsdGeom.XformOp.TypeTranslate)

    def test_rotateAxis(self):
        stage = Usd.Stage.CreateInMemory()
        xform = UsdGeom.Xform.Define(stage, "/pCube1")
        api = MayaTransformAPI(xform.GetPrim(), False)
        self.assertTrue(api.isValid())
        api.rotateAxis(Gf.Vec3d(2,3,4), Usd.TimeCode.Default())
        ops = xform.GetOrderedXformOps()
        self.assertTrue(len(ops) == 1)
        self.assertTrue(ops[0].GetOpType() == UsdGeom.XformOp.TypeRotateXYZ)

    def test_scalePivot(self):
        stage = Usd.Stage.CreateInMemory()
        xform = UsdGeom.Xform.Define(stage, "/pCube1")
        api = MayaTransformAPI(xform.GetPrim(), False)
        self.assertTrue(api.isValid())
        api.scalePivot(Gf.Vec3f(2,3,4), Usd.TimeCode.Default())
        ops = xform.GetOrderedXformOps()
        self.assertTrue(len(ops) == 2)
        self.assertTrue(ops[0].GetOpType() == UsdGeom.XformOp.TypeTranslate)
        self.assertTrue(ops[1].GetOpType() == UsdGeom.XformOp.TypeTranslate)

    def test_rotatePivot(self):
        stage = Usd.Stage.CreateInMemory()
        xform = UsdGeom.Xform.Define(stage, "/pCube1")
        api = MayaTransformAPI(xform.GetPrim(), False)
        self.assertTrue(api.isValid())
        api.rotatePivot(Gf.Vec3f(2,3,4), Usd.TimeCode.Default())
        ops = xform.GetOrderedXformOps()
        self.assertTrue(len(ops) == 2)
        self.assertTrue(ops[0].GetOpType() == UsdGeom.XformOp.TypeTranslate)
        self.assertTrue(ops[1].GetOpType() == UsdGeom.XformOp.TypeTranslate)

    def test_scalePivotTranslate(self):
        stage = Usd.Stage.CreateInMemory()
        xform = UsdGeom.Xform.Define(stage, "/pCube1")
        api = MayaTransformAPI(xform.GetPrim(), False)
        self.assertTrue(api.isValid())
        api.scalePivotTranslate(Gf.Vec3f(2,3,4), Usd.TimeCode.Default())
        ops = xform.GetOrderedXformOps()
        self.assertTrue(len(ops) == 1)
        self.assertTrue(ops[0].GetOpType() == UsdGeom.XformOp.TypeTranslate)

    def test_rotatePivotTranslate(self):
        stage = Usd.Stage.CreateInMemory()
        xform = UsdGeom.Xform.Define(stage, "/pCube1")
        api = MayaTransformAPI(xform.GetPrim(), False)
        self.assertTrue(api.isValid())
        api.rotatePivotTranslate(Gf.Vec3f(2,3,4), Usd.TimeCode.Default())
        ops = xform.GetOrderedXformOps()
        self.assertTrue(len(ops) == 1)
        self.assertTrue(ops[0].GetOpType() == UsdGeom.XformOp.TypeTranslate)

    def test_rotateXYZ(self):
        stage = Usd.Stage.CreateInMemory()
        xform = UsdGeom.Xform.Define(stage, "/pCube1")
        api = MayaTransformAPI(xform.GetPrim(), False)
        self.assertTrue(api.isValid())
        api.rotate(Gf.Vec3f(2,3,4), RotationOrder.kXYZ, Usd.TimeCode.Default())
        ops = xform.GetOrderedXformOps()
        self.assertTrue(len(ops) == 1)
        self.assertTrue(ops[0].GetOpType() == UsdGeom.XformOp.TypeRotateXYZ)

    def test_rotateXZY(self):
        stage = Usd.Stage.CreateInMemory()
        xform = UsdGeom.Xform.Define(stage, "/pCube1")
        api = MayaTransformAPI(xform.GetPrim(), False)
        self.assertTrue(api.isValid())
        api.rotate(Gf.Vec3f(2,3,4), RotationOrder.kXZY, Usd.TimeCode.Default())
        ops = xform.GetOrderedXformOps()
        self.assertTrue(len(ops) == 1)
        self.assertTrue(ops[0].GetOpType() == UsdGeom.XformOp.TypeRotateXZY)

    def test_rotateYXZ(self):
        stage = Usd.Stage.CreateInMemory()
        xform = UsdGeom.Xform.Define(stage, "/pCube1")
        api = MayaTransformAPI(xform.GetPrim(), False)
        self.assertTrue(api.isValid())
        api.rotate(Gf.Vec3f(2,3,4), RotationOrder.kYXZ, Usd.TimeCode.Default())
        ops = xform.GetOrderedXformOps()
        self.assertTrue(len(ops) == 1)
        self.assertTrue(ops[0].GetOpType() == UsdGeom.XformOp.TypeRotateYXZ)

    def test_rotateYZX(self):
        stage = Usd.Stage.CreateInMemory()
        xform = UsdGeom.Xform.Define(stage, "/pCube1")
        api = MayaTransformAPI(xform.GetPrim(), False)
        self.assertTrue(api.isValid())
        api.rotate(Gf.Vec3f(2,3,4), RotationOrder.kYZX, Usd.TimeCode.Default())
        ops = xform.GetOrderedXformOps()
        self.assertTrue(len(ops) == 1)
        self.assertTrue(ops[0].GetOpType() == UsdGeom.XformOp.TypeRotateYZX)

    def test_rotateZXY(self):
        stage = Usd.Stage.CreateInMemory()
        xform = UsdGeom.Xform.Define(stage, "/pCube1")
        api = MayaTransformAPI(xform.GetPrim(), False)
        self.assertTrue(api.isValid())
        api.rotate(Gf.Vec3f(2,3,4), RotationOrder.kZXY, Usd.TimeCode.Default())
        ops = xform.GetOrderedXformOps()
        self.assertTrue(len(ops) == 1)
        self.assertTrue(ops[0].GetOpType() == UsdGeom.XformOp.TypeRotateZXY)

    def test_rotateZYX(self):
        stage = Usd.Stage.CreateInMemory()
        xform = UsdGeom.Xform.Define(stage, "/pCube1")
        api = MayaTransformAPI(xform.GetPrim(), False)
        self.assertTrue(api.isValid())
        api.rotate(Gf.Vec3f(2,3,4), RotationOrder.kZYX, Usd.TimeCode.Default())
        ops = xform.GetOrderedXformOps()
        self.assertTrue(len(ops) == 1)
        self.assertTrue(ops[0].GetOpType() == UsdGeom.XformOp.TypeRotateZYX)
