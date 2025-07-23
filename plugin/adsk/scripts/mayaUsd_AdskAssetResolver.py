import maya.cmds as cmds
import mayaUsd.lib
import re
from mayaUSDRegisterStrings import getMayaUsdString
from maya.OpenMaya import MGlobal
import AdskAssetResolver as ar

def include_maya_project_tokens():
    ''' Include Maya project tokens in the USD Asset Resolver. '''
    directory = cmds.workspace(q=True, directory=True)
    MGlobal.displayInfo("Root project directory: {}".format(directory))
    tokenList = cmds.workspace(fileRuleList=True)
    MGlobal.displayInfo("Including Maya project tokens in the USD Asset Resolver: {}".format(tokenList))
    mayaUsdResolver = ar.AssetResolverContextDataManager.RegisterContextData("MayaUSDExtension")
    if mayaUsdResolver is not None:
        for token in tokenList:
            tokenValue = cmds.workspace(fileRuleEntry=token)
            MGlobal.displayInfo("Token: {} = {}".format(token, tokenValue))
            mayaUsdResolver.AddStaticToken(token, tokenValue)

def unload_maya_project_tokens():
    ''' Unload Maya project tokens from the USD Asset Resolver. '''
    #mayaUsdResolver = ar.AssetResolverContextDataManager.GetContextData("MayaUSDExtension")
    #if mayaUsdResolver is not None:
    ar.AssetResolverContextDataManager.RemoveContextData("MayaUSDExtension")