import collections
import re
import ufe
import mayaUsd.ufe as mayaUsdUfe
from pxr import Usd, UsdShade, Sdr


class AEShaderLayout(object):
    '''
    This class takes care of sorting attributes that use uifolder and uiorder metadata and to prepare
    a data structure that has the exact sequence of layouts and attributes necessary to properly
    lay out all the attributes in the right folders.
    '''
    # A named tuple for attribute information:
    _AttributeInfo = collections.namedtuple("_AttributeInfo", ["uiorder", "name", "uifolder"])
    # A string splitter for the most commonly used separators:
    _groupSplitter = re.compile(r"[/|\;:]")
    # Valid uiorder:
    _isDecimal = re.compile("^[0-9]+$")

    class Group(object):
        """Base class to return layout information. The list of items can contain subgroups."""
        def __init__(self, name):
            self._name = name
            self._items = []
        @property
        def name(self):
            return self._name
        @property
        def items(self):
            return self._items

    def __init__(self, ufeSceneItem):
        self.item = ufeSceneItem
        self.prim = mayaUsdUfe.ufePathToPrim(ufe.PathString.string(self.item.path()))
        self._attributeInfoList = []
        self._canCompute = True
        self._attributeLayout = AEShaderLayout.Group(self.item.nodeType())
        if ufeSceneItem.nodeType() == "Shader":
            self.parseShaderAttributes()
        else:
            self.parseNodeGraphAttributes()

    def parseShaderAttributes(self):
        """Parse shader attributes using the Sdr node and property APIs."""
        if not hasattr(ufe, "NodeDef"):
            # Ufe does not yet have unauthored attribute support. We can compute, but it won't help.
            self._canCompute = False
            return
        shader = UsdShade.Shader(self.prim)
        nodeId = shader.GetIdAttr().Get()
        if not nodeId:
            self._canCompute = False
            return
        nodeDef = Sdr.Registry().GetShaderNodeByIdentifier(nodeId)
        if not nodeDef:
            self._canCompute = False
            return
        label = nodeDef.GetLabel()
        if not label:
            label = nodeDef.GetFamily()

        self._attributeLayout = AEShaderLayout.Group(self._attributeLayout.name + ": " + mayaUsdUfe.prettifyName(label))

        # Best option: Use ordering metadata found the Sdr properties:
        hasMetadataOrdering = False
        if Usd.GetVersion() < (0, 25, 5):
            inputNames = nodeDef.GetInputNames()
        else:
            inputNames = nodeDef.GetShaderInputNames()

        for inputName in inputNames:
            input = nodeDef.GetShaderInput(inputName)
            metadata = input.GetHints()
            metadata.update(input.GetMetadata())
            if "uiorder" in metadata or "uifolder" in metadata:
                hasMetadataOrdering = True
                break

        if hasMetadataOrdering:
            # Prefer metadata over GetPages. The metadata can contain subgroups.
            unorderedIndex = 10000
            for inputName in inputNames:
                input = nodeDef.GetShaderInput(inputName)
                metadata = input.GetHints()
                metadata.update(input.GetMetadata())
                uiorder = metadata.get("uiorder", "").strip()
                if AEShaderLayout._isDecimal.match(uiorder):
                    uiorder = int(uiorder)
                else:
                    uiorder = unorderedIndex
                    unorderedIndex += 1
                uifolder = metadata.get("uifolder", "").strip()
                self._attributeInfoList.append(
                    AEShaderLayout._AttributeInfo(uiorder,
                                                  UsdShade.Utils.GetFullName(inputName, UsdShade.AttributeType.Input),
                                                  uifolder))
            return

        # Second best option: Use page information populated in the Sdr node at shader discovery time.
        pages = nodeDef.GetPages()
        if len(pages) == 1 and not pages[0]:
            pages = []

        if not pages:
            # Worst case: Flat layout
            for name in inputNames:
                self._attributeLayout.items.append(UsdShade.Utils.GetFullName(name, UsdShade.AttributeType.Input))
            return

        for page in pages:
            pageLabel = page
            if not page:
                pageLabel = 'Extra Shader Attributes'
            group = AEShaderLayout.Group(pageLabel)
            for name in nodeDef.GetPropertyNamesForPage(page):
                if Usd.GetVersion() < (0, 25, 5):
                    nodeInput = nodeDef.GetInput(name)
                else:
                    nodeInput = nodeDef.GetShaderInput(name)

                if nodeInput:
                    name = UsdShade.Utils.GetFullName(name, UsdShade.AttributeType.Input)
                    group.items.append(name)
            if group.items:
                self._attributeLayout.items.append(group)

    def parseNodeGraphAttributes(self):
        """NodeGraph and Material do not have associated Sdr information, but some layouting metadata
           might have been added by shader graph editors."""
        nodegraph = UsdShade.NodeGraph(self.prim)
        if not nodegraph:
            self._canCompute = False
            return
        # This should make sure items without uiorder appear at the end. Still no guarantee since
        # the user can use any numbering sequence he wants.
        unorderedIndex = 10000
        for input in nodegraph.GetInputs():
            uiorder = input.GetSdrMetadataByKey("uiorder").strip()
            if AEShaderLayout._isDecimal.match(uiorder):
                uiorder = int(uiorder)
            else:
                uiorder = unorderedIndex
                unorderedIndex += 1
            uifolder = input.GetSdrMetadataByKey("uifolder").strip()
            self._attributeInfoList.append(AEShaderLayout._AttributeInfo(uiorder, input.GetFullName(), uifolder))

    def get(self):
        '''Computes the layout based on metadata ordering if an info list is present. If the list
           was computed in a different way, the attribute info list will be empty, and we return the
           computed attributeLayout unchanged.'''
        if not self._canCompute:
            return None

        self._attributeInfoList.sort()
        folderIndex = {(): self._attributeLayout}

        for attributeInfo in self._attributeInfoList:
            groups = tuple()
            if attributeInfo.uifolder:
                groups = tuple(AEShaderLayout._groupSplitter.split(attributeInfo.uifolder))
                # Ensure the parent groups are there 
                for i in range(len(groups)):
                    subgroup = groups[0:i+1]
                    if subgroup not in folderIndex:
                        newGroup = AEShaderLayout.Group(subgroup[-1])
                        folderIndex[subgroup[0:i]].items.append(newGroup)
                        folderIndex[subgroup] = newGroup
            # Add the attribute to the group
            folderIndex[groups].items.append(attributeInfo.name)
        return self._attributeLayout