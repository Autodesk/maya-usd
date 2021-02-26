#!/usr/bin/env mayapy
#
# Copyright 2018 Pixar
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
import unittest

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as OM

from pxr import Gf, Sdf, Tf, Usd, UsdGeom, UsdSkel, UsdUtils, Vt

import fixturesUtils

class testUsdExportSkeleton(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.inputPath = fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testSkeletonTopology(self):
        """Tests that the joint topology is correct."""

        mayaFile = os.path.join(self.inputPath, "UsdExportSkeletonTest", "UsdExportSkeleton.ma")
        cmds.file(mayaFile, force=True, open=True)

        usdFile = os.path.abspath('UsdExportSkeleton.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
                       shadingMode='none', exportSkels='auto')
        stage = Usd.Stage.Open(usdFile)

        skeleton = UsdSkel.Skeleton.Get(stage, '/SkelChar/Hips')
        self.assertTrue(skeleton)

        joints = skeleton.GetJointsAttr().Get()
        self.assertEqual(joints, Vt.TokenArray([
            "Hips",
            "Hips/Torso",
            "Hips/Torso/Chest",
            "Hips/Torso/Chest/UpChest",
            "Hips/Torso/Chest/UpChest/Neck",
            "Hips/Torso/Chest/UpChest/Neck/Head",
            "Hips/Torso/Chest/UpChest/Neck/Head/LEye",
            "Hips/Torso/Chest/UpChest/Neck/Head/REye",
            "Hips/Torso/Chest/UpChest/LShldr",
            "Hips/Torso/Chest/UpChest/LShldr/LArm",
            "Hips/Torso/Chest/UpChest/LShldr/LArm/LElbow",
            "Hips/Torso/Chest/UpChest/LShldr/LArm/LElbow/LHand",
            "Hips/Torso/Chest/UpChest/RShldr",
            "Hips/Torso/Chest/UpChest/RShldr/RArm",
            "Hips/Torso/Chest/UpChest/RShldr/RArm/RElbow",
            "Hips/Torso/Chest/UpChest/RShldr/RArm/RElbow/RHand",
            # note: skips ExtraJoints because it's not a joint.
            "Hips/Torso/Chest/UpChest/RShldr/RArm/RElbow/RHand/ExtraJoints/RHandPropAttach",
            "Hips/LLeg",
            "Hips/LLeg/LKnee",
            "Hips/LLeg/LKnee/LFoot",
            "Hips/LLeg/LKnee/LFoot/LToes",
            "Hips/RLeg",
            "Hips/RLeg/RKnee",
            "Hips/RLeg/RKnee/RFoot",
            "Hips/RLeg/RKnee/RFoot/RToes"
        ]))

    def testSkelTransforms(self):
        """
        Tests that the computed joint transforms in USD, when tarnsformed into
        world space, match the world space transforms of the Maya joints.
        """

        mayaFile = os.path.join(self.inputPath, "UsdExportSkeletonTest", "UsdExportSkeleton.ma")
        cmds.file(mayaFile, force=True, open=True)

        # frameRange = [1, 30]
        frameRange = [1, 3]

        # TODO: The joint hierarchy intentionally includes non-joint nodes,
        # which are expected to be ignored. However, when we try to extract
        # restTransforms from the dagPose, the intermediate transforms cause
        # problems, since they are not members of the dagPose. As a result,
        # no dag pose is exported. Need to come up with a way to handle this
        # correctly in export.
        print("Expect warnings about invalid restTransforms")
        usdFile = os.path.abspath('UsdExportSkeleton.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
                       shadingMode='none', frameRange=frameRange,
                       exportSkels='auto')
        stage = Usd.Stage.Open(usdFile)

        root = UsdSkel.Root.Get(stage, '/SkelChar')
        self.assertTrue(root)

        skelCache = UsdSkel.Cache()

        if Usd.GetVersion() > (0, 20, 8):
            skelCache.Populate(root, Usd.PrimDefaultPredicate)
        else:
            skelCache.Populate(root)

        skel = UsdSkel.Skeleton.Get(stage, '/SkelChar/Hips')
        self.assertTrue(skel)

        skelQuery = skelCache.GetSkelQuery(skel)
        self.assertTrue(skelQuery)

        xfCache = UsdGeom.XformCache()

        for frame in range(*frameRange):
            cmds.currentTime(frame, edit=True)
            xfCache.SetTime(frame)

            skelLocalToWorld = xfCache.GetLocalToWorldTransform(skelQuery.GetPrim())

            usdJointXforms = skelQuery.ComputeJointSkelTransforms(frame)

            for joint,usdJointXf in zip(skelQuery.GetJointOrder(),
                                        usdJointXforms):

                usdJointWorldXf = usdJointXf * skelLocalToWorld
                
                selList = OM.MSelectionList()
                selList.add(Sdf.Path(joint).name)

                dagPath = selList.getDagPath(0)
                mayaJointWorldXf = Gf.Matrix4d(*dagPath.inclusiveMatrix())

                self.assertTrue(Gf.IsClose(mayaJointWorldXf,
                                           usdJointWorldXf, 1e-5))

    def testSkelWithIdentityBindPose(self):
        """
        Tests export of a Skeleton when a bindPose is not fully setup.

        In this test, there is a maya dagPose node, but it is set to identity
        """

        mayaFile = os.path.join(self.inputPath, "UsdExportSkeletonTest",
            "UsdExportSkeletonWithIdentityBindPose.ma")
        cmds.file(mayaFile, force=True, open=True)

        frameRange = [1, 5]
        usdFile = os.path.abspath('UsdExportSkeletonWithIdentityBindPose.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
                       shadingMode='none', frameRange=frameRange,
                       exportSkels='auto')

        stage = Usd.Stage.Open(usdFile)

        skeleton = UsdSkel.Skeleton.Get(stage, '/cubeRig/skel/joint1')
        self.assertEqual(skeleton.GetBindTransformsAttr().Get(),
            Vt.Matrix4dArray([Gf.Matrix4d(1.0)]))
        self.assertEqual(skeleton.GetJointsAttr().Get(),
            Vt.TokenArray(['joint1']))
        self.assertEqual(skeleton.GetRestTransformsAttr().Get(),
            Vt.Matrix4dArray([Gf.Matrix4d(1.0)]))

        self.assertTrue(skeleton.GetPrim().HasAPI(UsdSkel.BindingAPI))
        skelBindingAPI = UsdSkel.BindingAPI(skeleton)
        self.assertTrue(skelBindingAPI)

        animSourcePrim = skelBindingAPI.GetAnimationSource()
        self.assertEqual(animSourcePrim.GetPath(),
            '/cubeRig/skel/joint1/Animation')
        animSource = UsdSkel.Animation(animSourcePrim)
        self.assertTrue(animSource)

        self.assertEqual(skeleton.GetJointsAttr().Get(),
            Vt.TokenArray(['joint1']))
        self.assertEqual(animSource.GetRotationsAttr().Get(),
            Vt.QuatfArray([Gf.Quatf(1.0, Gf.Vec3f(0.0))]))
        self.assertEqual(animSource.GetScalesAttr().Get(),
            Vt.Vec3hArray([Gf.Vec3h(1.0)]))
        self.assertEqual(
            animSource.GetTranslationsAttr().Get(Usd.TimeCode.Default()),
            Vt.Vec3fArray([Gf.Vec3f(5.0, 5.0, 0.0)]))

        self.assertEqual(
            animSource.GetTranslationsAttr().Get(1.0),
            Vt.Vec3fArray([Gf.Vec3f(0.0, 0.0, 0.0)]))
        self.assertEqual(
            animSource.GetTranslationsAttr().Get(2.0),
            Vt.Vec3fArray([Gf.Vec3f(1.25, 1.25, 0.0)]))
        self.assertEqual(
            animSource.GetTranslationsAttr().Get(3.0),
            Vt.Vec3fArray([Gf.Vec3f(2.5, 2.5, 0.0)]))
        self.assertEqual(
            animSource.GetTranslationsAttr().Get(4.0),
            Vt.Vec3fArray([Gf.Vec3f(3.75, 3.75, 0.0)]))
        self.assertEqual(
            animSource.GetTranslationsAttr().Get(5.0),
            Vt.Vec3fArray([Gf.Vec3f(5.0, 5.0, 0.0)]))

    def testSkelRestXformsWithNoDagPose(self):
        """
        Tests export of rest xforms when there is no dagPose node at all.
        """
        mayaFile = os.path.join(self.inputPath, "UsdExportSkeletonTest",
            "UsdExportSkeletonNoDagPose.ma")
        cmds.file(mayaFile, force=True, open=True)

        usdFile = os.path.abspath('UsdExportSkeletonRestXformsWithNoDagPose.usda')
        cmds.select('skel_root')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFile,
                           shadingMode='none', exportSkels='auto', selection=True)

        stage = Usd.Stage.Open(usdFile)

        skeleton = UsdSkel.Skeleton.Get(stage, '/skel_root/joint1')

        self.assertEqual(skeleton.GetJointsAttr().Get(),
            Vt.TokenArray(['joint1',
                           'joint1/joint2',
                           'joint1/joint2/joint3',
                           'joint1/joint2/joint3/joint4']))

        self.assertEqual(
            skeleton.GetBindTransformsAttr().Get(),
            Vt.Matrix4dArray([
                Gf.Matrix4d( (-1, 0, 0, 0), (0, 1, 0, 0), (0, 0, -1, 0), (0, 0, 0, 1) ),
                Gf.Matrix4d( (0, -1, 0, 0), (-1, 0, 0, 0), (0, 0, -1, 0), (3, 0, 0, 1) ),
                Gf.Matrix4d( (0, -1, 0, 0), (0, 0, -1, 0), (1, 0, 0, 0), (3, 0, -2, 1) ),
                Gf.Matrix4d( (0, -1, 0, 0), (1, 0, 0, 0), (0, 0, 1, 0), (3, 0, -4, 1) ),
            ])
        )

        self.assertEqual(
            skeleton.GetRestTransformsAttr().Get(),
            Vt.Matrix4dArray([
                Gf.Matrix4d( (-1, 0, 0, 0), (0, 1, 0, 0), (0, 0, -1, 0), (0, 0, 0, 1) ),
                Gf.Matrix4d( (0, -1, 0, 0), (1, 0, 0, 0), (0, 0, 1, 0), (-3, 0, 0, 1) ),
                Gf.Matrix4d( (1, 0, 0, 0), (0, 0, 1, 0), (0, -1, 0, 0), (0, 0, 2, 1) ),
                Gf.Matrix4d( (1, 0, 0, 0), (0, 0, 1, 0), (0, -1, 0, 0), (0, 2, 0, 1) ),
            ])
        )        

    def testSkelWithJointsAtSceneRoot(self):
        """
        Tests that exporting joints at the scene root errors, since joints need
        to be encapsulated inside a transform or other node that can be
        converted into a SkelRoot.
        """
        mayaFile = os.path.join(self.inputPath, "UsdExportSkeletonTest", "UsdExportSkeletonAtSceneRoot.ma")
        cmds.file(mayaFile, force=True, open=True)

        usdFile = os.path.abspath('UsdExportSkeletonAtSceneRoot.usda')
        with self.assertRaises(RuntimeError):
            cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
                           shadingMode='none', exportSkels='auto')


    def testSkelForSegfault(self):
        """
        Certain skeletons cause heap corruption and segfaulting when exported multiple times.
        Because this can happen anywhere from the first to last time, we run this test multiple times.
        """
        mayaFile = os.path.join(self.inputPath, "UsdExportSkeletonTest", "UsdExportSkeletonSegfaultTest.ma")
        cmds.file(mayaFile, force=True, open=True)

        usdFile = os.path.abspath('UsdExportSkeletonSegfaultTest.usda')
        cmds.select('skinned_mesh')

        # Run 5 times because the crash only happens every few runs.
        for _ in range(5):
            cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFile,
                         shadingMode='none', exportSkels='auto', selection=True)
            
    def testSkelMissingJointFromDagPose(self):
        """
        Check that dagPoses that don't contain all desired joints issue an
        appropriate warning
        """
        mayaFile = os.path.join(self.inputPath, "UsdExportSkeletonTest", "UsdExportSkeletonBindPoseMissingJoints.ma")
        cmds.file(mayaFile, force=True, open=True)

        usdFile = os.path.abspath('UsdExportBindPoseMissingJointsTest.usda')

        joints = cmds.listRelatives('joint_grp', allDescendents=True, type='joint')
        bindMembers = cmds.dagPose('dagPose1', q=1, members=1)
        nonBindJoints = [j for j in joints if j not in bindMembers]
        self.assertEqual(nonBindJoints, [u'joint4'])

        delegate = UsdUtils.CoalescingDiagnosticDelegate()

        cmds.select('joint_grp')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFile, shadingMode='none',
                           exportSkels='auto', selection=True)

        messages = delegate.TakeUncoalescedDiagnostics()
        warnings = [x.commentary for x in messages if x.diagnosticCode == Tf.TF_DIAGNOSTIC_WARNING_TYPE]
        missingJointWarnings = [x for x in warnings if 'is not a member of dagPose' in x]
        self.assertEqual(len(missingJointWarnings), 1)
        self.assertIn("Node 'joint4' is not a member of dagPose 'dagPose1'",
                      missingJointWarnings[0])

    def testSkelBindPoseSparseIndices(self):
        """
        Check that if a dagPose has sparse indices on some of it's attributes,
        with differing number of created indices, that things still work.
        """
        mayaFile = os.path.join(self.inputPath, "UsdExportSkeletonTest", "UsdExportSkeletonBindPoseSparseIndices.ma")
        cmds.file(mayaFile, force=True, open=True)

        usdFile = os.path.abspath('UsdExportBindPoseSparseIndicesTest.usda')

        cmds.select('joint_grp')
        cmds.mayaUSDExport(mergeTransformAndShape=True, file=usdFile, shadingMode='none',
                           exportSkels='auto', selection=True)

        stage = Usd.Stage.Open(usdFile)

        skeleton = UsdSkel.Skeleton.Get(stage, '/joint_grp/joint1')
        self.assertEqual(skeleton.GetRestTransformsAttr().Get(),
            Vt.Matrix4dArray(
                # If we're not correlating using logical indices correctly, we may get this
                # matrix in here somehwere (which we shouldn't):
                # Gf.Matrix4d( (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (777, 888, 999, 1) ),
                [Gf.Matrix4d( (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 2, 1) ),
                 Gf.Matrix4d( (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 3, 0, 1) ),
                 Gf.Matrix4d( (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (5, 0, 0, 1) )]))

if __name__ == '__main__':
    unittest.main(verbosity=2)
