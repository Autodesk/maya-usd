import maya.cmds as cmds
from mayaUSDRegisterStrings import getMayaUsdString
from maya.OpenMaya import MGlobal
import AdskAssetResolver as ar
from pxr import Ar as pxrAr
import os

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

def load_mappingfile(mappingFilePath=None):
    if mappingFilePath is not None:
        os.environ["ADSK_AR_MAPPING_FILE"] = mappingFilePath
    try:
        adskResolver = pxrAr.GetUnderlyingResolver()
        ctx = adskResolver.GetCurrentContext()
        ctx = ar.AdskResolverContext()
        adskResolver.RefreshContext(ctx)
    
    except Exception as e:
        MGlobal.displayError(f"Error loading mapping file: {e}")
        return
