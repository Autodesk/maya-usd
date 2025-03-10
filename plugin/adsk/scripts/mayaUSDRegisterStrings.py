# Copyright 2021 Autodesk
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

import maya.cmds as cmds
import maya.mel as mel
from mayaUsdLibRegisterStrings import registerPluginResources, unregisterPluginResources, getPluginResource

__mayaUSDStringResources = {
    "kButtonYes": "Yes",
    "kButtonNo": "No",
    "kDiscardStageEditsTitle": "Discard Edits on ^1s's Layers",
    "kDiscardStageEditsLoadMsg": "Are you sure you want to load in a new file as the stage source?\n\nAll edits on your layers in ^1s will be discarded.",
    "kDiscardStageEditsReloadMsg": "Are you sure you want to reload ^1s as the stage source?\n\nAll edits on your layers (except the session layer) in ^2s will be discarded.",
    "kLoadUSDFile": "Load USD File",
    "kFileOptions": "File Options",
    "kRelativePathOptions": "Relative Pathing",
    "kMakePathRelativeToSceneFile": "Make Path Relative to Scene File",
    "kMakePathRelativeToSceneFileAnn": "Path will be relative to your Maya scene file.",
    "kMakePathRelativeToEditTargetLayer": "Make Path Relative to Edit Target Layer Directory",
    "kMakePathRelativeToEditTargetLayerAnn": "Enable to activate relative pathing to your current edit target layer's directory.",
    "kMakePathRelativeToImageEditTargetLayer": "Make Path Relative to Edit Target Layer Directory",
    "kMakePathRelativeToImageEditTargetLayerAnn": "Enable to activate relative pathing to your current edit target layer's directory.",
    "kMakePathRelativeToParentLayer": "Make Path Relative to Parent Layer Directory",
    "kMakePathRelativeToParentLayerAnn": "Enable to activate relative pathing to your current parent layer's directory.",
    "kUnresolvedPath": "Path Preview:",
    "kUnresolvedPathAnn": "This field indicates the path with the file name currently chosen in your text input. Note: This is the string that will be written out to the file in the chosen directory in order to enable portability.",
    "kCompositionArcOptions": "Composition Arc Options",
    "kPreview": "Preview",

    "kButtonSave": "Save",
    "kButtonSet": "Set",
    "kButtonCancel": "Cancel",
    "kButtonAdd": "Add",
    "kCreateUsdStageFromFile": "Create USD Stage from File",
    "kCreateUsdStageFromFileOptions": "Create USD Stage from File Options",
    "kEditAsMayaData": "Edit As Maya Data",
    "kEditAsMayaDataOptions": "Edit As Maya Data Options",
    "kHelpOnEditAsMayaDataOptions": "Help on Edit As Maya Data Options",
    "kCreateStageFromFile": "Create Stage from File",
    "kCreateStage": "Create",
    "kDefaultPrim": "Default Prim",
    "kDefaultPrimAnn": "As part of its metadata, each stage can identify a default prim. This is the primitive that is referenced in if you reference in a file. Right-click a root prim in a stage to set it as your default prim. Use the same method to clear a default prim once it has been set.",
    "kExcludePrimPaths": "Exclude Prim Paths:",
    "kExcludePrimPathsAnn": "Specify the path of a prim to exclude it from the viewport display. Multiple prim paths must be separated by a comma.",
    "kExcludePrimPathsSbm": "Specify the path of a prim to exclude it from the viewport display.",
    "kFileAnn": "Load in a file as the stage source.",
    "kInvalidSelectionKind": "Invalid Maya Usd selection kind!",
    "kKinds": "Kinds",
    "kLabelStage": "Stage",
    "kLabelStageSource": "Stage Source",
    "kLabelStageDisplay": "Stage Display",
    "kStageAdvancedLabel": "Advanced",
    "kStageAxisUnitConversionLabel": "Axis & Unit Conversion",
    "kStageUpAxisLabel": "Up Axis",
    "kStageUpAxisAnn": "If selected, when an up axis mismatch is detected\n" +
                       "between the imported data and your scene preferences,\n" +
                       "an automatic correction will be performed.",
    "kStageUnitLabel": "Unit",
    "kStageUnitAnn": "If selected, when a unit mismatch is detected\n" +
                     "between the imported data and your scene preferences,\n" +
                     "an automatic correction will be performed.",
    "kStageAxisAndUnitMethod": "Method",
    # Note: initial <b> is used to force Qt to render the text as HTML.
    "kStageAxisAndUnitMethodAnn": "<b></b>Select the method for axis/unit conversions.<br/>" +
                                  "<br/>" +
                                  "<b>Rotate/Scale</b>: Rotate/Scale the stage.<br/>" +
                                  "<b>Overwrite Maya Preferences</b>: Update Maya's axis/unit preferences based on the imported data.",
    "kStageAxisAndUnitRotateScale": "Rotate/Scale",
    "kStageAxisAndUnitOverwritePrefs": "Overwrite Maya Preferences",
    "kLoad": "Load",
    "kLoadPayloads": "Load Payloads:",
    "kLoadPayloadsAnn": "When on, loads all prims marked as payloads. When off, all prims marked as payloads and their children are not loaded.",
    "kLoadPayloadsSbm": "Loads prims marked as payloads",
    "kMenuAddSublayer": "Add Sublayer",
    "kMenuAddParentLayer": "Add Parent Layer",
    "kMenuClear": "Clear",
    "kMenuReload": "Reload",
    "kMenuLayerEditor": "USD Layer Editor",
    "kContextMenuLayerEditor": "USD Layer Editor...",
    "kMenuLayerEditorAnn": "Organize and edit USD data in layers",
    "kMenuLoadSublayers": "Load Sublayers...",
    "kMenuLock": "Lock",
    "kMenuLockLayerAndSublayers": "Lock Layer and Sublayers",
    "kMenuMute": "Mute",
    "kMenuPrintToScriptEditor": "Print to Script Editor",
    "kMenuRemove": "Remove",
    "kMenuSelectPrimsWithSpec": "Select Prims With Spec",
    "kMenuStageCreateMenuError": "Could not create mayaUSD create menu",
    "kMenuStageWithNewLayer": "Stage with New Layer",
    "kMenuStageWithNewLayerAnn": "Create a new, empty USD Stage",
    "kMenuStageFromFile": "Stage From File...",
    "kMenuStageFromFileAnn": "Create a USD Stage from an existing USD file",
    "kMenuStageFromFileOptionsAnn": "Create a USD Stage from an existing USD file options",
    "kMenuStageSubMenu": "Universal Scene Description (USD)",
    "kMenuStageSubMenuAnn": "Create a USD stage",
    "kMenuSaveAs": "Save As...",
    "kMenuSetAs": "Set As...",
    "kMenuSaveEdits": "Save Edits",
    "kMenuUnlock": "Unlock",
    "kMenuUnlockLayerAndSublayers": "Unlock Layer and Sublayers",
    "kMenuUnmute": "Unmute",
    "kPointInstances": "Point Instances",
    "kPurposeAnn": "Toggle purpose categories on and off to change their visibility in the viewport.",
    "kPurposeLabel": "Purpose",
    "kPurposeOption1": "Guide",
    "kPurposeOption2": "Proxy",
    "kPurposeOption3": "Render",
    "kPrimPath": "Prim Path:",
    "kPrimPathAnn": "Specify the path of a prim to display it alone in the viewport. If a prim path is not specified or a matching prim path is not found, all prims in the stage are displayed.",
    "kPrimPathSbm": "Specify the path of a prim to display it alone in the viewport.",
    "kRootLayer": "Root Layer",
    "kRootLayerAnn": "Identifies the root layer of a stage. If a file path is shown in this field, the root layer is a file on disk. If a layerName is shown in this field, the root layer is an anonymous layer.",
    "kSaveAndClose": "Save and Close",
    "kSaveOption1": "Save the Maya scene file and USD files.",
    "kSaveOption2": "Save all edits (including USD) to the Maya scene file.",
    "kSaveOption3": "Save the Maya scene file only (USD edits will not be saved).",
    "kSaveOptionAnn1": "Select this option to save your Maya scene file (.ma, .mb) and your USD files (.usd, .usda, .usdc) to disk respectively.",
    "kSaveOptionAnn2": "Select this option to save your current Maya session with in-memory USD edits into a Maya scene file on disk (.ma, .mb). Important: any files exceeding the limit of 2 GB cannot be saved.",
    "kSaveOptionAnn3": "Select this option to ignore all USD edits and save only your Maya scene file (.ma, .mb).",
    "kSaveOptionAsk": "Ask me",
    "kSaveOptionAskAnn": "If disabled, the selected USD save operation will always proceed.",
    "kSaveOptionNoPrompt": "Don't ask me again",
    "kSaveOptionNoPromptAnn": "You can re-enable this prompt under File|Save Scene Options.",
    "kSaveOptionTitle": "Save USD Options",
    "kSaveOptionUnsavedEdits": "You have unsaved USD edits. How would you like to proceed?",
    "kShareStageShareable": "Shareable",
    "kShareStageAnn": "Toggle sharable on and off to sandbox your changes and create a new stage",
    "kShowArrayAttributes": "Show Array Attributes",
    "kAddSchemaMenuItem": "Add Schema",
    "kRemoveSchemaMenuItem": "Remove Schema",
    "kAddSchemaInstanceTitle": "Add Schema Instance",
    "kAddSchemaInstanceMessage": "%s Schema Instance Name",
    "kCannotApplySchemaWarning": 'Cannot apply schema "%s": no USD prim are currently selected.',
    "kCannotRemoveSchemaWarning": 'Cannot remove schema "%s": no USD prim are currently selected.',
    "kUSDPointInstancesPickMode_PointInstancer": "Point Instancer",
    "kUSDPointInstancesPickMode_PointInstancerAnn": "Selection mode for all prims set as point instances.",
    "kUSDPointInstancesPickMode_Instances": "Instances",
    "kUSDPointInstancesPickMode_InstancesAnn": "Selection mode for prims set as instanceable only.",
    "kUSDPointInstancesPickMode_Prototypes": "Prototypes",
    "kUSDPointInstancesPickMode_PrototypesAnn": "Selection mode for prims set as prototypes only.",
    "kUSDSelectionMode": "USD Selection Mode",
    "kUSDSelectionModeAnn": "Choose a selection mode to reflect changes when you select prims in the Viewport and Outliner. Note: Default fallback selection mode is by prim.",
    "kUSDSelectionModeAssemblyAnn": "Selection mode for prims set to assembly kind. Tip: Set assembly kind in the Attribute Editor > Metadata to prims that are part of an important group.",
    "kUSDSelectionModeComponentAnn": "Selection mode for prims set to component kind. Tip: Set component kind in the Attribute Editor > Metadata to prims that are a collection of assets.",
    "kUSDSelectionModeCustom": "Selection mode for prims set to ^1s kind.",
    "kUSDSelectionModeGroupAnn": "Selection mode for prims set to group kind (including prims set to assembly kind). Tip: Set group kind in the Attribute Editor > Metadata to prims that are grouped.",
    "kUSDSelectionModeModelAnn": "Selection mode for prims in the model hierarchy (including prims set to group, assembly and component kind).",
    "kUSDSelectionModeNone": "(None)",
    "kUSDSelectionModeNoneAnn": "Selection mode for prims that have no kind set.",
    "kUSDSelectionModeSubComponentAnn": "Selection mode for prims set to subcomponent kind. Tip: Set subcomponent kind in the Attribute Editor > Metadata to prims that are an individual asset.",
    "kTimeAnn": "Edits the current time value of a stage, which corresponds to the animation frame drawn in the viewport. By default, this value connects to Maya's global time node.",
    "kTipYouCanChooseMultipleFiles": "<b>Tip:</b>  You can choose multiple files.",
    "kUniversalSceneDescription": "Universal Scene Description",
    "kUsdOptionsFrameLabel": "Universal Scene Description (USD) Options",
    "kSaveOption2GBWarning": "<b>Important</b>: per layer, any data exceeding the limit of 2GB will not be saved.",

    # All strings for export dialog:
    "kExportAnimDataAnn": "Exports Maya animation data as USD time samples.",
    "kExportAnimDataLbl": "Animation Data",
    "kExportBlendShapesAnn": "Exports Maya Blend Shapes as USD blendshapes. Requires skeletons to be exported as well.",
    "kExportBlendShapesLbl": "Blend Shapes",
    "kExportColorSetsAnn": "Exports Maya Color Sets as USD primvars.",
    "kExportColorSetsLbl": "Color Sets",
    "kExportMaterialsCBAnn": "If selected, material/shading data is included as part of the export.",
    "kExportMaterialsCBLbl": "Materials",
    "kExportAssignedMaterialsAnn": "If selected, only materials assigned to geometry will be exported.",
    "kExportAssignedMaterialsLbl": "Assigned Materials Only",
    "kExportMeshesLbl": "Meshes",
    "kExportMeshesAnn": "If selected, mesh geometry is extracted for USD export.",
    "kExportCamerasLbl": "Cameras",
    "kExportCamerasAnn": "If selected, cameras are extracted for USD export.",
    "kExportLightsLbl": "Lights",
    "kExportLightsAnn": "If selected, lights are extracted for USD export.",
    "kExportComponentTagsAnn": "If selected, component tags get exported as USDGeomSubsets. Note: Edges and vertices are unsupported in USD.",
    "kExportComponentTagsLbl": "Component Tags",
    "kExportStagesAsRefsAnn": "If selected, USD stages created from file will export as USD references.",
    "kExportStagesAsRefsLbl": "USD Stages as USD References",
    "kExportCurvesAnn": "If selected, curves get exported to USD primTypes NurbsCurves or BasisCurves.",
    "kExportCurvesLbl": "NURBS Curves",
    "kExportDefaultFormatAnn": "Select whether the .usd file is written out in binary or ASCII",
    "kExportDefaultFormatLbl": ".usd File Format:",
    "kExportDefaultFormatBinLbl": "Binary",
    "kExportDefaultFormatAscLbl": "ASCII",
    "kExportDefaultFormatStatus": "Select whether the .usd file is written out in binary or ASCII. You can save a file in .usdc (binary), or .usda (ASCII) format. Manually entering a file name with an extension overrides the selection in this drop-down menu.",
    "kExportDisplayColorAnn": "If selected, exports the diffuse color of the geometry's bound shader as a displayColor primvar on the USD mesh.",
    "kExportDisplayColorLbl": "Display Colors",
    "kExportEulerFilterAnn": "Exports the euler angle filtering that was performed in Maya.",
    "kExportEulerFilterLbl": "Euler Filter",
    "kExportFrameAdvancedLbl": "Advanced",
    "kExportFrameAnimationLbl": "Animation",
    "kExportFrameGeometryLbl": "Geometry",
    "kExportFrameMaterialsLbl": "Materials",
    "kExportFrameOutputLbl": "Output",
    "kExportFrameRangeBtn": "Use Animation Range",
    "kExportFrameRangeLbl": "Frame Range Start/End:",
    "kExportFrameSamplesAnn": "Specifies the value(s) used to multi-sample time frames during animation export. Multiple values separated by a space (-0.1 0.2) are supported.",
    "kExportFrameSamplesLbl": "Frame Sample:",
    "kExportFrameStepAnn": "Specifies the increment between USD time sample frames during animation export",
    "kExportFrameStepLbl": "Frame Step:",
    "kExportInstancesAnn": "Exports Maya instances as USD instanceable references.",
    "kExportInstancesLbl": "Instances:",
    "kExportInstancesFlatLbl": "Flatten",
    "kExportInstancesRefLbl": "Convert to USD Instanceable References",
    "kExportPluginConfigLbl": "Plug-in Configurations",
    "kExportPluginConfigAnn": "Turn on the plug-ins you wish to include for this process.\n" +
                              "When turned on, plug-ins (like Arnold) can modify or create data to ensure compatibility.\n" +
                              "Plug-ins may have extra settings you can adjust.",
    "kExportPluginConfigButtonAnn": "Options",
    "kExportMaterialsAnn": "Select the material(s) to bind to prims for export. With USD, you can bind multiple materials to prims.",
    "kExportMaterialsDefaultScopeName": "mtl",
    "kExportMergeShapesAnn": "Merges Maya transform and shape nodes into a single USD prim.",
    "kExportMergeShapesLbl": "Merge Transform and Shape Nodes",
    "kExportIncludeEmptyTransformsAnn": "If selected, transforms that don't contain objects other\n" +
                                        "than transforms will be included as part of the export\n" +
                                        "(ex: empty groups, or groups that contain empty groups,\n" +
                                        "and so on).",
    "kExportIncludeEmptyTransformsLbl": "Include Empty Transforms",
    "kExportNamespacesAnn": "By default, namespaces are exported to the USD file in the following format: nameSpaceExample_pPlatonic1",
    "kExportNamespacesLbl": "Include Namespaces",
    "kExportWorldspaceAnn": "Exports the root prims with their worldspace transform instead of their local transform.",
    "kExportWorldspaceLbl": "Worldspace Roots",
    "kExportRootPrimAnn": "Name the root/parent prim for your exported data.",
    "kExportRootPrimLbl": "Create Root Prim:",
    "kExportRootPrimPht": "USD Prim Name",
    "kExportRootPrimTypeLbl": "Root Prim Type:",
    "kExportScopeLbl": "Scope",
    "kExportXformLbl": "Xform",
    "kExportScopeAnn": "scope",
    "kExportXformAnn": "xform",
    "kExportSkelsAnn": "Exports Maya joints as part of a USD skeleton.",
    "kExportSkelsLbl": "Skeletons:",
    "kExportSkelsNoneLbl": "None",
    "kExportSkelsAllLbl": "All (Automatically Create SkelRoots)",
    "kExportSkelsRootLbl": "Only under SkelRoots",
    "kExportSkinClustersAnn": "Exports Maya skin clusters as part of a USD skeleton.",
    "kExportSkinClustersLbl": "Skin Clusters",
    "kExportStaticSingleSampleAnn": "Converts animated values with a single time sample to be static instead.",
    "kExportStaticSingleSampleLbl": "Static Single Sample",
    "kExportSubdMethodAnn": "Exports the selected subdivision method as a USD uniform attribute.",
    "kExportSubdMethodLbl": "Subdivision Method:",
    "kExportSubdMethodCCLbl": "Catmull-Clark",
    "kExportSubdMethodBiLbl": "Bilinear",
    "kExportSubdMethodLoLbl": "Loop",
    "kExportSubdMethodNoLbl": "None (Polygonal Mesh)",
    "kExportUVSetsAnn": "Exports Maya UV Sets as USD primvars.",
    "kExportUVSetsLbl": "UV Sets",
    "kExportRelativeTexturesAnn": "Choose whether your texture files are written as relative\n" +
                                  "or absolute paths as you export them to USD. If you select\n" +
                                  "Automatic, it will be chosen for you in the exported USD file\n" +
                                  "based on what they currently are as Maya data.",
    "kExportRelativeTexturesAutomaticLbl": "Automatic",
    "kExportRelativeTexturesAbsoluteLbl": "Absolute",
    "kExportRelativeTexturesRelativeLbl": "Relative",
    "kExportRelativeTexturesLbl": "Texture File Paths:",
    "kExportVisibilityAnn": "Exports Maya visibility attributes as USD metadata.",
    "kExportVisibilityLbl": "Visibility",
    "kExportDefaultPrimLbl": "Default Prim",
    "kExportDefaultPrimNoneLbl": "None",
    "kExportDefaultPrimAnn": "As part of its metadata, each USD stage can identify a default prim.\nThis is the primitive that is referenced in if you reference in a file.",

    "kExportAxisAndUnitLbl": "Axis & Unit Conversion",
    "kExportUpAxisLbl": "Up Axis",
    "kExportUpAxisAnn": "<b></b>Select the up axis for the export file.<br/>" +
                        "Rotation will be applied if converting to a different axis.<br/>" +
                        "<b>None</b>: do not author upAxis.<br/>" +
                       "<b>Use Maya Preferences</b>: use the axis of the current scene.",
    "kExportUpAxisNoneLbl": "None",
    "kExportUpAxisMayaPrefsLbl": "Use Maya Preferences",
    "kExportUpAxisYLbl": "Y",
    "kExportUpAxisZLbl": "Z",

    "kExportUnitLbl": "Unit",
    "kExportUnitAnn": "<b></b>Select the unit for the export file." +
                      " Scaling will be applied if converting to a different unit.<br/>" +
                      "<b>None</b>: Do not scale or write out unit metadata.<br/>" +
                      "<b>Use Maya Preferences</b>: Use the unit of the current scene.<br/>",
    "kExportUnitNoneLbl": "None",
    "kExportUnitMayaPrefsLbl": "Use Maya Preferences",
    "kExportUnitMillimeterLbl": "Millimeter",
    "kExportUnitCentimeterLbl": "Centimeter",
    "kExportUnitDecimeterLbl": "Decimeter",
    "kExportUnitMeterLbl": "Meter",
    "kExportUnitKilometerLbl": "Kilometer",
    "kExportUnitInchLbl": "Inch",
    "kExportUnitFootLbl": "Foot",
    "kExportUnitYardLbl": "Yard",
    "kExportUnitMileLbl": "Mile",

    # All strings for import dialog:
    "kImportAnimationDataLbl": "Animation Data",
    "kImportCustomFrameRangeLbl": "Custom Frame Range",
    "kImportFrameRangeLbl": "Frame Range Start/End: ",
    "kImportHierarchyViewLbl": "Hierarchy View",
    "kImportJobContextAnn": "Select a loaded plug-in configuration to modify import options",
    "kImportJobContextLbl": "Plug-in Configuration:",
    "kImportJobContextNoneLbl": "None",
    "kImportMaterialConvAnn": "Select the preferred conversion method for inbound USD shading data to bind with Maya geometry",
    "kImportMaterialConvLbl": "Convert to:",
    "kImportMaterialConvNoneLbl": "Automatic",
    "kImportMaterialConvOPBRLbl": "OpenPBR Surface",
    "kImportMaterialConvSSLbl": "Standard Surface",
    "kImportMaterialConvLamLbl": "Lambert",
    "kImportMaterialConvPSLbl": "USD Preview Surface",
    "kImportMaterialConvBlnLbl": "Blinn",
    "kImportMaterialConvPhgLbl": "Phong",
    "kImportRelativeTexturesLbl": "Texture File Paths:",
    "kImportRelativeTexturesAnn": "Choose whether your texture files are written as relative or absolute paths\n" +
                                  "as you import them to Maya. If you select <b>Automatic</b>, it will be chosen for you\n" +
                                  "in the import based on what they currently are as USD data.",
    "kImportRelativeTexturesAutomaticLbl": "Automatic",
    "kImportRelativeTexturesAbsoluteLbl": "Absolute",
    "kImportRelativeTexturesRelativeLbl": "Relative",
    "kImportMaterialsAnn": "If selected, shading data is extracted from the USD file for import",
    "kImportMaterialsLbl": "Materials",
    "kImportPrimsInScopeNumLbl": "Total Number of Prims in Scope:",
    "kImportSelectFileTxt": "Please select a file to enable this option",
    "kImportScopeVariantsAnn": "Select a USD file and click <strong>Hierarchy View</strong> to build the scope of your import and switch variants.<br><br><strong>Scope</strong>: A USD file consists of prims, the primary container object in USD. Prims can contain any scene element, like meshes, lights, cameras, etc. Use the checkboxes in the <strong>Hierarchy View</strong> to select and deselect prims to build the scope of your import.<br><br><strong>Variant</strong>: A variant is a single, named variation of a variant set. Each variant set is a package of alternatives that users can switch between non-destructively. A variant set has no limits to what it can store. Variants can be used to swap out a material or change the entire hierarchy of an asset. A single prim can have many variants and variant sets, but only one variant from each variant set can be selected for import into Maya.",
    "kImportScopeVariantsLbl": "Scope and Variants: ",
    "kImportScopeVariantsSbm": "Select a USD file and click Hierarchy View to open the Hierarchy View window. This window lets you toggle prim checkboxes and non-destructively switch between variants to build the scope of your import.",
    "kImportToInstanceAnn": "If selected, instance proxies will be converted to Maya instances. Instance proxies enable parts of the USD scenegraph to share prims through instancing even though instances cannot have descendants in USD. Each instanced prim is associated with a master prim that serves as the root of the scenegraph. This hierarchy reduces the time needed to load the file. Instanced prims must be manually tagged in USD. Enabling this option will convert any USD instance proxies to Maya instances as you import them.",
    "kImportToInstanceOpt": "Convert Instance Proxies to Instances",
    "kImportUSDZTxtAnn": "When a .usdz file is chosen and this is selected, .usdz texture files are imported and copied to new files on disk. To locate these files, navigate to the current Maya workspace /sourceimages directory.",
    "kImportUSDZTxtLbl": "USDZ Texture Import",
    "kImportVariantsInScopeNumLbl": "Variants Switched in Scope:",
    "kImportPluginConfigLbl": "Plug-in Configurations",
    "kImportPluginConfigAnn": "Turn on the plug-ins you wish to include for this process.\n" +
                              "When turned on, plug-ins (like Arnold) can modify or create data to ensure compatibility.\n" +
                              "Plug-ins may have extra settings you can adjust.",
    "kImportPluginConfigButtonAnn": "Options",

    "kImportAxisAndUnit": "Axis & Unit Conversion",
    "kImportUpAxis": "Up Axis",
    "kImportUpAxisAnn": "If selected, when an up axis mismatch is detected\n" +
                        "between the imported data and your scene preferences,\n" +
                        "an automatic correction will be performed.",
    "kImportUnit": "Unit",
    "kImportUnitAnn": "If selected, when a unit mismatch is detected\n" +
                      "between the imported data and your scene preferences,\n" +
                      "an automatic correction will be performed.",
    "kImportAxisAndUnitMethod": "Method",
    # Note: initial <b> is used to force Qt to render the text as HTML.
    "kImportAxisAndUnitMethodAnn": "<b></b>Select the method for axis/unit conversions.<br/>" +
                                   "<br/>" +
                                   "<b>Rotate/Scale</b>: Rotate/Scale the stage.<br/>" +
                                   "<b>Add Parent Transform</b>: Rotate/Scale all objects within the imported data as a group<br/>" +
                                   "<b>Overwrite Maya Preferences</b>: Update Maya's axis/unit preferences based on the imported data.",
    "kImportAxisAndUnitRotateScale": "Rotate/Scale",
    "kImportAxisAndUnitAddTransform": "Add Parent Transform",
    "kImportAxisAndUnitOverwritePrefs": "Overwrite Maya Preferences",

    "kMayaDiscardEdits": "Discard Maya Edits",
    "kMayaRefDiscardEdits": "Cancel Editing as Maya Data",
    "kMayaRefDuplicateAsUsdData": "Duplicate As USD Data",
    "kMayaRefDuplicateAsUsdDataDiv": "Stages",
    "kMayaRefDuplicateAsUsdDataOptions": "Options...",

    # All strings for the USD Global Preferences:
    "kUSDPreferences": "USD: USD Preferences",
    "KSavingUSDFiles": "Save USD Files",
    "kdotusdFileFormat": ".usd File Format:",
    "kAscii": "ASCII",
    "kBinary": "Binary",
    "KViewport": "Viewport",
    "KUntexturedMode": "Untextured mode",
    "KMaterialColors": "Material Colors",
    "KDisplayColors": "Display Colors",
    "KUntexturedModeAnn": "Choose what your colors should display as in the viewport. Select from default material colors or USD display colors. Select display colors when working in USD as material colors may be inaccurate from the endless types available.",
    "kSaveLayerUsdFileFormatAnn": "Select whether the .usd file is written out in binary or ASCII. You can save a file in .usdc (binary) or .usda (ASCII) format. Manually entering a file name with an extension overrides the selection in this preference.",
    "kSaveLayerUsdFileFormatSbm": "Select whether the .usd file is written out in binary or ASCII",
    "kConfirmExistFileSave": "Confirm Save of Existing Files",
    "kSavingMayaSceneFiles": "Save Maya Scene Files",
    "kRelativePathing": "Relative Pathing",
    "kAllRelativeToggle": "All files",
    "kAllRelativeToggleAnn": "When on, all of your files will be written as relative paths.",
    "kRootLayersRelativeToSceneFile": "Root layers",
    "kRootLayersRelativeToSceneFileAnn": "When on, your USD root layer file will be written as relative paths to your Maya scene file.",
    "kSubLayersRelativeToParentLayer": "Sublayers",
    "kSubLayersRelativeToParentLayerAnn": "When on, all sublayers will be written as relative paths to their parent layers.",
    "kReferencesRelativeToEditTargetLayer": "USD References/payloads",
    "kReferencesRelativeToEditTargetLayerAnn": "When on, any USD references or payloads will be written as relative paths to their respective edit target layer.",
    "kFileDependenciesRelativeToEditTargetLayer": "File dependencies",
    "kFileDependenciesRelativeToEditTargetLayerAnn": "When on, any file dependencies, such as textures or Maya references will be written as relative paths to their respective edit target layer.",
}

def getMayaUsdString(key):
    return getPluginResource('mayaUsdPlugin', key)

def mayaUSDRegisterStrings():
    # This function is called from the equivalent MEL proc
    # with the same name. The strings registered here and all the
    # ones registered from the MEL proc can be used in either
    # MEL or python.
    registerPluginResources('mayaUsdPlugin', __mayaUSDStringResources)

def mayaUSDUnregisterStrings():
    # This function is called from the equivalent MEL proc
    # with the same name. The strings registered here and all the
    # ones registered from the MEL proc can be used in either
    # MEL or python.
    unregisterPluginResources('mayaUsdPlugin', __mayaUSDStringResources)
