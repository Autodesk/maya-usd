import maya.cmds as cmds
import mayaUsd.lib
import re
from mayaUSDRegisterStrings import getMayaUsdString
from maya.OpenMaya import MGlobal
import AdskAssetResolver as ar

def include_maya_project_tokens():
    ''' Include Maya project tokens in the USD Asset Resolver. '''
    directory = cmds.workspace(q=True, rd=True)
    tokenList = cmds.workspace(fileRuleList=True)
    mayaUsdResolver = ar.AssetResolverContextDataManager.RegisterContextData("MayaUSDExtension")
    if mayaUsdResolver is not None:
        mayaUsdResolver.AddStaticToken("Project", directory)
        for token in tokenList:
            tokenValue = cmds.workspace(fileRuleEntry=token)
            mayaUsdResolver.AddStaticToken(token, tokenValue)

def unload_maya_project_tokens():
    ''' Unload Maya project tokens from the USD Asset Resolver. '''
    ar.AssetResolverContextDataManager.RemoveContextData("MayaUSDExtension")