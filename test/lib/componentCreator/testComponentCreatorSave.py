import unittest

import os.path
import tempfile
from distutils.dir_util import copy_tree

import fixturesUtils
import testUtils
import mayaUsd.lib
import mayaUsd.ufe
from maya import cmds
from pxr import Sdf, Usd

from testComponentCreatorBase import _ComponentCreatorTestBase

class SaveToComponentTestCase(_ComponentCreatorTestBase, unittest.TestCase):
    """
    Tests for usd_component_creator_plugin.create_component.add_to_component_from_nodes
    and then saving and reloading.
    """

    _tempFolder = None

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

        testFolderName = "component3Variants"
        fromDirectory = testUtils.getTestScene(testFolderName)
        toDirectory = os.path.join(tempfile.gettempdir(), 'SaveToComponentTestCase')
        copy_tree(fromDirectory, toDirectory)
        SaveToComponentTestCase._tempFolder = toDirectory

    @classmethod
    def tearDownClass(cls):
        if SaveToComponentTestCase._tempFolder:
            if os.path.exists(SaveToComponentTestCase._tempFolder):
                import shutil
                shutil.rmtree(SaveToComponentTestCase._tempFolder)
        cls._resetDefaultTemplate()
        return super().tearDownClass()

    def _findVariantSet(self, desc, withName):
        variantSetMap = desc.GetVariantSets()
        self.assertGreaterEqual(len(variantSetMap), 1, "There must be at least one variant set")
        for variantSetName, variantSet in variantSetMap.items():
            if withName not in variantSetName:
                continue
            return variantSet
        return None

    def setUp(self):
        self._setUpCC()
        # Clear the variant editor state so there is no lingering component from a
        # previous test.
        self._resetDefaultTemplate()
        return super().setUp()

    def testSaveAndReload(self):
        """
        Open a Maya scene containing a USD stage of a component.
        Modify all variants of the component by replacing its data with a new node.
        Save the scene.  Re-open the scene and verify that the component is still
        valid and has the new data.
        """
        before = self._snapshotProxyShapes()

        mayaSceneFilePath = os.path.join(SaveToComponentTestCase._tempFolder, "repro-3530.ma")
        cmds.file(mayaSceneFilePath, open=True)

        proxy = self._findNewProxyShape(before)
        self.assertIsNotNone(proxy)
        stage = mayaUsd.ufe.getStage(proxy)
        self.assertIsNotNone(stage)
        desc = self._getDescFromStage(stage)
        self.assertIsNotNone(desc, 'Could not get ComponentDescription from the Maya scene')

        first_vs = self._findVariantSet(desc, 'variant_set_1')
        self.assertTrue(first_vs)
        self.assertGreaterEqual(len(first_vs.GetVariants()), 3, "There must be at least three variants in the set {}".format(first_vs.GetName()))

        variantsMap = first_vs.GetVariants()
        self.assertGreaterEqual(len(variantsMap), 3, "There must be at least three variants")

        for variantName in variantsMap.keys():
            polyCubeName = cmds.polyCube(name='pCubeExtra')[0]
            self.assertIn('pCubeExtra', polyCubeName)

            # The following code is equivalent to add_to_component_from_nodes,
            # but that function does not work in maya batch mode and we need to run this test in batch mode
            # because the Maya save would pop-up a dialog in interactive mode.
            #
            # from usd_component_creator_plugin import add_to_component_from_nodes
            # result = add_to_component_from_nodes(
            #     [polyCubeName],
            #     [(first_vs.name, variantName)],
            #     is_replacing=True,
            #     component_desc=desc)
            # self.assertTrue(result)

            from AdskUsdComponentCreator import CreateFromFileCommand
            from usd_component_creator_plugin import ExportNodesCommand, execute_ufe_command, AddComponentToManagerCommand, UfeCommandWrapper
            from usd_component_creator_plugin.create_component import _generate_unique_temp_filename, _update_options_for_node, _update_options_for_purpose_from_nodes

            component_desc = desc
            nodes = [polyCubeName]
            variant_selections = [(first_vs.name, variantName)]
            is_replacing = True
            export_options = None
            purpose = None

            options = component_desc.GetOptions().Clone()
            _update_options_for_node(options, nodes[0], nodes)
            options.component_variants = variant_selections if variant_selections else []
            options.replace_variant_content = is_replacing
            # Never change the default variant when appending to an existing component.
            options.is_default_variant = False

            input_usd_filename = _generate_unique_temp_filename(options)
            export_cmd = ExportNodesCommand(nodes, input_usd_filename, export_options)
            execute_ufe_command(export_cmd)

            creation_options = _update_options_for_purpose_from_nodes(options, purpose, input_usd_filename)
            creation_options.Validate()

            delete_input_file = False
            create_cmd = CreateFromFileCommand(component_desc, input_usd_filename, creation_options, delete_input_file)
            self.assertTrue(UfeCommandWrapper.executeWithUndo(create_cmd))

            add_cmd = AddComponentToManagerCommand(create_cmd)
            execute_ufe_command(add_cmd)


        updated_desc = self._getDescFromStage(stage)
        self.assertIsNotNone(updated_desc, 'Could not get updated ComponentDescription')

        # Save the file. Make sure the USD edits will go to a USD file.
        cmds.optionVar(intValue=('mayaUsd_ConfirmExistingFileSave', 0))
        saveLocation = 1
        cmds.optionVar(intValue=(mayaUsd.lib.OptionVarTokens.SerializedUsdEditsLocation, saveLocation))
        cmds.file(save=True, force=True)

        cmds.file(new=True, force=True)
        cmds.file(mayaSceneFilePath, open=True)

        proxy = self._findNewProxyShape(before)
        self.assertIsNotNone(proxy)
        stage = mayaUsd.ufe.getStage(proxy)
        self.assertIsNotNone(stage)
        desc = self._getDescFromStage(stage)
        self.assertIsNotNone(desc, 'Could not get ComponentDescription from the Maya scene')

        first_vs = self._findVariantSet(desc, 'variant_set_1')
        self.assertTrue(first_vs)

        variantsMap = first_vs.GetVariants()
        self.assertGreaterEqual(len(variantsMap), 3, "There must be at least three variants")

        variant_to_cube_map = {
            'pPlane1': 'pCubeExtra',
            'pPlane2': 'pCubeExtra1',
            'pPlane3': 'pCubeExtra2',
        }

        found_cubes = set()

        for variant in variantsMap.values():
            primPath = desc.root_prim_path
            prim  = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim)
            prim.GetVariantSet(first_vs.GetName()).SetVariantSelection(variant.GetName())

            stage.Reload()

            expected_cube_name = variant_to_cube_map.get(variant.GetName())
            self.assertIsNotNone(expected_cube_name, "No expected cube name for variant {}".format(variant.GetName()))

            geoPrim = stage.GetPrimAtPath(Sdf.Path('/root/geo'))
            for child in geoPrim.GetChildren():
                if child.GetTypeName() == 'Mesh':
                    self.assertEqual(expected_cube_name, child.GetName(), f"The variant {variant.GetName()} should have a cube named {expected_cube_name}")
                    found_cubes.add(child.GetName())

        self.assertEqual(len(found_cubes), len(variantsMap), "There should be one cube for each variant")


if __name__ == '__main__':
    fixturesUtils.runTests(globals())

