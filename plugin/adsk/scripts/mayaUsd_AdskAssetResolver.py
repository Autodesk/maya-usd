import maya.cmds as cmds
from mayaUSDRegisterStrings import getMayaUsdString
from maya.OpenMaya import MGlobal
import AdskAssetResolver as ar
from pxr import Ar as pxrAr
from pathlib import Path

def get_resolver_version():
    ''' Get the version of the Autodesk Asset Resolver. '''
    try:
        adskResolver = pxrAr.GetUnderlyingResolver()
        if adskResolver is not None:
            version = ar.VersionInfo()
            arVersion = version.split("\n")[0]
            arVersion = arVersion.split(" ")[0]
            print("Autodesk Asset Resolver Version:", arVersion)
            return arVersion
    except Exception as e:
        # The Autodesk Asset Resolver is not available
        print("Error getting Autodesk Asset Resolver version:", e)
        return None

def meets_min_version(targetVersion):
    ''' Compare two version strings of the Resolver. '''
    def version_tuple(v):
        return tuple(map(int, (v.split("."))))

    currentVersion = get_resolver_version()
    if currentVersion is None:
        return False

    return (version_tuple(currentVersion) >= version_tuple(targetVersion))

def include_maya_project_tokens():
    ''' Include Maya project tokens in the USD Asset Resolver. '''
    directory = cmds.workspace(q=True, fullName=True)
    tokenList = cmds.workspace(fileRuleList=True)
    mayaUsdResolver = ar.AssetResolverContextDataRegistry.RegisterContextData("MayaUSDExtension")
    if mayaUsdResolver is not None:
        mayaUsdResolver.AddStaticToken("Project", directory)
        for token in tokenList:
            tokenValue = cmds.workspace(fileRuleEntry=token)
            mayaUsdResolver.AddStaticToken(token, tokenValue)
