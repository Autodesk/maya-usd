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
#pragma once

#include "AL/maya/utils/Api.h"

#include <maya/MColor.h>
#include <maya/MFloatVector.h>
#include <maya/MFnNumericAttribute.h>

#include <deque>
#include <string>
#include <vector>

namespace AL {
namespace maya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This macro simply adds an MObject member variable to an MPxNode derived class to be used
/// as a node attribute.
///         It also add's a couple of public methods to access a nodes plug, and to access the
///         attributes MObject. For example, if my node looked like:
/// \code
/// class MyNode
///   : public MPxNode
/// {
///   AL_DECL_ATTRIBUTE(myAttr);
/// };
/// \endcode
/// This would effectively expand to:
/// \code
/// class MyNode
///   : public MPxNode
/// {
///   static private MObject m_myAttr; ///< the attribute handle
/// public:
///
///   // access the attribute handle
///   static MObject myAttr()
///     { return m_myAttr; }
///
///   // access the attribute plug
///   MPlug myAttrPlug() const
///     { return MPlug(thisMObject(), m_myAttr); }
/// };
/// \endcode
/// \ingroup mayagui
//----------------------------------------------------------------------------------------------------------------------

// For children of multi-attribute, a generic XXPlug() method
// isn't very helpful, as we need to attach to a specific indexed
// element plug of the parent array... and defining it just
// creates a confusing name
#define AL_DECL_MULTI_CHILD_ATTRIBUTE(XX) \
protected:                                \
    AL_MAYA_MACROS_PUBLIC                 \
    static MObject m_##XX;                \
                                          \
public:                                   \
    AL_MAYA_MACROS_PUBLIC                 \
    static const MObject& XX() { return m_##XX; }

#define AL_DECL_ATTRIBUTE(XX)         \
    AL_DECL_MULTI_CHILD_ATTRIBUTE(XX) \
    AL_MAYA_MACROS_PUBLIC             \
    MPlug XX##Plug() const { return MPlug(thisMObject(), m_##XX); }

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This is a little helper object designed to reduce the amount of boilerplate GUI code you
/// need to jump through
///         to add your own nodes that match a USD schema type. It has been designed to attempt to
///         match the attribute types of USD as closely as possible, so adds support for 2x2 / 3x3
///         matrix types, half float support, etc.
///
///         In order to use this class, you should inherit from which ever MPxNode type you need
///         (e.g. MPxLocator, MPxSurfaceShape, etc), as well as the NodeHelper class. So your header
///         file should look something like this:
/// \code
/// #pragma once
/// #include "AL/maya/NodeHelper.h"
/// #include <maya/MPxNode.h>
///
/// // extremely simple node that adds two numbers
/// class Add2Floats
///   : public MPxNode,
///     public AL::maya::NodeHelper
/// {
/// public:
///
///   MStatus compute(const MPlug& plug, MDataBlock& dataBlock) override;
///
///   AL_DECL_ATTRIBUTE(input0);
///   AL_DECL_ATTRIBUTE(input1);
///   AL_DECL_ATTRIBUTE(output);
/// };
/// \endcode
///
/// And then in the cpp file...
///
/// \code
/// #include "Add2Floats.h"
///
/// // NOTE: I'm prefixing all calls to inherited functions from NodeHelper with the full scope.
/// There is no reason
/// // to do this in C++. I'm only doing it in this example code to get the documentation to contain
/// links to the actual
/// // functions involved!
///
/// // some boiler plate to define the typeId as 0x1234, and the node name as
/// "AL_examples_Add2Floats" AL_MAYA_DEFINE_NODE(Add2Floats, 0x1234, AL_examples);
///
/// // make sure you add the static MObjects for your node attributes
/// MObject Add2Floats::m_input0 = MObject::kNullObj;
/// MObject Add2Floats::m_input1 = MObject::kNullObj;
/// MObject Add2Floats::m_output = MObject::kNullObj;
///
/// // initialise the attributes
/// MStatus Add2Floats::initialise()
/// {
///   // the node helper will throw an exception if any underlying MFnAttribute calls fails.
///   try
///   {
///     // first specify the typename of the node (this is required in order to sure the AE Template
///     // generated will correctly match this node type)
///     AL::maya::NodeHelper::setNodeType(kTypeName);
///
///     // You *MUST* add at least one AETemplate frame to insert controls into. Once you have added
///     // a frame, all subsequent attributes will be displayed under the frame group. You can add
///     // as many frames as you wish.
///     AL::maya::NodeHelper::addFrame("Add 2 Float GUI");
///
///     // add the two input attributes
///     m_input0 = AL::maya::NodeHelper::addFloatAttr("input0", "in0", 0.0f, kReadable | kWritable |
///     kStorable | kConnectable | kKeyable); m_input1 =
///     AL::maya::NodeHelper::addFloatAttr("input1", "in1", 0.0f, kReadable | kWritable | kStorable
///     | kConnectable | kKeyable);
///
///     // add the output
///     m_output = AL::maya::NodeHelper::addFloatAttr("output", "out", 0.0f, kReadable |
///     kConnectable);
///
///     // set up the attribute dependencies as per usual
///     attributeAffects(m_input0, m_output);
///     attributeAffects(m_input1, m_output);
///   }
///   catch(const MStatus& status)
///   {
///     return status;
///   }
///
///   // finally, generate and execute the AE Template for this node.
///   AL::maya::NodeHelper::generateAETemplate();
///
///   // all done!
///   return MS::kSuccess;
/// }
///
/// // the compute method
/// MStatus Add2Floats::compute(const MPlug& plug, MDataBlock& dataBlock)
/// {
///   if(plug == output())
///   {
///     // grab input data
///     const float in0 = AL::maya::NodeHelper::inputFloatValue(dataBlock, input0());
///     const float in1 = AL::maya::NodeHelper::inputFloatValue(dataBlock, input1());
///
///     // sum both values, and set the output
///     return AL::maya::NodeHelper::outputFloatValue(dataBlock, output(), in0 + in1);
///   }
///   return MPxNode::compute(plug, dataBlock);
/// }
/// \endcode
/// and finally, within your main plugin.cpp, you should be able to simply do:
/// \code
/// #include <maya/MFnPlugin.h>
/// #include "Add2Floats.h"
///
/// DSO_EXPORT MStatus initializePlugin(MObject obj)
/// {
///   MFnPlugin plugin(obj);
///   AL_REGISTER_DEPEND_NODE(plugin, Add2Floats);
///   return MS::kSuccess;
/// }
///
/// DSO_EXPORT MStatus uninitializePlugin(MObject obj)
/// {
///   MFnPlugin plugin(obj);
///   AL_UNREGISTER_NODE(plugin, Add2Floats);
///   return MS::kSuccess;
/// }
/// \endcode
///
/// \note   For a complete example of how it should be used, please see the
/// AL::maya::NodeHelperUnitTest files. \ingroup mayagui
//----------------------------------------------------------------------------------------------------------------------
class NodeHelper
{
#ifndef AL_GENERATING_DOCS
    struct Frame
    {
        Frame(const char* frameTitle)
            : m_title(frameTitle)
        {
        }

        enum AttributeUiType
        {
            // deliberately numbered!
            kLoadFilePath = 0,
            kSaveFilePath = 1,
            kDirPathWithFiles = 2,
            kDirPath = 3,
            kMultiLoadFilePath = 4,
            kNormal = 5,
            kHidden = 6
        };
        std::string                  m_title;
        std::vector<AttributeUiType> m_attributeTypes;
        std::vector<std::string>     m_attributes;
        std::vector<std::string>     m_fileFilters;
    };

    struct InternalData
    {
        std::string              m_typeBeingRegistered;
        std::vector<std::string> m_baseTemplates;
        std::deque<Frame>        m_frames;
    };
    AL_MAYA_UTILS_PUBLIC
    static InternalData* m_internal;
#endif

public:
    /// \brief  ctor
    NodeHelper() { }

    /// \brief  dtor
    ~NodeHelper() { }

    /// \name   Access Input Values from an MDataBlock

    /// \brief  get an input boolean value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static bool inputBoolValue(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input 8 bit integer value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static int8_t inputInt8Value(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input 16 bit integer value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static int16_t inputInt16Value(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input 32 bit integer value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static int32_t inputInt32Value(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input 64 bit integer value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static int64_t inputInt64Value(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input float value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static float inputFloatValue(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input double value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static double inputDoubleValue(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input matrix value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static MMatrix inputMatrixValue(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input point value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static MPoint inputPointValue(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input point value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static MFloatPoint inputFloatPointValue(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input vector value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static MVector inputVectorValue(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input time value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static MTime inputTimeValue(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input vector value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static MFloatVector inputFloatVectorValue(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input colour value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static MColor inputColourValue(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input string value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static MString inputStringValue(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input data value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    AL_MAYA_UTILS_PUBLIC
    static MPxData* inputDataValue(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an input data value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to query
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    template <typename MPxDataType>
    static MPxDataType* inputDataValue(MDataBlock& dataBlock, const MObject& attribute)
    {
        MPxData* data = NodeHelper::inputDataValue(dataBlock, attribute);
        if (data) {
            return dynamic_cast<MPxDataType*>(data);
        }
        return 0;
    }

    /// \name   Set Output Values from an MDataBlock

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus outputBoolValue(MDataBlock& dataBlock, const MObject& attribute, bool value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus outputInt8Value(MDataBlock& dataBlock, const MObject& attribute, int8_t value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus outputInt16Value(MDataBlock& dataBlock, const MObject& attribute, int16_t value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus outputInt32Value(MDataBlock& dataBlock, const MObject& attribute, int32_t value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus outputInt64Value(MDataBlock& dataBlock, const MObject& attribute, int64_t value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus outputFloatValue(MDataBlock& dataBlock, const MObject& attribute, float value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus outputDoubleValue(MDataBlock& dataBlock, const MObject& attribute, double value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus
    outputMatrixValue(MDataBlock& dataBlock, const MObject& attribute, const MMatrix& value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus
    outputPointValue(MDataBlock& dataBlock, const MObject& attribute, const MPoint& value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus outputFloatPointValue(
        MDataBlock&        dataBlock,
        const MObject&     attribute,
        const MFloatPoint& value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus
    outputVectorValue(MDataBlock& dataBlock, const MObject& attribute, const MVector& value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus
    outputEulerValue(MDataBlock& dataBlock, const MObject& attribute, const MEulerRotation& value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus outputFloatVectorValue(
        MDataBlock&         dataBlock,
        const MObject&      attribute,
        const MFloatVector& value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus
    outputColourValue(MDataBlock& dataBlock, const MObject& attribute, const MColor& value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus
    outputStringValue(MDataBlock& dataBlock, const MObject& attribute, const MString& value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus
    outputTimeValue(MDataBlock& dataBlock, const MObject& attribute, const MTime& value);

    /// \brief  Set the output value of the specified attribute in the datablock
    /// \param  dataBlock the data block to set the value in
    /// \param  attribute the handle to the attribute to set
    /// \param  value the attribute value to set
    /// \return MS::kSuccess if all went well
    AL_MAYA_UTILS_PUBLIC
    static MStatus outputDataValue(MDataBlock& dataBlock, const MObject& attribute, MPxData* value);

    /// \brief  get an output data value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to get an MPxData for
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    ///         Useful when you want to modify something on the underlying MPxData, without
    ///         creating / setting an entirely new instance of the MPxData
    AL_MAYA_UTILS_PUBLIC
    static MPxData* outputDataValue(MDataBlock& dataBlock, const MObject& attribute);

    /// \brief  get an output data value from the dataBlock from the specified attribute
    /// \param  dataBlock the data block to get the value from
    /// \param  attribute the handle to the attribute to get an MPxData subclass for
    /// \return the value
    /// \note   If the value could not be queried, and error will be logged to std::cerr
    ///         Useful when you want to modify something on the underlying MPxData subclass, without
    ///         creating / setting an entirely new instance of the MPxData subclass
    template <typename MPxDataType>
    static MPxDataType* outputDataValue(MDataBlock& dataBlock, const MObject& attribute)
    {
        MPxData* data = NodeHelper::outputDataValue(dataBlock, attribute);
        if (data) {
            return dynamic_cast<MPxDataType*>(data);
        }
        return 0;
    }

    /// \brief  helper method to create new data objects of the specified data type
    /// \param  dataTypeId the MTypeId of the plugin data object to create
    /// \param  data the returned handle to the created data object, usually passed to
    /// MDataHandle::set, or MPlug::setValue. \return a pointer to the data object created
    AL_MAYA_UTILS_PUBLIC
    static MPxData* createData(const MTypeId& dataTypeId, MObject& data);

    /// \brief  helper method to create new data objects of the specified data type
    /// \param  dataTypeId the MTypeId of the plugin data object to create
    /// \param  data the returned handle to the created data object, usually passed to
    /// MDataHandle::set, or MPlug::setValue. \return a pointer to the data object created
    template <typename MPxDataType>
    static MPxDataType* createData(const MTypeId& dataTypeId, MObject& data)
    {
        MPxData* ptr = NodeHelper::createData(dataTypeId, data);
        return static_cast<MPxDataType*>(ptr);
    }

    /// \name   Specify the attributes of a node, and AE GUI generation

    /// \brief  A set of bit flags you can apply to an attribute
    enum AttributeFlags
    {
        kCached = 1 << 0,            ///< The attribute should be cached
        kReadable = 1 << 1,          ///< The attribute should be readable (output)
        kWritable = 1 << 2,          ///< The attribute should be writable (input)
        kStorable = 1 << 3,          ///< The attribute should be stored in a maya file
        kAffectsAppearance = 1 << 4, ///< the attribute affects the appearance of a shape
        kKeyable = 1 << 5,           ///< The attribute can be animated
        kConnectable = 1 << 6,       ///< The attribute can be connected to another attr
        kArray = 1 << 7,             ///< The attribute is an array
        kColour
        = 1 << 8, ///< The attribute is a colour (UI will display a colour picker in the GUI)
        kHidden = 1 << 9,    ///< The attribute is hidden
        kInternal = 1 << 10, ///< The attribute value will be stored as a member variable, and
                             ///< getInternalValueInContext / setInternalValueInContext will be
                             ///< overridden to get/set the value
        kAffectsWorldSpace
        = 1 << 11, ///< The attribute affects the world space matrix of a custom transform node
        kUsesArrayDataBuilder = 1 << 12, ///< The array can be resized via an array data builder
        kDontAddToNode
        = 1 << 30,         ///< prevent the attribute from being added to the current node type
        kDynamic = 1 << 31 ///< The attribute is a dynamic attribute added at runtime (and not
                           ///< during a plug-in node initialization)
    };

    /// \brief  Specify the type of file/dir path when adding file path attributes. See
    /// addFilePathAttr
    enum FileMode
    {
        kSave = 0,               ///< a save file dialog
        kLoad = 1,               ///< a load file dialog
        kDirectoryWithFiles = 2, ///< a directory dialog, but displays files.
        kDirectory = 3,          ///< a directory dialog
        kMultiLoad = 4           ///< multiple input files
    };

    /// \brief  Sets the node type name you are adding attributes. Please call this before adding
    /// any frames! \param  typeName the type name of the node
    AL_MAYA_UTILS_PUBLIC
    static void setNodeType(const MString& typeName);

    /// \brief  Add a new frame control into the AE template.
    /// \param  frameTitle the text to appear in the ui frame
    /// \note   You MUST call this method at least once before adding any attributes
    AL_MAYA_UTILS_PUBLIC
    static void addFrame(const char* frameTitle);

    /// \brief  add an attribute to the current AE template frame
    /// \param  longName  long name of the attribute
    AL_MAYA_UTILS_PUBLIC
    static bool addFrameAttr(
        const char*            longName,
        uint32_t               flags,
        bool                   forceShow = false,
        Frame::AttributeUiType attrType = Frame::kNormal);

    /// \brief  add a new compound attribute to this node type
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  list the attributes you wish to add as children
    /// to this node.
    AL_MAYA_UTILS_PUBLIC
    static MObject addCompoundAttr(
        const char*                    longName,
        const char*                    shortName,
        uint32_t                       flags,
        std::initializer_list<MObject> list);

    /// \brief  add a new enum attribute to this node type
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  strings an array of text strings for the enum
    /// values. This last item in this array must be NULL \param  values an array of numeric enum
    /// values. \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addEnumAttr(
        const char*        longName,
        const char*        shortName,
        uint32_t           flags,
        const char* const* strings,
        const int16_t*     values);

    /// \brief  add a new string attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  forceShow  force attribute to be shown.  Used
    /// in case attribute is not writable but needs to be shown i.e. read-only. \return the MObject
    /// for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addStringAttr(
        const char* longName,
        const char* shortName,
        uint32_t    flags,
        bool        forceShow = false);

    /// \brief  inherit in this node type a string attribute from a base node type.
    /// \param  longName  long name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  forceShow  force attribute to be shown.  Used
    /// in case attribute is not writable but needs to be shown i.e. read-only.
    AL_MAYA_UTILS_PUBLIC
    static void inheritStringAttr(const char* longName, uint32_t flags, bool forceShow = false);

    /// \brief  add a new string attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  forceShow  force attribute to be shown.  Used
    /// in case attribute is not writable but needs to be shown i.e. read-only. \return the MObject
    /// for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addStringAttr(
        const char* longName,
        const char* shortName,
        const char* defaultValue,
        uint32_t    flags,
        bool        forceShow = false);

    /// \brief  add a new file path attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  fileMode  an enum that determines whether the
    /// GUI should display a file open dialog, file save, or directory dialog. \param  fileFilter a
    /// file filter of the form:
    ///           "USD Files (*.usd*) (*.usd*);;Alembic Files (*.abc) (*.abc);;All files (*.*)
    ///           (*.*)"
    /// \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addFilePathAttr(
        const char* longName,
        const char* shortName,
        uint32_t    flags,
        FileMode    fileMode,
        const char* fileFilter = "");

    /// \brief  inherit in this node type a file path attribute from a base node type.
    /// \param  longName  long name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  fileMode  an enum that determines whether the
    /// GUI should display a file open dialog, file save, or directory dialog. \param  fileFilter a
    /// file filter of the form:
    ///           "USD Files (*.usd*) (*.usd*);;Alembic Files (*.abc) (*.abc);;All files (*.*)
    ///           (*.*)"
    AL_MAYA_UTILS_PUBLIC
    static void inheritFilePathAttr(
        const char* longName,
        uint32_t    flags,
        FileMode    fileMode,
        const char* fileFilter = "");

    /// \brief  add a new integer attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject
    addInt8Attr(const char* longName, const char* shortName, int8_t defaultValue, uint32_t flags);

    /// \brief  add a new integer attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject
    addInt16Attr(const char* longName, const char* shortName, int16_t defaultValue, uint32_t flags);

    /// \brief  add a new integer attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject
    addInt32Attr(const char* longName, const char* shortName, int32_t defaultValue, uint32_t flags);

    /// \brief  inherit in this node type an integer attribute from a base node type.
    /// \param  longName  long name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc
    AL_MAYA_UTILS_PUBLIC
    static void inheritInt32Attr(const char* longName, uint32_t flags);

    /// \brief  add a new integer attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject
    addInt64Attr(const char* longName, const char* shortName, int64_t defaultValue, uint32_t flags);

    /// \brief  add a new floating point attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject
    addFloatAttr(const char* longName, const char* shortName, float defaultValue, uint32_t flags);

    /// \brief  add a new double attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject
    addDoubleAttr(const char* longName, const char* shortName, double defaultValue, uint32_t flags);

    /// \brief  add a new time attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addTimeAttr(
        const char*  longName,
        const char*  shortName,
        const MTime& defaultValue,
        uint32_t     flags);

    /// \brief  inherit in this node type a time attribute from a base node type.
    /// \param  longName  long name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc
    AL_MAYA_UTILS_PUBLIC
    static void inheritTimeAttr(const char* longName, uint32_t flags);

    /// \brief  add a new time attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addDistanceAttr(
        const char*      longName,
        const char*      shortName,
        const MDistance& defaultValue,
        uint32_t         flags);

    /// \brief  add a new time attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addAngleAttr(
        const char*   longName,
        const char*   shortName,
        const MAngle& defaultValue,
        uint32_t      flags);

    /// \brief  add a new boolean attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject
    addBoolAttr(const char* longName, const char* shortName, bool defaultValue, uint32_t flags);

    /// \brief  inherit in this node type a boolean attribute from a base node type.
    /// \param  longName  long name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc
    AL_MAYA_UTILS_PUBLIC
    static void inheritBoolAttr(const char* longName, uint32_t flags);

    /// \brief  add a new float3 attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultX  the default value for the attribute
    /// \param  defaultY  the default value for the attribute
    /// \param  defaultZ  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addFloat3Attr(
        const char* longName,
        const char* shortName,
        float       defaultX,
        float       defaultY,
        float       defaultZ,
        uint32_t    flags);

    /// \brief  add a new float3 attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultX  the default value for the attribute
    /// \param  defaultY  the default value for the attribute
    /// \param  defaultZ  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addAngle3Attr(
        const char* longName,
        const char* shortName,
        float       defaultX,
        float       defaultY,
        float       defaultZ,
        uint32_t    flags);

    /// \brief  add a new float3 attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultX  the default value for the attribute
    /// \param  defaultY  the default value for the attribute
    /// \param  defaultZ  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc
    AL_MAYA_UTILS_PUBLIC
    static MObject addDistance3Attr(
        const char* longName,
        const char* shortName,
        float       defaultX,
        float       defaultY,
        float       defaultZ,
        uint32_t    flags);

    /// \brief  add a new point attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addPointAttr(
        const char*   longName,
        const char*   shortName,
        const MPoint& defaultValue,
        uint32_t      flags);

    /// \brief  add a new float point attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addFloatPointAttr(
        const char*        longName,
        const char*        shortName,
        const MFloatPoint& defaultValue,
        uint32_t           flags)
    {
        return addFloat3Attr(
            longName, shortName, defaultValue.x, defaultValue.y, defaultValue.z, flags);
    }

    /// \brief  add a new vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addVectorAttr(
        const char*    longName,
        const char*    shortName,
        const MVector& defaultValue,
        uint32_t       flags);

    /// \brief  add a new float vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addFloatVectorAttr(
        const char*         longName,
        const char*         shortName,
        const MFloatVector& defaultValue,
        uint32_t            flags)
    {
        return addFloat3Attr(
            longName, shortName, defaultValue.x, defaultValue.y, defaultValue.z, flags);
    }

    /// \brief  add a new colour attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addColourAttr(
        const char*   longName,
        const char*   shortName,
        const MColor& defaultValue,
        uint32_t      flags)
    {
        return addFloat3Attr(
            longName, shortName, defaultValue.r, defaultValue.g, defaultValue.b, flags | kColour);
    }

    /// \brief  add a new matrix attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addMatrix2x2Attr(
        const char* longName,
        const char* shortName,
        const float defaultValue[2][2],
        uint32_t    flags);

    /// \brief  add a new matrix attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addMatrix3x3Attr(
        const char* longName,
        const char* shortName,
        const float defaultValue[3][3],
        uint32_t    flags);

    /// \brief  add a new matrix attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addMatrixAttr(
        const char*    longName,
        const char*    shortName,
        const MMatrix& defaultValue,
        uint32_t       flags);

    /// \brief  add a new data attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  type the data type for the attribute \param
    /// behaviour optionally control what happens when the attribute is disconnected \return the
    /// MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addDataAttr(
        const char*                      longName,
        const char*                      shortName,
        MFnData::Type                    type,
        uint32_t                         flags,
        MFnAttribute::DisconnectBehavior behaviour = MFnAttribute::kNothing);

