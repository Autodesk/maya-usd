import maya.cmds as _cmds
import maya.api.OpenMaya as _om
import mayaUsd.ufe as _ufe

from mayaUsdLibRegisterStrings import getMayaUsdLibString as _getMayaUsdLibString

from pxr import UsdGeom as _UsdGeom


def convertUpAxisAndUnit(shapeNode, convertUpAxis, convertUnit, conversionMethod):
    '''
    Edit the USD stage associated with the given Maya stage proxy node to
    convert the up-axis or the units used to match what is in the USD file
    with what Maya is using.
    '''
    # If neither up-axis nor unit are requested to be modified, return immediately.
    conversionInfo = StageConversionInfo(shapeNode, convertUpAxis, convertUnit)
    if not conversionInfo.needUnitsConversion and not conversionInfo.needUpAxisConversion:
        return

    resultMsg = _getMayaUsdLibString("kStageConversionSuccessful")

    if conversionMethod.lower() == 'rotatescale':
        convertUpAxisAndUnitByModifyingStage(conversionInfo)
    elif conversionMethod.lower() == 'overwriteprefs':
        convertUpAxisAndUnitByModifyingPrefs(conversionInfo)
    else:
        resultMsg = _getMayaUsdLibString("kStageConversionUnknownMethod") % conversionMethod

    print(resultMsg)


class StageConversionInfo:
    '''
    Analyze the contents of the USD file and compare it to the Maya settings
    to determine what actions need to be done to match them.
    '''

    @staticmethod
    def _isMayaUpAxisZ():
        return _cmds.upAxis(query=True, axis=True).lower() == 'z'
    
    @staticmethod
    def _isUsdUpAxisZ(stage):
        return _UsdGeom.GetStageUpAxis(stage).lower() == 'z'
    
    _mayaToMetersPerUnit = {
        _om.MDistance.kInches        : _UsdGeom.LinearUnits.inches,
        _om.MDistance.kFeet          : _UsdGeom.LinearUnits.feet,
        _om.MDistance.kYards         : _UsdGeom.LinearUnits.yards,
        _om.MDistance.kMiles         : _UsdGeom.LinearUnits.miles,
        _om.MDistance.kMillimeters   : _UsdGeom.LinearUnits.millimeters,
        _om.MDistance.kCentimeters   : _UsdGeom.LinearUnits.centimeters,
        _om.MDistance.kKilometers    : _UsdGeom.LinearUnits.kilometers,
        _om.MDistance.kMeters        : _UsdGeom.LinearUnits.meters,
    }

    @staticmethod
    def _convertMayaUnitToMetersPerUnit(mayaUnits):
        if mayaUnits not in StageConversionInfo._mayaToMetersPerUnit:
            return _UsdGeom.LinearUnits.centimeters
        return StageConversionInfo._mayaToMetersPerUnit[mayaUnits]
    
    _metersPerUnitToMayaUnitName = {
        _UsdGeom.LinearUnits.inches      : "inch",
        _UsdGeom.LinearUnits.feet        : "foot",
        _UsdGeom.LinearUnits.yards       : "yard",
        _UsdGeom.LinearUnits.miles       : "mile",
        _UsdGeom.LinearUnits.millimeters : "mm",
        _UsdGeom.LinearUnits.centimeters : "cn",
        _UsdGeom.LinearUnits.kilometers  : "km",
        _UsdGeom.LinearUnits.meters      : "m",
    }

    @staticmethod
    def _convertMetersPerUnitToMayaUnitName(metersPerUnit):
        if metersPerUnit not in StageConversionInfo._metersPerUnitToMayaUnitName:
            return "cm"
        return StageConversionInfo._metersPerUnitToMayaUnitName[metersPerUnit]
    
    @staticmethod
    def _getMayaMetersPerUnit():
        mayaUnits = _om.MDistance.internalUnit()
        return StageConversionInfo._convertMayaUnitToMetersPerUnit(mayaUnits)
    
    @staticmethod
    def _getUsdMetersPerUnit(stage):
        return _UsdGeom.GetStageMetersPerUnit(stage)
    
    @staticmethod
    def _getStageFromShapeNode(shapeNode):
        res = _cmds.ls(shapeNode, l=True)
        fullStageName = res[0]
        return _ufe.getStage(fullStageName)
    
    def __init__(self, shapeNode, convertUpAxis, convertUnit):
        self.shapeNode = shapeNode
        self.stage = self._getStageFromShapeNode(shapeNode)

        self.isMayaUpAxisZ = self._isMayaUpAxisZ()
        self.isUsdUpAxisZ = self._isUsdUpAxisZ(self.stage)
        self.needUpAxisConversion = convertUpAxis and (self.isMayaUpAxisZ != self.isUsdUpAxisZ)

        self.mayaMetersPerUnit = self._getMayaMetersPerUnit()
        self.usdMetersPerUnit = self._getUsdMetersPerUnit(self.stage)
        self.needUnitsConversion = convertUnit and (self.mayaMetersPerUnit != self.usdMetersPerUnit)


def convertUpAxisAndUnitByModifyingStage(conversionInfo):
    '''
    Handle the differences of up-axis and units from the USD file by modifying
    the Maya proxy shape node transform to compensate for the differences.
    '''
    if conversionInfo.needUpAxisConversion:
        angle = 90 if conversionInfo.isMayaUpAxisZ else -90
        _cmds.rotate(angle, 0, 0, conversionInfo.shapeNode, relative=True, euler=True, pivot=(0, 0, 0), forceOrderXYZ=True)

    if conversionInfo.needUnitsConversion:
        factor = conversionInfo.usdMetersPerUnit / conversionInfo.mayaMetersPerUnit
        _cmds.scale(factor, factor, factor, conversionInfo.shapeNode, relative=True, pivot=(0, 0, 0), scaleXYZ=True)


def convertUpAxisAndUnitByModifyingPrefs(conversionInfo):
    '''
    Handle the differences of up-axis and units from the USD file by modifying
    the Maya preferences to match the USD file.
    '''
    if conversionInfo.needUpAxisConversion:
        newAxis = 'z' if conversionInfo.isUsdUpAxisZ else 'y'
        _cmds.upAxis(axis=newAxis)

    if conversionInfo.needUnitsConversion:
        newUnit = conversionInfo._convertMetersPerUnitToMayaUnitName(conversionInfo.usdMetersPerUnit)
        _cmds.currentUnit(linear=newUnit)

