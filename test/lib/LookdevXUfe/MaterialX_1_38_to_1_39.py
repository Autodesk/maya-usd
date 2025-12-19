import argparse
import re
from pxr import Usd, UsdMtlx, Sdf, UsdShade

CHANNEL_INDEX_MAP = {
        'r': 0, 'g': 1, 'b': 2, 'a': 3,
        'x': 0, 'y': 1, 'z': 2, 'w': 3 
    }

CHANNEL_CONSTANT_MAP = {
        '0': 0.0, '1': 1.0
    }
CHANNEL_COUNT_MAP = {
        "float": 1,
        "color3": 3, "color4": 4,
        "vector2": 2, "vector3": 3, "vector4": 4
    }
CHANNEL_CONVERT_PATTERNS = (
        ( "rgb", 3 ), ( "rgb1", 4 ), ( "rgba", 4 ),
        ( "xyz", 3 ), ( "xyz1", 4 ), ( "xyzw", 4 ),
        ( "rr", 1 ), ( "rrr", 1 ),
        ( "xx", 1 ), ( "xxx", 1 )
    )
CHANNEL_ATTRIBUTE_PATTERNS = (
        ( { "xx", "xxx", "xxxx" }, "float" ),
        ( { "xyz", "x", "y", "z" }, "vector3" ),
        ( { "rgba", "a" }, "color4" )
    )
SWITCH_NODES = {
        "ND_switch_float",
        "ND_switch_color3",
        "ND_switch_color4",
        "ND_switch_vector2",
        "ND_switch_vector3",
        "ND_switch_vector4",
        "ND_switch_floatI",
        "ND_switch_color3I",
        "ND_switch_color4I",
        "ND_switch_vector2I",
        "ND_switch_vector3I",
        "ND_switch_vector4I",
    }
SWIZZLE_NODES = {
        "ND_swizzle_float_color3",
        "ND_swizzle_float_color4",
        "ND_swizzle_float_vector2",
        "ND_swizzle_float_vector3",
        "ND_swizzle_float_vector4",
        "ND_swizzle_color3_float",
        "ND_swizzle_color3_color3",
        "ND_swizzle_color3_color4",
        "ND_swizzle_color3_vector2",
        "ND_swizzle_color3_vector3",
        "ND_swizzle_color3_vector4",
        "ND_swizzle_color4_float",
        "ND_swizzle_color4_color3",
        "ND_swizzle_color4_color4",
        "ND_swizzle_color4_vector2",
        "ND_swizzle_color4_vector3",
        "ND_swizzle_color4_vector4",
        "ND_swizzle_vector2_float",
        "ND_swizzle_vector2_color3",
        "ND_swizzle_vector2_color4",
        "ND_swizzle_vector2_vector2",
        "ND_swizzle_vector2_vector3",
        "ND_swizzle_vector2_vector4",
        "ND_swizzle_vector3_float",
        "ND_swizzle_vector3_color3",
        "ND_swizzle_vector3_color4",
        "ND_swizzle_vector3_vector2",
        "ND_swizzle_vector3_vector3",
        "ND_swizzle_vector3_vector4",
        "ND_swizzle_vector4_float",
        "ND_swizzle_vector4_color3",
        "ND_swizzle_vector4_color4",
        "ND_swizzle_vector4_vector2",
        "ND_swizzle_vector4_vector3",
        "ND_swizzle_vector4_vector4",
    }
SWIZZLE_RE = re.compile("ND_swizzle_([^_]+)_([^_]+)")
MX_TO_USD_TYPE = {
        "float": Sdf.ValueTypeNames.Float,
        "vector2": Sdf.ValueTypeNames.Float2,
        "vector3": Sdf.ValueTypeNames.Float3,
        "vector4": Sdf.ValueTypeNames.Float4,
        "color3": Sdf.ValueTypeNames.Color3f,
        "color4": Sdf.ValueTypeNames.Color4f,
        "integer": Sdf.ValueTypeNames.Int,
        "boolean":  Sdf.ValueTypeNames.Bool,
        "filename": Sdf.ValueTypeNames.Asset,
        "string": Sdf.ValueTypeNames.String,
    }
THIN_FILM_BSDF = {
        "ND_dielectric_bsdf",
        "ND_conductor_bsdf",
        "ND_generalized_schlick_bsdf"
    }

