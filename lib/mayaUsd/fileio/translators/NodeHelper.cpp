//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "mayaUsd/fileio/translators/NodeHelper.h"

#include <maya/MDataBlock.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MPxNode.h>
#include <maya/MTime.h>

#include <cassert>
#include <cctype>
#include <iostream>
#include <sstream>

//----------------------------------------------------------------------------------------------------------------------
// takes an attribute name such as "thisIsAnAttribute" and turns it into "This Is An Attribute".
// Just used to make the attributes a little bit more readable in the Attribute Editor GUI.
//----------------------------------------------------------------------------------------------------------------------
std::string beautifyAttrName(std::string attrName)
{
    if (std::islower(attrName[0])) {
        attrName[0] = std::toupper(attrName[0]);
    }
    for (size_t i = 1; i < attrName.size(); ++i) {
        if (std::isupper(attrName[i])) {
            attrName.insert(i++, 1, ' ');
        }
    }
    return attrName;
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A little code generator that outputs the custom AE gui needed to handle file path
/// attributes. \param  nodeName type name of the node \param  attrName the name of the file path
/// attribute \param  fileFilter a filter string of the form:  "USD Files (*.usd*) (*.usd*);;Alembic
/// Files (*.abc)"
//----------------------------------------------------------------------------------------------------------------------
void constructFilePathUi(
    std::ostringstream&        oss,
    const std::string&         nodeName,
    const std::string&         attrName,
    const std::string&         fileFilter,
    const NodeHelper::FileMode mode)
{
    // generate code to create a file attribute GUI (with button to click to load the file)
    oss << "global proc AE" << nodeName << "Template_" << attrName << "New(string $anAttr) {\n";
    oss << "  setUITemplate -pushTemplate attributeEditorTemplate;\n";
    oss << "  rowLayout -numberOfColumns 3;\n";
    oss << "    text -label \"" << beautifyAttrName(attrName) << "\";\n";
    oss << "    textField " << attrName << "FilePathField;\n";
    oss << "    symbolButton -image \"navButtonBrowse.xpm\" " << attrName << "FileBrowserButton;\n";
    oss << "  setParent ..;\n";
    oss << "  AE" << nodeName << "Template_" << attrName << "Replace($anAttr);\n";
    oss << "  setUITemplate -popTemplate;\n";
    oss << "}\n";

    // generate the method that will replace the value in the control when another node of the same
    // type is selected
    oss << "global proc AE" << nodeName << "Template_" << attrName << "Replace(string $anAttr) {\n";
    oss << "  evalDeferred (\"connectControl " << attrName << "FilePathField \" + $anAttr);\n";
    oss << "  button -edit -command (\"AE" << nodeName << "Template_" << attrName
        << "FileBrowser \" + $anAttr) " << attrName << "FileBrowserButton;\n";
    oss << "}\n";

    // generate the button callback that will actually create the file dialog for our attribute.
    // Depending on the fileMode used, we may end up having more than one filename, which will be
    // munged together with a semi-colon as the seperator. It's arguably a little wasteful to retain
    // the code that munges together multiple paths when using a single file select mode. Meh. :)
    oss << "global proc AE" << nodeName << "Template_" << attrName
        << "FileBrowser(string $anAttr) {\n";
    oss << "  string $fileNames[] = `fileDialog2 -caption \"Specify " << beautifyAttrName(attrName)
        << "\"";
    if (!fileFilter.empty()) {
        oss << " -fileFilter \"" << fileFilter << "\"";
    }
    oss << " -fileMode " << mode << "`;\n";
    oss << "  if (size($fileNames) > 0) {\n";
    oss << "    string $concatonated = $fileNames[0];\n";
    oss << "    for($ii=1; $ii < size($fileNames); ++$ii) $concatonated += (\";\" + "
           "$fileNames[$ii]);\n";
    oss << "    evalEcho (\"setAttr -type \\\"string\\\" \" + $anAttr + \" \\\"\" + $concatonated "
           "+ \"\\\"\");\n";
    oss << "  }\n";
    oss << "}\n";
}

//----------------------------------------------------------------------------------------------------------------------
NodeHelper::InternalData* NodeHelper::m_internal = 0;

//----------------------------------------------------------------------------------------------------------------------
void NodeHelper::setNodeType(const MString& typeName)
{
    if (!m_internal) {
        m_internal = new InternalData;
    }
    m_internal->m_typeBeingRegistered = typeName.asChar();
}

//----------------------------------------------------------------------------------------------------------------------
void NodeHelper::addFrame(const char* frameTitle)
{
    if (!m_internal)
        m_internal = new InternalData;
    m_internal->m_frames.push_front(Frame(frameTitle));
}

//----------------------------------------------------------------------------------------------------------------------
bool NodeHelper::addFrameAttr(
    const char*            longName,
    uint32_t               flags,
    bool                   forceShow,
    Frame::AttributeUiType attrType)
{
    if (forceShow || ((flags & kWritable) && !(flags & kHidden) && !(flags & kDontAddToNode))) {
        if (m_internal) {
            Frame& frame = *m_internal->m_frames.begin();
            frame.m_attributes.push_back(longName);
            frame.m_attributeTypes.push_back(attrType);
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addEnumAttr(
    const char*        longName,
    const char*        shortName,
    uint32_t           flags,
    const char* const* strings,
    const int16_t*     values)
{
    addFrameAttr(longName, flags);

    MFnEnumAttribute fn;
    MObject          attribute = fn.create(longName, shortName, MFnData::kString);
    while (*strings) {
        fn.addField(*strings, *values);
        ++values;
        ++strings;
    }
    fn.setDefault(0);

    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addMeshAttr(const char* longName, const char* shortName, uint32_t flags)
{
    MFnTypedAttribute fn;
    MStatus           status;
    MObject attr = fn.create(longName, shortName, MFnData::kMesh, MObject::kNullObj, &status);
    if (!status)
        throw status;
    status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;

    return attr;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addStringAttr(
    const char* longName,
    const char* shortName,
    uint32_t    flags,
    bool        forceShow)
{
    return addStringAttr(longName, shortName, "", flags, forceShow);
}

//----------------------------------------------------------------------------------------------------------------------
void NodeHelper::inheritStringAttr(const char* longName, uint32_t flags, bool forceShow)
{
    addFrameAttr(longName, flags, forceShow);
}
//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addStringAttr(
    const char* longName,
    const char* shortName,
    const char* defaultValue,
    uint32_t    flags,
    bool        forceShow)
{
    inheritStringAttr(longName, flags, forceShow);

    MFnTypedAttribute fn;
    MFnStringData     stringData;
    MStatus           stat;
    MObject           attribute = fn.create(
        longName, shortName, MFnData::kString, stringData.create(MString(defaultValue), &stat));
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
void NodeHelper::inheritFilePathAttr(
    const char* longName,
    uint32_t    flags,
    FileMode    fileMode,
    const char* fileFilter)
{
    if (addFrameAttr(longName, flags, false, (Frame::AttributeUiType)fileMode)) {
        // Technically, shouldn't need to check m_internal again, as addFrameAttr
        // shouldn't return true unless m_internal is non-null... however, checking
        // out of paranoia that this might change in the future.
        if (m_internal) {
            Frame& frame = *m_internal->m_frames.begin();
            frame.m_fileFilters.push_back(fileFilter);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addFilePathAttr(
    const char* longName,
    const char* shortName,
    uint32_t    flags,
    FileMode    fileMode,
    const char* fileFilter)
{
    inheritFilePathAttr(longName, flags, fileMode, fileFilter);
    MFnTypedAttribute fn;
    MObject           attribute = fn.create(longName, shortName, MFnData::kString);
    MStatus           status = applyAttributeFlags(fn, flags);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addInt8Attr(
    const char* longName,
    const char* shortName,
    int8_t      defaultValue,
    uint32_t    flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute fn;
    MObject attribute = fn.create(longName, shortName, MFnNumericData::kChar, defaultValue);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addInt16Attr(
    const char* longName,
    const char* shortName,
    int16_t     defaultValue,
    uint32_t    flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute fn;
    MObject attribute = fn.create(longName, shortName, MFnNumericData::kShort, defaultValue);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
void NodeHelper::inheritInt32Attr(const char* longName, uint32_t flags)
{
    addFrameAttr(longName, flags);
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addInt32Attr(
    const char* longName,
    const char* shortName,
    int32_t     defaultValue,
    uint32_t    flags)
{
    inheritInt32Attr(longName, flags);

    MFnNumericAttribute fn;
    MObject attribute = fn.create(longName, shortName, MFnNumericData::kInt, defaultValue);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addInt64Attr(
    const char* longName,
    const char* shortName,
    int64_t     defaultValue,
    uint32_t    flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute fn;
    MObject attribute = fn.create(longName, shortName, MFnNumericData::kInt64, defaultValue);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addFloatAttr(
    const char* longName,
    const char* shortName,
    float       defaultValue,
    uint32_t    flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute fn;
    MObject attribute = fn.create(longName, shortName, MFnNumericData::kFloat, defaultValue);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
void NodeHelper::inheritTimeAttr(const char* longName, uint32_t flags)
{
    addFrameAttr(longName, flags);
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addTimeAttr(
    const char*  longName,
    const char*  shortName,
    const MTime& defaultValue,
    uint32_t     flags)
{
    inheritTimeAttr(longName, flags);

    MFnUnitAttribute fn;
    MObject          attribute = fn.create(longName, shortName, defaultValue);
    MStatus          status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addDistanceAttr(
    const char*      longName,
    const char*      shortName,
    const MDistance& defaultValue,
    uint32_t         flags)
{
    addFrameAttr(longName, flags);
    MFnUnitAttribute fn;
    MObject          attribute = fn.create(longName, shortName, defaultValue);
    MStatus          status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addAngleAttr(
    const char*   longName,
    const char*   shortName,
    const MAngle& defaultValue,
    uint32_t      flags)
{
    addFrameAttr(longName, flags);
    MFnUnitAttribute fn;
    MObject          attribute = fn.create(longName, shortName, defaultValue);
    MStatus          status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}
//----------------------------------------------------------------------------------------------------------------------

MObject NodeHelper::addFloatArrayAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    uint32_t       flags)
{
    addFrameAttr(longName, flags);

    MStatus           status;
    MFnTypedAttribute fnAttr;
    MString           ln(longName);
    MString           sn(shortName);

    MObject attribute = fnAttr.create(ln, sn, MFnData::kFloatArray, MObject::kNullObj, &status);

    if (status != MS::kSuccess) {
        MGlobal::displayWarning("addFloatArrayAttr:Failed to create attribute");
    }
    applyAttributeFlags(fnAttr, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);

    MFnDependencyNode fn(node, &status);
    if (!status) {
        throw status;
    }

    status = fn.addAttribute(attribute);
    if (status != MS::kSuccess)
        MGlobal::displayWarning(
            MString("addFloatArrayAttr::addAttribute: ") + MString(status.errorString()));

    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addDoubleAttr(
    const char* longName,
    const char* shortName,
    double      defaultValue,
    uint32_t    flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute fn;
    MObject attribute = fn.create(longName, shortName, MFnNumericData::kDouble, defaultValue);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
void NodeHelper::inheritBoolAttr(const char* longName, uint32_t flags)
{
    addFrameAttr(longName, flags);
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addBoolAttr(
    const char* longName,
    const char* shortName,
    bool        defaultValue,
    uint32_t    flags)
{
    inheritBoolAttr(longName, flags);

    MFnNumericAttribute fn;
    MObject attribute = fn.create(longName, shortName, MFnNumericData::kBoolean, defaultValue);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addFloat3Attr(
    const char* longName,
    const char* shortName,
    float       defaultX,
    float       defaultY,
    float       defaultZ,
    uint32_t    flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute fn;
    MObject             attribute;
    if (flags & kColour) {
        attribute = fn.createColor(longName, shortName);
        fn.setDefault(defaultX, defaultY, defaultZ);
    } else {
        MString ln(longName);
        MString sn(shortName);
        MObject x = fn.create(ln + "X", sn + "x", MFnNumericData::kFloat, defaultX);
        MObject y = fn.create(ln + "Y", sn + "y", MFnNumericData::kFloat, defaultY);
        MObject z = fn.create(ln + "Z", sn + "z", MFnNumericData::kFloat, defaultZ);
        attribute = fn.create(ln, sn, x, y, z);
    }
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addPointAttr(
    const char*   longName,
    const char*   shortName,
    const MPoint& defaultValue,
    uint32_t      flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute fn;
    MObject             attribute;
    attribute = fn.createPoint(longName, shortName);
    fn.setDefault(defaultValue.x, defaultValue.y, defaultValue.z);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addVectorAttr(
    const char*    longName,
    const char*    shortName,
    const MVector& defaultValue,
    uint32_t       flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute fn;
    MObject             attribute;
    MString             ln(longName);
    MString             sn(shortName);
    MObject             x = fn.create(ln + "X", sn + "x", MFnNumericData::kDouble, defaultValue.x);
    MObject             y = fn.create(ln + "Y", sn + "y", MFnNumericData::kDouble, defaultValue.y);
    MObject             z = fn.create(ln + "Z", sn + "z", MFnNumericData::kDouble, defaultValue.z);
    attribute = fn.create(ln, sn, x, y, z);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addAngle3Attr(
    const char* longName,
    const char* shortName,
    float       defaultX,
    float       defaultY,
    float       defaultZ,
    uint32_t    flags)
{
    addFrameAttr(longName, flags);
    MFnUnitAttribute    fnu;
    MFnNumericAttribute fn;
    MObject             attribute;
    MString             ln(longName);
    MString             sn(shortName);
    MObject             x = fnu.create(ln + "X", sn + "x", MFnUnitAttribute::kAngle, defaultX);
    MObject             y = fnu.create(ln + "Y", sn + "y", MFnUnitAttribute::kAngle, defaultY);
    MObject             z = fnu.create(ln + "Z", sn + "z", MFnUnitAttribute::kAngle, defaultZ);
    attribute = fn.create(ln, sn, x, y, z);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addDistance3Attr(
    const char* longName,
    const char* shortName,
    float       defaultX,
    float       defaultY,
    float       defaultZ,
    uint32_t    flags)
{
    addFrameAttr(longName, flags);
    MFnUnitAttribute    fnu;
    MFnNumericAttribute fn;
    MObject             attribute;
    MString             ln(longName);
    MString             sn(shortName);
    MObject             x = fnu.create(ln + "X", sn + "x", MFnUnitAttribute::kDistance, defaultX);
    MObject             y = fnu.create(ln + "Y", sn + "y", MFnUnitAttribute::kDistance, defaultY);
    MObject             z = fnu.create(ln + "Z", sn + "z", MFnUnitAttribute::kDistance, defaultZ);
    attribute = fn.create(ln, sn, x, y, z);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addMatrixAttr(
    const char*    longName,
    const char*    shortName,
    const MMatrix& defaultValue,
    uint32_t       flags)
{
    addFrameAttr(longName, flags);
    MFnMatrixAttribute fn;
    MObject            attribute;
    attribute = fn.create(longName, shortName, MFnMatrixAttribute::kDouble);
    fn.setDefault(defaultValue);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addMatrix3x3Attr(
    const char* longName,
    const char* shortName,
    const float defaultValue[3][3],
    uint32_t    flags)
{
    addFrameAttr(longName, flags);

    MFnNumericAttribute  fn;
    MFnCompoundAttribute fnc;
    MString              ln(longName);
    MString              sn(shortName);
    MObject xx = fn.create(ln + "XX", sn + "xx", MFnNumericData::kFloat, defaultValue[0][0]);
    MObject xy = fn.create(ln + "XY", sn + "xy", MFnNumericData::kFloat, defaultValue[0][1]);
    MObject xz = fn.create(ln + "XZ", sn + "xz", MFnNumericData::kFloat, defaultValue[0][2]);
    MObject yx = fn.create(ln + "YX", sn + "yx", MFnNumericData::kFloat, defaultValue[1][0]);
    MObject yy = fn.create(ln + "YY", sn + "yy", MFnNumericData::kFloat, defaultValue[1][1]);
    MObject yz = fn.create(ln + "YZ", sn + "yz", MFnNumericData::kFloat, defaultValue[1][2]);
    MObject zx = fn.create(ln + "ZX", sn + "zx", MFnNumericData::kFloat, defaultValue[2][0]);
    MObject zy = fn.create(ln + "ZY", sn + "zy", MFnNumericData::kFloat, defaultValue[2][1]);
    MObject zz = fn.create(ln + "ZZ", sn + "zz", MFnNumericData::kFloat, defaultValue[2][2]);

    MObject x = fnc.create(ln + "X", sn + "x");
    fnc.addChild(xx);
    fnc.addChild(xy);
    fnc.addChild(xz);

    MObject y = fnc.create(ln + "Y", sn + "y");
    fnc.addChild(yx);
    fnc.addChild(yy);
    fnc.addChild(yz);

    MObject z = fnc.create(ln + "Z", sn + "z");
    fnc.addChild(zx);
    fnc.addChild(zy);
    fnc.addChild(zz);

    MObject attribute = fnc.create(ln, sn);
    fnc.addChild(x);
    fnc.addChild(y);
    fnc.addChild(z);

    MStatus status = applyAttributeFlags(fnc, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addMatrix2x2Attr(
    const char* longName,
    const char* shortName,
    const float defaultValue[2][2],
    uint32_t    flags)
{
    addFrameAttr(longName, flags);

    MFnNumericAttribute  fn;
    MFnCompoundAttribute fnc;
    MString              ln(longName);
    MString              sn(shortName);
    MObject xx = fn.create(ln + "XX", sn + "xx", MFnNumericData::kFloat, defaultValue[0][0]);
    MObject xy = fn.create(ln + "XY", sn + "xy", MFnNumericData::kFloat, defaultValue[0][1]);
    MObject yx = fn.create(ln + "YX", sn + "yx", MFnNumericData::kFloat, defaultValue[1][0]);
    MObject yy = fn.create(ln + "YY", sn + "yy", MFnNumericData::kFloat, defaultValue[1][1]);

    MObject x = fnc.create(ln + "X", sn + "x");
    fnc.addChild(xx);
    fnc.addChild(xy);

    MObject y = fnc.create(ln + "Y", sn + "y");
    fnc.addChild(yx);
    fnc.addChild(yy);

    MObject attribute = fnc.create(ln, sn);
    fnc.addChild(x);
    fnc.addChild(y);

    MStatus status = applyAttributeFlags(fnc, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addDataAttr(
    const char*                      longName,
    const char*                      shortName,
    MFnData::Type                    type,
    uint32_t                         flags,
    MFnAttribute::DisconnectBehavior behaviour)
{
    MFnTypedAttribute fn;
    MObject           attribute = fn.create(longName, shortName, type);
    fn.setDisconnectBehavior(behaviour);
    MStatus status = applyAttributeFlags(fn, flags | kHidden);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addDataAttr(
    const char*                      longName,
    const char*                      shortName,
    const MTypeId&                   type,
    uint32_t                         flags,
    MFnAttribute::DisconnectBehavior behaviour)
{
    MFnTypedAttribute fn;
    MObject           attribute = fn.create(longName, shortName, type);
    fn.setDisconnectBehavior(behaviour);
    MStatus status = applyAttributeFlags(fn, flags | kHidden);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addMessageAttr(const char* longName, const char* shortName, uint32_t flags)
{
    MFnMessageAttribute fn;
    MStatus             status;
    MObject             attribute = fn.create(longName, shortName, &status);
    status = applyAttributeFlags(fn, flags | kHidden | kConnectable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addVec2fAttr(const char* longName, const char* shortName, uint32_t flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute fn;
    MObject             attribute;
    MString             ln(longName);
    MString             sn(shortName);
    MObject             x = fn.create(ln + "X", sn + "x", MFnNumericData::kFloat, 0);
    MObject             y = fn.create(ln + "Y", sn + "y", MFnNumericData::kFloat, 0);
    attribute = fn.create(ln, sn, x, y);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}
//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addVec2iAttr(const char* longName, const char* shortName, uint32_t flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute fn;
    MObject             attribute;
    MString             ln(longName);
    MString             sn(shortName);
    MObject             x = fn.create(ln + "X", sn + "x", MFnNumericData::kLong, 0);
    MObject             y = fn.create(ln + "Y", sn + "y", MFnNumericData::kLong, 0);
    attribute = fn.create(ln, sn, x, y);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}
//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addVec2dAttr(const char* longName, const char* shortName, uint32_t flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute fn;
    MObject             attribute;
    MString             ln(longName);
    MString             sn(shortName);
    MObject             x = fn.create(ln + "X", sn + "x", MFnNumericData::kDouble, 0);
    MObject             y = fn.create(ln + "Y", sn + "y", MFnNumericData::kDouble, 0);
    attribute = fn.create(ln, sn, x, y);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addDoubleArrayAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    uint32_t       flags)
{
    addFrameAttr(longName, flags);

    MStatus           status;
    MFnTypedAttribute fnAttr;
    MString           ln(longName);
    MString           sn(shortName);

    MObject attribute = fnAttr.create(ln, sn, MFnData::kDoubleArray, MObject::kNullObj, &status);

    if (status != MS::kSuccess) {
        MGlobal::displayWarning("addDoubleArrayAttr:Failed to create attribute");
    }

    applyAttributeFlags(fnAttr, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);

    MFnDependencyNode fn(node, &status);
    if (!status) {
        throw status;
    }

    status = fn.addAttribute(attribute);

    if (status != MS::kSuccess)
        MGlobal::displayWarning(
            MString("addDoubleArrayAttr::addAttribute: ") + MString(status.errorString()));

    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addVec3fAttr(const char* longName, const char* shortName, uint32_t flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute fn;
    MObject             attribute;
    MString             ln(longName);
    MString             sn(shortName);
    MObject             x = fn.create(ln + "X", sn + "x", MFnNumericData::kFloat, 0);
    MObject             y = fn.create(ln + "Y", sn + "y", MFnNumericData::kFloat, 0);
    MObject             z = fn.create(ln + "Z", sn + "z", MFnNumericData::kFloat, 0);
    attribute = fn.create(ln, sn, x, y, z);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}
//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addVec3iAttr(const char* longName, const char* shortName, uint32_t flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute fn;
    MObject             attribute;
    MString             ln(longName);
    MString             sn(shortName);
    MObject             x = fn.create(ln + "X", sn + "x", MFnNumericData::kInt, 0);
    MObject             y = fn.create(ln + "Y", sn + "y", MFnNumericData::kInt, 0);
    MObject             z = fn.create(ln + "Z", sn + "z", MFnNumericData::kInt, 0);
    attribute = fn.create(ln, sn, x, y, z);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}
//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addVec3dAttr(const char* longName, const char* shortName, uint32_t flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute fn;
    MObject             attribute;
    MString             ln(longName);
    MString             sn(shortName);
    MObject             x = fn.create(ln + "X", sn + "x", MFnNumericData::kDouble, 0);
    MObject             y = fn.create(ln + "Y", sn + "y", MFnNumericData::kDouble, 0);
    MObject             z = fn.create(ln + "Z", sn + "z", MFnNumericData::kDouble, 0);
    attribute = fn.create(ln, sn, x, y, z);
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addVec4fAttr(const char* longName, const char* shortName, uint32_t flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute  fn;
    MFnCompoundAttribute fnc;
    MObject              attribute;
    MString              ln(longName);
    MString              sn(shortName);
    MObject              x = fn.create(ln + "X", sn + "x", MFnNumericData::kFloat, 0);
    MObject              y = fn.create(ln + "Y", sn + "y", MFnNumericData::kFloat, 0);
    MObject              z = fn.create(ln + "Z", sn + "z", MFnNumericData::kFloat, 0);
    MObject              w = fn.create(ln + "W", sn + "w", MFnNumericData::kFloat, 0);
    attribute = fnc.create(ln, sn);

    fnc.addChild(x); // , "could not add x");
    fnc.addChild(y); // , "could not add y");
    fnc.addChild(z); // , "could not add z");
    fnc.addChild(w); // , "could not add w");
    MStatus status = applyAttributeFlags(fnc, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addVec4iAttr(const char* longName, const char* shortName, uint32_t flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute  fn;
    MFnCompoundAttribute fnc;
    MObject              attribute;
    MString              ln(longName);
    MString              sn(shortName);
    MObject              x = fn.create(ln + "X", sn + "x", MFnNumericData::kLong, 0);
    MObject              y = fn.create(ln + "Y", sn + "y", MFnNumericData::kLong, 0);
    MObject              z = fn.create(ln + "Z", sn + "z", MFnNumericData::kLong, 0);
    MObject              w = fn.create(ln + "W", sn + "w", MFnNumericData::kLong, 0);
    attribute = fnc.create(ln, sn);
    fnc.addChild(x); // , "could not add x");
    fnc.addChild(y); // , "could not add y");
    fnc.addChild(z); // , "could not add z");
    fnc.addChild(w); // , "could not add w");
    MStatus status = applyAttributeFlags(fnc, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addVec4dAttr(const char* longName, const char* shortName, uint32_t flags)
{
    addFrameAttr(longName, flags);
    MFnNumericAttribute  fn;
    MFnCompoundAttribute fnc;
    MObject              attribute;
    MString              ln(longName);
    MString              sn(shortName);
    MObject              x = fn.create(ln + "X", sn + "x", MFnNumericData::kDouble, 0);
    MObject              y = fn.create(ln + "Y", sn + "y", MFnNumericData::kDouble, 0);
    MObject              z = fn.create(ln + "Z", sn + "z", MFnNumericData::kDouble, 0);
    MObject              w = fn.create(ln + "W", sn + "w", MFnNumericData::kDouble, 0);
    attribute = fnc.create(ln, sn);
    fnc.addChild(x); // , "could not add x");
    fnc.addChild(y); // , "could not add y");
    fnc.addChild(z); // , "could not add z");
    fnc.addChild(w); // , "could not add w");
    MStatus status = applyAttributeFlags(fnc, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return attribute;
}

//----------------------------------------------------------------------------------------------------------------------
MObject NodeHelper::addCompoundAttr(
    const char*                    longName,
    const char*                    shortName,
    uint32_t                       flags,
    std::initializer_list<MObject> objs)
{
    addFrameAttr(longName, flags);
    MFnCompoundAttribute fn;
    MObject              obj = fn.create(longName, shortName);
    for (auto it : objs) {
        MStatus status = fn.addChild(it);
        if (!status)
            throw status;
    }
    MStatus status = applyAttributeFlags(fn, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
    if (!status)
        throw status;
    return obj;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::applyAttributeFlags(MFnAttribute& fn, uint32_t flags)
{
    // const char* const errorString = "NodeHelper::applyAttributeFlags";
    fn.setCached((flags & kCached) != 0); // , errorString);
    fn.setReadable((flags & kReadable) != 0); //, errorString);
    fn.setStorable((flags & kStorable) != 0); //, errorString);
    fn.setWritable((flags & kWritable) != 0); //, errorString);
    fn.setAffectsAppearance((flags & kAffectsAppearance) != 0); //, errorString);
    fn.setKeyable((flags & kKeyable) != 0); //, errorString);
    fn.setConnectable((flags & kConnectable) != 0); //, errorString);
    fn.setArray((flags & kArray) != 0); //, errorString);
    fn.setUsedAsColor((flags & kColour) != 0); //, errorString);
    fn.setHidden((flags & kHidden) != 0); //, errorString);
    fn.setInternal((flags & kInternal) != 0); //, errorString);
    fn.setAffectsWorldSpace((flags & kAffectsWorldSpace) != 0); //, errorString);
    fn.setUsesArrayDataBuilder((flags & kUsesArrayDataBuilder) != 0); //, errorString);

    if (!(flags & (kDynamic | kDontAddToNode))) {
        MStatus status = MPxNode::addAttribute(fn.object());
        if (!status)
            throw status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
void NodeHelper::generateAETemplate()
{
    assert(m_internal);

    // first hunt down all of the call custom attributes and generate the custom AE templates. This
    // needs to be done before we generate the main template procedure (these are all global
    // methods).
    std::ostringstream oss;
    auto               it = m_internal->m_frames.rbegin();
    auto               end = m_internal->m_frames.rend();
    for (; it != end; ++it) {
        size_t fileIndex = 0;
        for (size_t i = 0; i < it->m_attributes.size(); ++i) {
            switch (it->m_attributeTypes[i]) {
            case Frame::kLoadFilePath:
            case Frame::kSaveFilePath:
            case Frame::kDirPathWithFiles:
            case Frame::kDirPath:
            case Frame::kMultiLoadFilePath:
                constructFilePathUi(
                    oss,
                    m_internal->m_typeBeingRegistered,
                    it->m_attributes[i],
                    it->m_fileFilters[fileIndex++],
                    (FileMode)it->m_attributeTypes[i]);
                break;
            default: break;
            }
        }
    }

    // start generating our AE template, and ensure it's wrapped in a scroll layout.
    oss << "global proc AE" << m_internal->m_typeBeingRegistered
        << "Template(string $nodeName) {\n";
    oss << " editorTemplate -beginScrollLayout;\n";

    // loop through each collapsible frame
    it = m_internal->m_frames.rbegin();
    for (; it != end; ++it) {
        // frame layout begin!
        oss << "  editorTemplate -beginLayout \"" << it->m_title << "\" -collapse 0;\n";
        for (size_t i = 0; i < it->m_attributes.size(); ++i) {
            switch (it->m_attributeTypes[i]) {
            // If we have a file path attribute, use the custom callbacks
            case Frame::kLoadFilePath:
            case Frame::kSaveFilePath:
            case Frame::kDirPathWithFiles:
            case Frame::kDirPath:
            case Frame::kMultiLoadFilePath:
                oss << "    editorTemplate -callCustom \"AE" << m_internal->m_typeBeingRegistered
                    << "Template_" << it->m_attributes[i] << "New\" "
                    << "\"AE" << m_internal->m_typeBeingRegistered << "Template_"
                    << it->m_attributes[i] << "Replace\" \"" << it->m_attributes[i] << "\";\n";
                break;

            // for all other attributes, just add a normal control
            default:
                oss << "    editorTemplate -addControl \"" << it->m_attributes[i] << "\";\n";
                break;
            }
        }
        oss << "  editorTemplate -endLayout;\n";
    }

    // add all of our base templates that have been added
    for (size_t i = 0; i < m_internal->m_baseTemplates.size(); ++i) {
        oss << "  " << m_internal->m_baseTemplates[i] << " $nodeName;\n";
    }

    // finish off the call by adding in the custom attributes section
    oss << "  editorTemplate -addExtraControls;\n";
    oss << " editorTemplate -endScrollLayout;\n";
    oss << "}\n";

    // run our script (AE template command will now exist in memory)
    MGlobal::executeCommand(MString(oss.str().c_str(), oss.str().size()));

    // get rid of our internal rubbish.
    delete m_internal;
    m_internal = 0;
}

#define report_get_error(attribute, type, status)                                                 \
    {                                                                                             \
        MFnAttribute fn(attribute);                                                               \
        std::cerr << "Unable to get attribute \"" << fn.name().asChar() << "\" of type " << #type \
                  << std::endl;                                                                   \
        std::cerr << "  - " << status.errorString().asChar() << std::endl;                        \
    }

//----------------------------------------------------------------------------------------------------------------------
bool NodeHelper::inputBoolValue(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        return inDataHandle.asBool();
    }
    report_get_error(attribute, bool, status);
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
int8_t NodeHelper::inputInt8Value(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        return inDataHandle.asChar();
    }
    report_get_error(attribute, int8_t, status);
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
int16_t NodeHelper::inputInt16Value(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        return inDataHandle.asShort();
    }
    report_get_error(attribute, int16_t, status);
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
int32_t NodeHelper::inputInt32Value(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        return inDataHandle.asInt();
    }
    report_get_error(attribute, int32_t, status);
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
int64_t NodeHelper::inputInt64Value(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        return inDataHandle.asInt64();
    }
    report_get_error(attribute, int64_t, status);
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
float NodeHelper::inputFloatValue(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        return inDataHandle.asFloat();
    }
    report_get_error(attribute, float, status);
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
double NodeHelper::inputDoubleValue(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        return inDataHandle.asDouble();
    }
    report_get_error(attribute, double, status);
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
MTime NodeHelper::inputTimeValue(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        return inDataHandle.asTime();
    }
    report_get_error(attribute, MTime, status);
    return MTime();
}

//----------------------------------------------------------------------------------------------------------------------
MMatrix NodeHelper::inputMatrixValue(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        return inDataHandle.asMatrix();
    }
    report_get_error(attribute, MMatrix, status);
    return MMatrix();
}

//----------------------------------------------------------------------------------------------------------------------
MPoint NodeHelper::inputPointValue(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        const double3& v = inDataHandle.asDouble3();
        return MPoint(v[0], v[1], v[2]);
    }
    report_get_error(attribute, MPoint, status);
    return MPoint();
}

//----------------------------------------------------------------------------------------------------------------------
MFloatPoint NodeHelper::inputFloatPointValue(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        const float3& v = inDataHandle.asFloat3();
        return MFloatPoint(v[0], v[1], v[2]);
    }
    report_get_error(attribute, MFloatPoint, status);
    return MFloatPoint();
}

//----------------------------------------------------------------------------------------------------------------------
MVector NodeHelper::inputVectorValue(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        const double3& v = inDataHandle.asDouble3();
        return MVector(v[0], v[1], v[2]);
    }
    report_get_error(attribute, MVector, status);
    return MVector();
}

//----------------------------------------------------------------------------------------------------------------------
MFloatVector NodeHelper::inputFloatVectorValue(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        const float3& v = inDataHandle.asFloat3();
        return MFloatVector(v[0], v[1], v[2]);
    }
    report_get_error(attribute, MFloatVector, status);
    return MFloatVector();
}

//----------------------------------------------------------------------------------------------------------------------
MString NodeHelper::inputStringValue(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        return inDataHandle.asString();
    }
    report_get_error(attribute, MString, status);
    return MString();
}

//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
MColor NodeHelper::inputColourValue(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        const float3& v = inDataHandle.asFloat3();
        return MColor(v[0], v[1], v[2]);
    }
    report_get_error(attribute, MColor, status);
    return MColor();
}

//----------------------------------------------------------------------------------------------------------------------
MPxData* NodeHelper::inputDataValue(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle inDataHandle = dataBlock.inputValue(attribute, &status);
    if (status) {
        return inDataHandle.asPluginData();
    }
    report_get_error(attribute, MPxData, status);
    return 0;
}

#define report_set_error(attribute, type, status)                                                 \
    {                                                                                             \
        MFnAttribute fn(attribute);                                                               \
        std::cerr << "Unable to set attribute \"" << fn.name().asChar() << "\" of type " << #type \
                  << std::endl;                                                                   \
        std::cerr << "  - " << status.errorString().asChar() << std::endl;                        \
    }

//----------------------------------------------------------------------------------------------------------------------
MStatus
NodeHelper::outputBoolValue(MDataBlock& dataBlock, const MObject& attribute, const bool value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.setBool(value);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, bool, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
NodeHelper::outputInt8Value(MDataBlock& dataBlock, const MObject& attribute, const int8_t value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.setChar(value);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, int8_t, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
NodeHelper::outputInt16Value(MDataBlock& dataBlock, const MObject& attribute, const int16_t value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.setShort(value);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, int16_t, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
NodeHelper::outputInt32Value(MDataBlock& dataBlock, const MObject& attribute, const int32_t value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.setInt(value);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, int32_t, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
NodeHelper::outputInt64Value(MDataBlock& dataBlock, const MObject& attribute, const int64_t value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.setInt64(value);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, int64_t, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
NodeHelper::outputFloatValue(MDataBlock& dataBlock, const MObject& attribute, const float value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.setFloat(value);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, float, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
NodeHelper::outputDoubleValue(MDataBlock& dataBlock, const MObject& attribute, const double value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.setDouble(value);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, double, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
NodeHelper::outputMatrixValue(MDataBlock& dataBlock, const MObject& attribute, const MMatrix& value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.setMMatrix(value);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, MMatrix, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
NodeHelper::outputPointValue(MDataBlock& dataBlock, const MObject& attribute, const MPoint& value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.set(value.x, value.y, value.z);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, MPoint, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::outputFloatPointValue(
    MDataBlock&        dataBlock,
    const MObject&     attribute,
    const MFloatPoint& value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.set(value.x, value.y, value.z);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, MFloatPoint, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
NodeHelper::outputVectorValue(MDataBlock& dataBlock, const MObject& attribute, const MVector& value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.set(value.x, value.y, value.z);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, MVector, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::outputEulerValue(
    MDataBlock&           dataBlock,
    const MObject&        attribute,
    const MEulerRotation& value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.set(value.x, value.y, value.z);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, MEulerRotation, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::outputFloatVectorValue(
    MDataBlock&         dataBlock,
    const MObject&      attribute,
    const MFloatVector& value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.set(value.x, value.y, value.z);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, MFloatVector, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
NodeHelper::outputColourValue(MDataBlock& dataBlock, const MObject& attribute, const MColor& value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.set(value.r, value.g, value.b);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, MColor, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
NodeHelper::outputStringValue(MDataBlock& dataBlock, const MObject& attribute, const MString& value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.setString(value);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, MString, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
NodeHelper::outputTimeValue(MDataBlock& dataBlock, const MObject& attribute, const MTime& value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.set(value);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, MTime, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::outputDataValue(MDataBlock& dataBlock, const MObject& attribute, MPxData* value)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        outDataHandle.set(value);
        outDataHandle.setClean();
    } else {
        report_set_error(attribute, MPxData, status);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MPxData* NodeHelper::outputDataValue(MDataBlock& dataBlock, const MObject& attribute)
{
    MStatus     status;
    MDataHandle outDataHandle = dataBlock.outputValue(attribute, &status);
    if (status) {
        return outDataHandle.asPluginData();
    }
    report_get_error(attribute, MPxData, status);
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
MPxData* NodeHelper::createData(const MTypeId& dataTypeId, MObject& data)
{
    MStatus       status;
    MFnPluginData pluginDataFactory;
    data = pluginDataFactory.create(dataTypeId, &status);
    if (!status) {
        std::cerr << "Unable to create data object of type id: " << dataTypeId.id() << ":"
                  << dataTypeId.className() << std::endl;
        return 0;
    }
    return pluginDataFactory.data();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addStringAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    uint32_t       flags,
    bool           forceShow,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addStringAttr(longName, shortName, flags | kDynamic, forceShow);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add string attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addFilePathAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    uint32_t       flags,
    FileMode       forSaving,
    const char*    fileFilter,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr
            = addFilePathAttr(longName, shortName, flags | kDynamic, forSaving, fileFilter);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add filename attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addInt8Attr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    int8_t         defaultValue,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addInt8Attr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add int attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addInt16Attr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    int16_t        defaultValue,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addInt16Attr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add int attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addInt32Attr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    int32_t        defaultValue,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addInt32Attr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add int attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addInt64Attr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    int64_t        defaultValue,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addInt64Attr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add int attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addFloatAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    float          defaultValue,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addFloatAttr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add float attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addDoubleAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    double         defaultValue,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addDoubleAttr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add double attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addTimeAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    const MTime&   defaultValue,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addTimeAttr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add time attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addDistanceAttr(
    const MObject&   node,
    const char*      longName,
    const char*      shortName,
    const MDistance& defaultValue,
    uint32_t         flags,
    MObject*         attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addDistanceAttr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add distance attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addAngleAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    const MAngle&  defaultValue,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addAngleAttr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add angle attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addBoolAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    bool           defaultValue,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addBoolAttr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add bool attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addFloat3Attr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    float          defaultX,
    float          defaultY,
    float          defaultZ,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr
            = addFloat3Attr(longName, shortName, defaultX, defaultY, defaultZ, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add float3 attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addAngle3Attr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    float          defaultX,
    float          defaultY,
    float          defaultZ,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr
            = addAngle3Attr(longName, shortName, defaultX, defaultY, defaultZ, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add angle3 attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addPointAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    const MPoint&  defaultValue,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addPointAttr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add point attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addVectorAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    const MVector& defaultValue,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addVectorAttr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add vector attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addMatrixAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    const MMatrix& defaultValue,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addMatrixAttr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add matrix attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addMatrix2x2Attr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    const float    defaultValue[2][2],
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addMatrix2x2Attr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add matrix2x2 attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addMatrix3x3Attr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    const float    defaultValue[3][3],
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addMatrix3x3Attr(longName, shortName, defaultValue, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add matrix3x3 attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addDataAttr(
    const MObject&                   node,
    const char*                      longName,
    const char*                      shortName,
    MFnData::Type                    type,
    uint32_t                         flags,
    MFnAttribute::DisconnectBehavior behaviour,
    MObject*                         attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addDataAttr(longName, shortName, type, flags | kDynamic, behaviour);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add data attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addDataAttr(
    const MObject&                   node,
    const char*                      longName,
    const char*                      shortName,
    const MTypeId&                   type,
    uint32_t                         flags,
    MFnAttribute::DisconnectBehavior behaviour,
    MObject*                         attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addDataAttr(longName, shortName, type, flags | kDynamic, behaviour);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add data attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addMessageAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addMessageAttr(longName, shortName, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add message attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addVec2fAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addVec2fAttr(longName, shortName, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add vec2 attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addVec2iAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addVec2iAttr(longName, shortName, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add vec2 attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addVec2dAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addVec2dAttr(longName, shortName, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add vec2 attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addVec3fAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addVec3fAttr(longName, shortName, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add vec3 attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addVec3iAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addVec3iAttr(longName, shortName, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add vec3 attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addVec3dAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addVec3dAttr(longName, shortName, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add vec3 attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addVec4fAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addVec4fAttr(longName, shortName, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add vec4 attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addVec4iAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addVec4iAttr(longName, shortName, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add vec4 attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus NodeHelper::addVec4dAttr(
    const MObject& node,
    const char*    longName,
    const char*    shortName,
    uint32_t       flags,
    MObject*       attribute)
{
    try {
        MStatus           status;
        MFnDependencyNode fn(node, &status);
        if (!status) {
            throw status;
        }
        MObject attr = addVec4dAttr(longName, shortName, flags | kDynamic | kConnectable | kKeyable | kWritable | kReadable | kStorable);
        status = fn.addAttribute(attr);
        if (!status) {
            MGlobal::displayError(
                MString("Unable to add vec4 attribute ") + longName + " to node " + fn.name());
            throw status;
        }
        if (attribute)
            *attribute = attr;
    } catch (MStatus status) {
        return status;
    }
    return MS::kSuccess;
}