    /// \brief  add a new data attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  type the data type for the attribute \param
    /// behaviour optionally control what happens when the attribute is disconnected \return the
    /// MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addDataAttr(
        const char*                      longName,
        const char*                      shortName,
        const MTypeId&                   type,
        uint32_t                         flags,
        MFnAttribute::DisconnectBehavior behaviour = MFnAttribute::kNothing);

    /// \brief  add a new message attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addMessageAttr(const char* longName, const char* shortName, uint32_t flags);

    /// \brief  add a new 2D vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    static MObject addVec2hAttr(const char* longName, const char* shortName, uint32_t flags)
    {
        return addVec2fAttr(longName, shortName, flags);
    }

    /// \brief  add a new 2D vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addVec2fAttr(const char* longName, const char* shortName, uint32_t flags);

    /// \brief  add a new 2D vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addVec2iAttr(const char* longName, const char* shortName, uint32_t flags);

    /// \brief  add a new 2D vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addVec2dAttr(const char* longName, const char* shortName, uint32_t flags);

    /// \brief  add a new double attribute to this node type.
    /// \param  node the node to which the attribute will be added
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addFloatArrayAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags);

    /// \brief  add a new DoubleArray attribute to this node type.
    /// \param  node the node to which the attribute will be added
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addDoubleArrayAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags);

    /// \brief  add a new 3D vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    static MObject addVec3hAttr(const char* longName, const char* shortName, uint32_t flags)
    {
        return addVec3fAttr(longName, shortName, flags);
    }

    /// \brief  add a new 3D vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addVec3fAttr(const char* longName, const char* shortName, uint32_t flags);

    /// \brief  add a new 3D vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addVec3iAttr(const char* longName, const char* shortName, uint32_t flags);

    /// \brief  add a new 3D vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addVec3dAttr(const char* longName, const char* shortName, uint32_t flags);

    /// \brief  add a new 4D vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    static MObject addVec4hAttr(const char* longName, const char* shortName, uint32_t flags)
    {
        return addVec4fAttr(longName, shortName, flags);
    }

    /// \brief  add a new 4D vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addVec4fAttr(const char* longName, const char* shortName, uint32_t flags);

    /// \brief  add a new 4D vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addVec4iAttr(const char* longName, const char* shortName, uint32_t flags);

    /// \brief  add a new 4D vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \return the MObject for the attribute
    AL_MAYA_UTILS_PUBLIC
    static MObject addVec4dAttr(const char* longName, const char* shortName, uint32_t flags);

    /// \brief  set the min/max values on a numeric attribute
    /// \param  obj the attribute handle
    /// \param  minimum the min value for the attribute
    /// \param  maximum the max value for the attribute
    template <typename datatype>
    static void setMinMax(MObject obj, datatype minimum, datatype maximum)
    {
        MFnNumericAttribute fn(obj);
        fn.setMin(minimum);
        fn.setMax(maximum);
    }

    /// \brief  set the min/max/softmax values on a numeric attribute
    /// \param  obj the attribute handle
    /// \param  minimum the min value for the attribute
    /// \param  maximum the max value for the attribute
    /// \param  softmin the soft min value for the attribute
    /// \param  softmax the soft max value for the attribute
    template <typename datatype>
    static void
    setMinMax(MObject obj, datatype minimum, datatype maximum, datatype softmin, datatype softmax)
    {
        MFnNumericAttribute fn(obj);
        fn.setMin(minimum);
        fn.setMax(maximum);
        fn.setSoftMin(softmin);
        fn.setSoftMax(softmax);
    }

    /// \brief  used to add additional references to AETemplate calls for standard types, e.g.
    /// "AEsurfaceShapeTemplate"
    ///         these will be inserted into the correct location
    /// \param  baseTemplate the additional AE template UI
    static void addBaseTemplate(const std::string& baseTemplate)
    {
        if (!baseTemplate.empty())
            m_internal->m_baseTemplates.push_back(baseTemplate);
    }

    /// \brief  This method will construct up the MEL script code for the attribute editor template
    /// for your node.
    ///         Once constructed, the code will be executed silently in the background. If you wish
    ///         to see the code being executed, enable 'echo all commands' in the MEL script editor
    ///         prior to loading your plug-in.
    AL_MAYA_UTILS_PUBLIC
    static void generateAETemplate();

    //--------------------------------------------------------------------------------------------------------------------
    /// \name   Add Dynamic Attributes to Node
    //--------------------------------------------------------------------------------------------------------------------

    /// \brief  add a new string attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  forceShow  force attribute to be shown.  Used
    /// in case attribute is not writable but needs to be shown i.e. read-only. \param  node the
    /// node to add the attribute to \param  attribute an optional pointer to an MObject in which
    /// the attribute handle will be returned \return MS::kSuccess when succeeded, otherwise the
    /// error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addStringAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        bool           forceShow = false,
        MObject*       attribute = 0);

    /// \brief  add a new file path attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  fileFilter a file filter of the form:
    ///           "USD Files (*.usd*) (*.usd*);;Alembic Files (*.abc) (*.abc);;All files (*.*)
    ///           (*.*)"
    /// \param  fileMode  an enum that determines whether the GUI should display a file open dialog,
    /// file save, or directory dialog. \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addFilePathAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        FileMode       fileMode,
        const char*    fileFilter = "",
        MObject*       attribute = 0);

    /// \brief  add a new integer attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addInt8Attr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        int8_t         defaultValue,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new integer attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addInt16Attr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        int16_t        defaultValue,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new integer attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addInt32Attr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        int32_t        defaultValue,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new integer attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addInt64Attr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        int64_t        defaultValue,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new floating point attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addFloatAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        float          defaultValue,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new double attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addDoubleAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        double         defaultValue,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new time attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addTimeAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        const MTime&   defaultValue,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new time attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addDistanceAttr(
        const MObject&   node,
        const char*      longName,
        const char*      shortName,
        const MDistance& defaultValue,
        uint32_t         flags,
        MObject*         attribute = 0);

    /// \brief  add a new time attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addAngleAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        const MAngle&  defaultValue,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new boolean attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addBoolAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        bool           defaultValue,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new float3 attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultX  the default value for the attribute
    /// \param  defaultY  the default value for the attribute
    /// \param  defaultZ  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    AL_MAYA_UTILS_PUBLIC
    static MStatus addFloat3Attr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        float          defaultX,
        float          defaultY,
        float          defaultZ,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new float3 attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultX  the default value for the attribute
    /// \param  defaultY  the default value for the attribute
    /// \param  defaultZ  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    AL_MAYA_UTILS_PUBLIC
    static MStatus addAngle3Attr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        float          defaultX,
        float          defaultY,
        float          defaultZ,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new point attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addPointAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        const MPoint&  defaultValue,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new float point attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    static MStatus addFloatPointAttr(
        const MObject&     node,
        const char*        longName,
        const char*        shortName,
        const MFloatPoint& defaultValue,
        uint32_t           flags,
        MObject*           attribute = 0)
    {
        return addFloat3Attr(
            node,
            longName,
            shortName,
            defaultValue.x,
            defaultValue.y,
            defaultValue.z,
            flags,
            attribute);
    }

    /// \brief  add a new vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addVectorAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        const MVector& defaultValue,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new float vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    static MStatus addFloatVectorAttr(
        const MObject&      node,
        const char*         longName,
        const char*         shortName,
        const MFloatVector& defaultValue,
        uint32_t            flags,
        MObject*            attribute = 0)
    {
        return addFloat3Attr(
            node, longName, shortName, defaultValue.x, defaultValue.y, defaultValue.z, flags);
    }

    /// \brief  add a new mesh attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MObject addMeshAttr(const char* longName, const char* shortName, uint32_t flags);

    /// \brief  add a new colour attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    static MStatus addColourAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        const MColor&  defaultValue,
        uint32_t       flags,
        MObject*       attribute = 0)
    {
        return addFloat3Attr(
            node,
            longName,
            shortName,
            defaultValue.r,
            defaultValue.g,
            defaultValue.b,
            flags | kColour,
            attribute);
    }

    /// \brief  add a new matrix attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addMatrixAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        const MMatrix& defaultValue,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new matrix attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addMatrix2x2Attr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        const float    defaultValue[2][2],
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new matrix attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  defaultValue  the default value for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addMatrix3x3Attr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        const float    defaultValue[3][3],
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new data attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  type the data type for the attribute \param
    /// behaviour optionally control what happens when the attribute is disconnected \param  node
    /// the node to add the attribute to \param  attribute an optional pointer to an MObject in
    /// which the attribute handle will be returned \return MS::kSuccess when succeeded, otherwise
    /// the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addDataAttr(
        const MObject&                   node,
        const char*                      longName,
        const char*                      shortName,
        MFnData::Type                    type,
        uint32_t                         flags,
        MFnAttribute::DisconnectBehavior behaviour = MFnAttribute::kNothing,
        MObject*                         attribute = 0);

    /// \brief  add a new data attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  type the data type for the attribute \param
    /// behaviour optionally control what happens when the attribute is disconnected \param  node
    /// the node to add the attribute to \param  attribute an optional pointer to an MObject in
    /// which the attribute handle will be returned \return MS::kSuccess when succeeded, otherwise
    /// the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addDataAttr(
        const MObject&                   node,
        const char*                      longName,
        const char*                      shortName,
        const MTypeId&                   type,
        uint32_t                         flags,
        MFnAttribute::DisconnectBehavior behaviour = MFnAttribute::kNothing,
        MObject*                         attribute = 0);

    /// \brief  add a new message attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addMessageAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new 2D floating point vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    static MStatus addVec2hAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        MObject*       attribute = 0)
    {
        return addVec2fAttr(node, longName, shortName, flags);
    }

    /// \brief  add a new 2D floating point vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addVec2fAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new 2D integer vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addVec2iAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new 2D double precision vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addVec2dAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new 3D floating point vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    static MStatus addVec3hAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        MObject*       attribute = 0)
    {
        return addVec3fAttr(node, longName, shortName, flags, attribute);
    }

    /// \brief  add a new 3D floating point vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addVec3fAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new 3D integer vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addVec3iAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new 3D double precision vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addVec3dAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new 4D floating point vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    static MStatus addVec4hAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        MObject*       attribute = 0)
    {
        return addVec4fAttr(node, longName, shortName, flags, attribute);
    }

    /// \brief  add a new 4D floating point vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addVec4fAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new 4D integer vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addVec4iAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        MObject*       attribute = 0);

    /// \brief  add a new 4D double precision vector attribute to this node type.
    /// \param  longName  long name for the attribute
    /// \param  shortName  short name for the attribute
    /// \param  flags  a bitfield containing a mask of the AttributeFlags enumeration. Describes if
    /// the attribute is an input/output/etc \param  node the node to add the attribute to \param
    /// attribute an optional pointer to an MObject in which the attribute handle will be returned
    /// \return MS::kSuccess when succeeded, otherwise the error code
    AL_MAYA_UTILS_PUBLIC
    static MStatus addVec4dAttr(
        const MObject& node,
        const char*    longName,
        const char*    shortName,
        uint32_t       flags,
        MObject*       attribute = 0);

private:
    AL_MAYA_UTILS_PUBLIC
    static MStatus applyAttributeFlags(MFnAttribute& fn, uint32_t flags);
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