SUFFIX_SPLIT_RE = re.compile("(.*[^0-9])([0-9]+)$")

def splitNumericalSuffix(srcName):
    # Compiled regular expression to find a numerical suffix to a path component.
    # It searches for any number of characters followed by a single non-numeric,
    # then one or more digits at end of string.
    suffixMatch = SUFFIX_SPLIT_RE.match(srcName)
    if suffixMatch:
        return suffixMatch.groups()
    return (srcName, "")

def uniqueName(existingNames, srcName):
    base, suffixStr = splitNumericalSuffix(srcName)
    suffix = 1
    suffixLen = 1
    if suffixStr:
        suffix = int(suffixStr) + 1
        suffixLen = len(suffixStr)
    # Create a suffix string from the number keeping the same number of digits as
    # numerical suffix from input srcName (padding with 0's if needed).
    suffixStr = str(suffix)
    suffixStr = "0" * (suffixLen - min(suffixLen, len(suffixStr))) + suffixStr
    dstName = base + suffixStr
    while dstName in existingNames:
        suffix += 1
        suffixStr = str(suffix)
        suffixStr = "0" * (suffixLen - min(suffixLen, len(suffixStr))) + suffixStr
        dstName = base + suffixStr
    return dstName

def uniqueChildName(usdParent, name):
    if not usdParent.IsValid():
        return ""
    # The prim GetChildren method used the UsdPrimDefaultPredicate which includes
    # active prims. We also need the inactive ones.
    #
    # const Usd_PrimFlagsConjunction UsdPrimDefaultPredicate =
    #			UsdPrimIsActive && UsdPrimIsDefined &&
    #			UsdPrimIsLoaded && !UsdPrimIsAbstract;
    # Note: removed 'UsdPrimIsLoaded' from the predicate. When it is present the
    #		 filter doesn't properly return the inactive prims. UsdView doesn't
    #		 use loaded either in _computeDisplayPredicate().
    # Note: removed 'UsdPrimIsAbstract' from the predicate since when naming
    #       we want to consider all the prims (even if hidden) to generate a real
    #       unique sibling.
    #
    # Note: our UsdHierarchy uses instance proxies, so we also use them here.
    childrenNames = set([i.GetName() for i in usdParent.GetFilteredChildren(Usd.TraverseInstanceProxies())])
    return uniqueName(childrenNames, name)

def CreateSiblingNode(node, shaderId, name):
    ngPrim = node.GetPrim().GetParent()
    # childOrder = ngPrim.GetAllChildrenNames()
    newNode = UsdShade.Shader.Define(ngPrim.GetStage(), ngPrim.GetPath().AppendChild(uniqueChildName(ngPrim, name)))
    newNode.SetShaderId(shaderId)
    # TODO: Find a way to make sure the sibling appears in the right order in the outliner.
    #       There is a Usd.Prim.SetChildrenReorder(), but it is only metadata applied
    #       after the fact. We would like a more direct approach.
    # childOrder.insert(childOrder.index(node.GetPrim().GetName()) + 1, newNode.GetPrim().GetName())
    # ngPrim.SetChildrenReorder(childOrder)
    return newNode

def GetUpstreamNodes(startNode, idFilter, visited):
    """Recursively traverse connections of startNode trying to
       find nodes with ID in idFilter set."""
    retVal = set()
    for input in startNode.GetInputs():
        for sourceInfo in startNode.GetConnectedSources(input)[0]:
            source = sourceInfo.source
            sourcePrim = source.GetPrim()
            if sourcePrim in visited:
                continue
            visited.add(sourcePrim)
            sourceShader = UsdShade.Shader(sourcePrim)
            if sourceShader and sourceShader.GetShaderId() in idFilter:
                retVal.add(source)
                
            retVal.update(GetUpstreamNodes(source, idFilter, visited))
    
    return retVal
    
def GetDownstreamPorts(node):
    """Returns a list(node, port) of all items that connect to the output port of
        the node passed in as parameter.
        
        We assume a nicely behaved graph without connections teleporting
        across NodeGraph boundaries."""
    retVal = []
    ng = UsdShade.NodeGraph(node.GetPrim().GetParent())
    if not ng:
        return retVal
    # Look for NodeGraph connections:
    for output in ng.GetOutputs():
        for sourceInfo in ng.ConnectableAPI().GetConnectedSources(output)[0]:
            if sourceInfo.source.GetPrim() == node.GetPrim():
                retVal.append((ng.ConnectableAPI(), output))
    # Check every node inside the graph:
    for child in ng.GetPrim().GetChildren():
        shader = UsdShade.Shader(child)
        if not shader:
            continue
        for input in shader.GetInputs():
            for sourceInfo in shader.ConnectableAPI().GetConnectedSources(input)[0]:
                if sourceInfo.source.GetPrim() == node.GetPrim():
                    retVal.append((shader.ConnectableAPI(), input))
    return retVal

def MoveInput(sourceNode, sourceInputName, destNode, destInputName):
    editor = Usd.NamespaceEditor(sourceNode.GetPrim().GetStage())
    editor.ReparentProperty(sourceNode.GetInput(sourceInputName).GetAttr(), destNode.GetPrim(), "inputs:" + destInputName)
    if editor.CanApplyEdits():
        editor.ApplyEdits()
    else:
        print("WARNING: Failed to move input '{}' from node '{}' to input '{}' on node '{}'. Please make sure "
            "the material layer is writable.".format(
            sourceInputName,
            sourceNode.GetPrim().GetPath(),
            destInputName,
            destNode.GetPrim().GetPath()))

def ConvertMaterialTo139(usdMaterial):
    # If this material is already 1.39, then nothing to do:
    if usdMaterial.HasAPI(UsdMtlx.MaterialXConfigAPI):
        configAPI = UsdMtlx.MaterialXConfigAPI(usdMaterial)
        versionAttr = configAPI.GetConfigMtlxVersionAttr()
        if versionAttr.Get() > "1.38":
            return

    # If we had nothing to upgrade, leave it as 1.38. The
    # DCC may choose later to tag it as 1.39 if editing using
    # a 1.39 library.
    hasUpgrade = False
        
    # Build list of nodes upfront since we will be adding
    # nodes mid-flight which might throw off iterators:
    allNodes = []
    toVisit = [usdMaterial,]
    visited = set()
    toVisit.append(usdMaterial)
    while toVisit:
        node = toVisit.pop()
        if node in visited:
            continue
        visited.add(node)
        nodeGraph = UsdShade.NodeGraph(node)
        if nodeGraph:
            toVisit += list(node.GetChildren())
            continue
        shader = UsdShade.Shader(node)
        if shader:
            allNodes.append((shader.GetPrim().GetPath(),shader))
    
    # Not necessary, but allows Python and C++ to process nodes in the exact same order, thus producing comparable output.
    allNodes.sort()

    # No need to look for "channels" as this feature was never supported in USD.

    # Update all nodes.
    unusedNodes = []
    for _, node in allNodes:
        shaderID = node.GetShaderId()
        if shaderID == "ND_layer_bsdf":
            # Convert layering of thin_film_bsdf nodes to thin-film parameters on the affected BSDF nodes.
            if not node.GetPrim().HasAttribute("inputs:top") or not node.GetPrim().HasAttribute("inputs:base"):
                continue
            
            topSource = node.ConnectableAPI().GetConnectedSource(node.GetInput("top"))
            baseSource = node.ConnectableAPI().GetConnectedSource(node.GetInput("base"))
            if topSource and baseSource and UsdShade.Shader(topSource[0]).GetShaderId() == "ND_thin_film_bsdf":
                # Apply thin-film parameters to all supported BSDF's upstream.
                topSource = topSource[0]
                baseSource = baseSource[0]
                for upstream in GetUpstreamNodes(node.ConnectableAPI(), THIN_FILM_BSDF, set()):
                    if not upstream.GetPrim().HasAttribute("inputs:scatter_mode") or upstream.GetPrim().GetAttribute("inputs:scatter_mode").Get() != "T":
                        topSource.GetInput("thickness").GetAttr().FlattenTo(upstream.GetPrim(), "inputs:thinfilm_thickness")
                        topSource.GetInput("ior").GetAttr().FlattenTo(upstream.GetPrim(), "inputs:thinfilm_ior")

                # Bypass the thin-film layer operator.
                for downstreamNode, downstreamPort in GetDownstreamPorts(node):
                    downstreamNode.DisconnectSource(downstreamPort, node.GetOutput("out"))
                    downstreamPort.ConnectToSource(baseSource.GetOutput("out"))

                # Mark original nodes as unused.
                unusedNodes.append(node)
                unusedNodes.append(topSource)
                hasUpgrade = True
        elif shaderID == "ND_subsurface_bsdf":
            radiusInput = node.GetInput("radius")
            if radiusInput and radiusInput.GetTypeName() == MX_TO_USD_TYPE["vector3"]:
                convertNode = CreateSiblingNode(node, "ND_convert_vector3_color3", "convert")
                MoveInput(node, "radius", convertNode, "in")
                radiusInput = node.CreateInput("radius", MX_TO_USD_TYPE["color3"])
                radiusInput.ConnectToSource(convertNode.CreateOutput("out", MX_TO_USD_TYPE["color3"]))
                hasUpgrade = True
        elif shaderID in SWITCH_NODES:
            # Upgrade switch nodes from 5 to 10 inputs, handling the fallback behavior for
            # constant "which" values that were previously out of range.
            which = node.GetInput("which")
            if which and which.GetAttr().HasAuthoredValue():
                if which.Get() >= 5:
                    which.Set(0)
                    hasUpgrade = True
        elif shaderID in SWIZZLE_NODES:
            inInput = node.GetInput("in")
            swizzleMatch = SWIZZLE_RE.match(shaderID)
            
            if not swizzleMatch:
                continue
                
            sourceType, destType = swizzleMatch.groups()
            if sourceType not in CHANNEL_COUNT_MAP or destType not in CHANNEL_COUNT_MAP:
                continue

            channelsInput = node.GetInput("channels")
            channelString = channelsInput.Get() if channelsInput else ""
            sourceChannelCount = CHANNEL_COUNT_MAP[sourceType]
            destChannelCount = CHANNEL_COUNT_MAP[destType]

            # We convert to a constant node if "in" input is a constant value:
            convertToConstantNode = not inInput or (inInput.GetAttr().HasAuthoredValue() and not inInput.HasConnectedSource())
            # We also convert to a constant node if every destination
            # channel is constant:
            #   eg: "ND_swizzle_color3_color3" node with
            #       "010" in the "channels" input.
            if not convertToConstantNode:
                convertToConstantNode = True
                for i in range(destChannelCount):
                    if i < len(channelString):
                        if channelString[i] in CHANNEL_CONSTANT_MAP:
                            # Still in constant territory:
                            continue
                    # Every other scenario: not constant
                    convertToConstantNode = False
                    break

            if convertToConstantNode:
                node.SetShaderId("ND_constant_" + destType)
                if inInput:
                    if sourceType == "float":
                        valueTuple = (inInput.Get(),)
                    else:
                        valueTuple = tuple(inInput.Get())
                else:
                    valueTuple = tuple([0,] * CHANNEL_COUNT_MAP[sourceType])
                newValue = []
                for i in range(destChannelCount):
                    if i < len(channelString):
                        if channelString[i] in CHANNEL_INDEX_MAP:
                            index = CHANNEL_INDEX_MAP[channelString[i]]
                            if index < len(valueTuple):
                                newValue.append(valueTuple[index])
                                continue
                        elif channelString[i] in CHANNEL_CONSTANT_MAP:
                            newValue.append(CHANNEL_CONSTANT_MAP[channelString[i]])
                            continue
                    # Invalid channel name, or missing channel name:
                    newValue.append(valueTuple[0])

                valueInput = node.CreateInput("value", MX_TO_USD_TYPE[destType])
                if destType == "float":
                    valueInput.Set(newValue[0])
                else:
                    valueInput.Set(tuple(newValue))

                if inInput:
                    editor = Usd.NamespaceEditor(usdMaterial.GetStage())
                    editor.DeleteProperty(inInput.GetAttr())
                    if editor.CanApplyEdits():
                        editor.ApplyEdits()
                    else:
                        print("WARNING: Failed to remove 'in' input from node '{}'. Please make sure "
                            "the material layer is writable.".format(
                            node.GetPrim().GetPath()))
                hasUpgrade = True

            elif destChannelCount == 1:
                # Replace swizzle with extract.
                node.SetShaderId(f"ND_extract_{sourceType}")
                if channelString and channelString[0] in CHANNEL_INDEX_MAP:
                    node.CreateInput("index", MX_TO_USD_TYPE["integer"]).Set(CHANNEL_INDEX_MAP[channelString[0]])
                hasUpgrade = True

            elif sourceType != destType and (channelString, sourceChannelCount) in CHANNEL_CONVERT_PATTERNS:
                # Replace swizzle with convert.
                node.SetShaderId(f"ND_convert_{sourceType}_{destType}")
                hasUpgrade = True

            elif sourceChannelCount == 1:
                # Replace swizzle with combine.
                node.SetShaderId(f"ND_combine{destChannelCount}_{destType}")
                for i in range(destChannelCount):
                    if i < len(channelString) and channelString[i] in CHANNEL_CONSTANT_MAP:
                        combineInInput = node.CreateInput(f"in{i+1}", MX_TO_USD_TYPE["float"])
                        combineInInput.Set(CHANNEL_CONSTANT_MAP[channelString[i]])
                    else:
                        inInput.GetAttr().FlattenTo(node.GetPrim(), f"inputs:in{i+1}")
                editor = Usd.NamespaceEditor(usdMaterial.GetStage())
                editor.DeleteProperty(inInput.GetAttr())
                if editor.CanApplyEdits():
                    editor.ApplyEdits()
                else:
                    print("WARNING: Failed to remove 'in' input from node '{}'. Please make sure "
                        "the material layer is writable.".format(
                        node.GetPrim().GetPath()))
                hasUpgrade = True

            else:
                # Replace swizzle with separate and combine.
                separateNode = CreateSiblingNode(node, f"ND_separate{sourceChannelCount}_{sourceType}", "separate")

                node.SetShaderId(f"ND_combine{destChannelCount}_{destType}")
                for i in range(destChannelCount):
                    combineInInput = node.CreateInput(f"in{i+1}", MX_TO_USD_TYPE["float"])
                    if i < len(channelString):
                        if channelString[i] in CHANNEL_INDEX_MAP:
                            combineInInput.ConnectToSource(separateNode.CreateOutput(f"out{channelString[i]}", MX_TO_USD_TYPE["float"]))
                            continue
                        elif channelString[i] in CHANNEL_CONSTANT_MAP:
                            combineInInput.Set(CHANNEL_CONSTANT_MAP[channelString[i]])
                            continue
                    # Invalid channel name, or missing channel name:
                    separateOutputName = "outr" if sourceType in ("color3", "color4") else "outx"
                    combineInInput.ConnectToSource(separateNode.CreateOutput(separateOutputName, MX_TO_USD_TYPE["float"]))
                MoveInput(node, inInput.GetBaseName(), separateNode, "in")
                hasUpgrade = True

            # Remove the channels input from the converted node.
            if channelsInput:
                editor = Usd.NamespaceEditor(usdMaterial.GetStage())
                editor.DeleteProperty(channelsInput.GetAttr())
                if editor.CanApplyEdits():
                    editor.ApplyEdits()
                else:
                    print("WARNING: Failed to remove 'channels' input from node '{}'. Please make sure "
                        "the material layer is writable.".format(
                        node.GetPrim().GetPath()))

        elif shaderID in ("ND_atan2_float", "ND_atan2_vector2",
                          "ND_atan2_vector3", "ND_atan2_vector4"):
            editor = Usd.NamespaceEditor(usdMaterial.GetStage())
            input1 = node.GetInput("in1")
            if input1:
                input1.GetAttr().FlattenTo(node.GetPrim(), "inputs:iny")
                editor.DeleteProperty(input1.GetAttr())
                if editor.CanApplyEdits():
                    editor.ApplyEdits()
                else: 
                    print("WARNING: Failed to remove 'in1' input from node '{}'. Please make sure "
                        "the material layer is writable.".format(
                        node.GetPrim().GetPath()))
                hasUpgrade = True
            input2 = node.GetInput("in2")
            if input2:
                input2.GetAttr().FlattenTo(node.GetPrim(), "inputs:inx")
                editor.DeleteProperty(input2.GetAttr())
                if editor.CanApplyEdits():
                    editor.ApplyEdits()
                else: 
                    print("WARNING: Failed to remove 'in2' input from node '{}'. Please make sure "
                        "the material layer is writable.".format(
                        node.GetPrim().GetPath()))
                hasUpgrade = True

        elif shaderID in ("ND_normalmap", "ND_normalmap_vector2"):
            space = node.GetInput("space")
            if space and space.Get() == "object":
                # Replace object-space normalmap with a graph.
                multiply = CreateSiblingNode(node, "ND_multiply_vector3FA", "multiply")
                MoveInput(node, "in", multiply, "in1")
                multiply.CreateInput("in2", MX_TO_USD_TYPE["float"]).Set(2)
                subtract = CreateSiblingNode(node, "ND_subtract_vector3FA", "subtract")
                subtract.CreateInput("in1", MX_TO_USD_TYPE["vector3"]).ConnectToSource(multiply.CreateOutput("out", MX_TO_USD_TYPE["vector3"]))
                subtract.CreateInput("in2", MX_TO_USD_TYPE["float"]).Set(1)
                node.SetShaderId("ND_normalize_vector3")
                for input in list(node.GetInputs()):
                    editor = Usd.NamespaceEditor(usdMaterial.GetStage())
                    editor.DeleteProperty(input.GetAttr())
                    if editor.CanApplyEdits():
                        editor.ApplyEdits()
                    else:
                        print("WARNING: Failed to remove '{}' input from node '{}'. Please make sure "
                            "the material layer is writable.".format(
                            input.GetBaseName(),
                            node.GetPrim().GetPath()))
                node.CreateInput("in", MX_TO_USD_TYPE["vector3"]).ConnectToSource(subtract.CreateOutput("out", MX_TO_USD_TYPE["vector3"]))
                hasUpgrade = True

            else:
                # Clear tangent-space input.
                editor = Usd.NamespaceEditor(usdMaterial.GetStage())
                editor.DeleteProperty(space.GetAttr())
                if editor.CanApplyEdits():
                    editor.ApplyEdits()
                else:
                    print("WARNING: Failed to remove 'space' input from node '{}'. Please make sure "
                        "the material layer is writable.".format(
                        node.GetPrim().GetPath()))

                # If the normal or tangent inputs are set and the bitangent input is not, 
                # the bitangent should be set to normalize(cross(N, T))
                normalInput = node.GetInput("normal")
                tangentInput = node.GetInput("tangent")
                bitangentInput = node.GetInput("bitangent")
                if (normalInput or tangentInput) and not bitangentInput:
                    ngPrim = node.GetPrim().GetParent()
                    crossNode = CreateSiblingNode(node, "ND_crossproduct_vector3", "normalmap_cross")
                    node.GetInput("normal").GetAttr().FlattenTo(crossNode.GetPrim(), "inputs:in1")
                    node.GetInput("tangent").GetAttr().FlattenTo(crossNode.GetPrim(), "inputs:in2")

                    normalizeNode = CreateSiblingNode(node, "ND_normalize_vector3", "normalmap_cross_norm")
                    normalizeNode.CreateInput("in", MX_TO_USD_TYPE["vector3"]).ConnectToSource(crossNode.CreateOutput("out", MX_TO_USD_TYPE["vector3"]))
                    node.CreateInput("bitangent", MX_TO_USD_TYPE["vector3"]).ConnectToSource(normalizeNode.CreateOutput("out", MX_TO_USD_TYPE["vector3"]))


                node.SetShaderId("ND_normalmap_float")
                hasUpgrade = True
    
    editor = Usd.NamespaceEditor(usdMaterial.GetStage())
    for node in unusedNodes:
        editor.DeletePrim(node.GetPrim())
        if editor.CanApplyEdits():
            editor.ApplyEdits()
        else:
            print("WARNING: Failed to remove unused node '{}'. Please make sure "
                "the material layer is writable.".format(
                node.GetPrim().GetPath()))

    if hasUpgrade:
        usdMaterial.ApplyAPI(UsdMtlx.MaterialXConfigAPI)
        configAPI = UsdMtlx.MaterialXConfigAPI(usdMaterial)
        versionAttr = configAPI.CreateConfigMtlxVersionAttr()
        versionAttr.Set("1.39")

    return hasUpgrade

if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument("input")
    parser.add_argument("output")
    parser.add_argument('--path', dest='paths', action='append', nargs='+', help='SdfPath to process. Can be called multiple times')

    args = parser.parse_args()

    stage = Usd.Stage.Open(args.input)

    if args.paths:
        for sdfPath in args.paths:
            prim = stage.GetPrimAtPath(sdfPath)
            if prim and prim.IsA(UsdShade.Material):
                ConvertMaterialTo139(prim)
    else:
        # Traverse stage:
        for prim in [x for x in stage.Traverse() if x.IsA(UsdShade.Material)]:
            ConvertMaterialTo139(prim)

    stage.GetRootLayer().Export(args.output)
