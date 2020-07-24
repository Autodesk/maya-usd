# Developer

## Maya Helper Classes

#### lib/AL_USDMaya/AL/maya
 contains a large number of non-USD specific Maya helper and convenience classes for example:

- Common.h: mostly macros for shortening the amount of boilerplate code we need
- CommandGuiHelper.h: Automatically generate a simple UI from MpxCommands
- DGNodeHelper.h - get/set types on DG Nodes
- FileTranslatorBase.h - macros and templates to help automate the boiler plate setup of Maya import/export plugins     
- FileTranslatorOptions.h - Utility class that parses the file translator options passed through by Maya
- MenuBuilder.h - generate Menu entries for an entire Plugin
- NodeHelper.h - various utility functions for adding static/dynamic attributes to an MpxNode, get data from a dataBlock, AETemplate Generators etc.

Some of the modules are not even  maya dependent:
- CodeTimings.h - Profiling
- SIMD.h - low-level SSE/AVX routines

more details [here](https://animallogic.github.io/AL_USDMaya/group__mayautils.html)

## Adding Maya Nodes

Adding custom Maya nodes via the Maya API is an experience laden with boilerplate code, and general misery. To help speed up this process, and to help autogenerate tedious-to-write AE templates, the class al::alNodeHelper can be used to make life a little easier. The best way to explain how this code works, is to simply walk through a very basic example

```cpp
/// This will be a very basic node that has 1 input, and 1 output. 
/// The compute will simply pass the input float through to the output. 
class MyMayaNode : public MPxNode, AL::maya::NodeHelper
{
public:
  // this adds a creator method, kTypeId, and kTypeName
  AL_DECLARE_NODE();

  // declare the attributes (more on that in a bit). 
  AL_DECL_ATTRIBUTE(inFloat);
  AL_DECL_ATTRIBUTE(outFloat);

  // and we'll add a compute method, because we can!
  MStatus compute(const MPlug& plug, MDataBlock& dataBlock) override;
};
```

To understand what this ends up generating, the macros above will expand into the following:

```cpp
/// This will be a very basic node that has 1 input, and 1 output. 
/// The compute will simply pass the input float through to the output. 
class MyMayaNode : public MPxNode, AL::maya::NodeHelper
{
public:
  //-----------------------------
  // AL_DECLARE_NODE();
  //-----------------------------
  static void* creator();
  static MStatus initialise();
  static const MString kTypeName;
  static const MTypeId kTypeId;
  //-----------------------------

  //-----------------------------
  // AL_DECL_ATTRIBUTE(inFloat);
  //-----------------------------
  private:
    static MObject m_inFloat;
  public:
    MPlug inFloatPlug() const;
    static const MObject& inFloat();
  //-----------------------------

  //-----------------------------
  // AL_DECL_ATTRIBUTE(outFloat);
  //-----------------------------
  private:
    static MObject m_outFloat;
  public:
    MPlug outFloatPlug() const;
    static const MObject& outFloat();
  //-----------------------------

  MStatus compute(const MPlug& plug, MDataBlock& dataBlock) override;
};
```

In our cpp file then, we will need to do the following. First, let's add this line:

```cpp
AL_MAYA_DEFINE_NODE(MyMayaNode, 0x12345);
```

Which actually expands into:

```cpp
// the typename for the node
const MString MyMayaNode::kTypeName("MyMayaNode");

// the unique type ID of the node
const MTypeId MyMayaNode::kTypeId(0x12345);

// a very simple creator function for the node 
void* MyMayaNode::creator()
{
  return new MyMayaNode;
}
```

You may notice that in the class definition, the AL_MAYA_DECLARE_NODE macro added an initialise function, however it's missing from the AL_MAYA_DEFINE_NODE macro. Also missing from the cpp file, are the static MObjects that the AL_DECL_ATTRIBUTE macros added. So let's go add in the missing bits (without an implementation as yet!)

```cpp
// the handles to the attribute definitions
MObject MyMayaNode::m_inFloat;
MObject MyMayaNode::m_outFloat;

// this will decsribe the attributes on our node
MStatus MyMayaNode::initialise()
{
  // to do
}

// this will implement the compute method.
MStatus MyMayaNode::compute(const MPlug& plug, MDataBlock& dataBlock)
{
  // to do
}
```

Typically when using the Maya API, you'll start off by adding each attribute by creating some attribute function set (e.g. MFnNumericAttribute, MFnUnitAttribute, etc), and then calling a myriad of methods such as setArray, setReadable, setWritable, etc. This is tedious, and the error-per-lines-of-code ratio can be high!


```cpp
MStatus MyMayaNode::initialise()
{
  // the node helper tests *every* return argument from Maya, so you don't have
  // to. Just wrap the initialise in a try/catch, and if anything goes wrong 
  // it will jump into the catch block with an MStatus
  try
  {
    // make sure you add this line first! 
    // The node helper will generate the AE template automatically, 
    // in a function called "AEMyMayaNodeTemplate". We need to tell
    // the node helper that the node type is "MyMayaNode", otherwise
    // it won't be able to generate a meaningful name!
    setNodeType(kTypeName);

    // Next we will add a UI Frame, that will have the title 
    // "My Maya Node Controls". All attributes that appear 
    // under this frame, will be added to that UI control in the 
    // Attribute Editor. (unless they are hidden, or outputs)
    addFrame("My Maya Node Controls");

    // Now lets add a couple of float attributes. The arguments are:
    //   - the long name
    //   - the short name
    //   - the default value
    //   - a collection of flags (described in a section below!)
    m_inFloat  = addFloat("inputFloat", "if", 0.0f, kReadable | kWritable | kStorable | kConnectable);
    m_outFloat = addFloat("outputFloat", "of", 0.0f, kReadable | kConnectable);

    // now lets set up the dependency between the input and the output attribute
    attributeAffects(m_inFloat, m_outFloat);
  }
  catch (const MStatus& status)
  {
    return status;
  }

  // Our node successfully initialised! Woot!
  // All that's left is to generate the attribute editor template. 
  generateAETemplate();

  return MS::kSuccess;
}
```

Whilst this simple example only calls addFloat, there are a myriad of other methods available to add integers, vectors, matrices, etc. If you wish to add array attributes, simply specify the kArray flag. 

The possible flags are as follows:

```cpp
    // the attribute value will be cached
    kCached

    // the attribute can be read via getAttr
    kReadable

    // the attribute can be set via setAttr
    kWritable

    // the attribute will be saved into the maya ascii/binary files
    kStorable

    // Only for use on SurfaceShapes. When changed, the shape will look different
    kAffectsAppearance

    // the attribute can be keyframed
    kKeyable

    // the attribute can be connected to others
    kConnectable

    // the attribute is an array
    kArray

    // if the attribute is a vector 3, display a colour picker in the GUI
    kColour

    // the attribute will not appear in the UI
    kHidden

    // the data for the attribute will be stored as a member variable. 
    // you should overload MPxNode::getInternalValueInContext and 
    // MPxNode::setInternalValueInContext to get/set the value. 
    kInternal

    // if the node is an MPxTransform, this attribute will affect the 
    // world space matrix of the transform.
    kAffectsWorldSpace

    // if the attribute is an array, an MArrayDataBuilder can be used to resize 
    // the array within a compute function. 
    kUsesArrayDataBuilder
```

And finally, there are a few helper methods to simplify getting and setting values within the nodes compute function. They add some standardised error reporting, and will set the output plugs as clean. 

```cpp
MStatus MyMayaNode::compute(const MPlug& plug, MDataBlock& dataBlock)
{
  if(plug == outFloat()) {
    return outputFloatValue(dataBlock, outFloat(), inputFloatValue(dataBlock, inFloat()));
  }
  // not my param
  return MS::kInvalidParameter;
}
```

## Adding Maya Commands

To add a new MEL command:

```cpp
class MyCommand : public MPxCommand
{
public:
  AL_MAYA_DECLARE_COMMAND();
  MStatus doIt(const MArgList& args) override;
}
```

The AL_MAYA_DECLARE_COMMAND macro expands into the following:

```cpp
  static const char* const g_helpText;
  static void* creator();
  static MSyntax createSyntax();
  static const MString kName;
```

In your cpp file, add something similar to:

```cpp
AL_MAYA_DEFINE_COMMAND(MyCommand);

const char* const MyCommand::g_helpText = R"(
MyCommand Overview:

  I am some information about how the command should be used.
)";

MSyntax MyCommand::createSyntax()
{
  MSyntax syn;
  syn.addFlag("-h", "-help", MSyntax::kNoArg);
  syn.addFlag("-sa", "-someArg", MSyntax::kString);
  return syn;
}

MStatus MyCommand::doIt(const MArgList& argList)
{
  MStatus status;
  MArgDatabase database(syntax(), args, &status);
  if(!status)
    return status;
  AL_COMMAND_HELP(args, g_helpText);

  // test for any flags
  MString someArg = "defaultValue";
  if(args.isFlagSet("-sa"))
  {
    args.getFlagArgument("-sa", 0, someArg);
  }

  // now do the actual commandy stuff

  return MS::kSuccess;
}
```

This will now provide you with a command (which effectively does nothing). Typically within Maya, most commands are callable from a menu item somewhere (typically with an option box, that will store values in userPrefs.mel via the optionVar command). To add a GUI of that sort, make use of the AL::maya::CommandGuiHelper class, e.g. 

```cpp
void constructMyCommandUI()
{
  AL::maya::CommandGuiHelper MyCommandUI("MyCommand", "My Command", "Run", "Menu/Path/My Command", true);
  MyCommandUI.addStringOption("sa", "Some Argument");
}
```

The menu to the item "Menu/Path/My Command" will automatically be created with an option box. The first argument to the constructor is the name of the command itself (which will be 'MyCommand' in this case). The second string is the window title on the option box, and the third is the label on the 'action' button (e.g. "create", "ok", "yes", etc). If the final argument is true, then an option box will be created (and running the command without the optionbox, will use the last set preferences). If the argument is false, a GUI will always be displayed. 

For any arguments that are specified to your command, just add a simple option for each one. The GUI will automatically be specified. 

To register your command, within your initialise just do:

```cpp
  // register the command
  status = fnPlugin.registerCommand(
      MyCommand::kName,
      MyCommand::creator,
      MyCommand::createSyntax);
  if(!status) {
    status.perror("unable to register command MyCommand");
    return status;
  }

  // build the GUI
  constructMyCommandUI();

  // call this ONCE per plugin. 
  AL::maya::MenuBuilder::generatePluginUI(fnPlugin, "somePrefix");
```

The AL::maya::MenuBuilder will construct any menus that were specified when creating the command GUI's. It requires some user defined prefix to avoid the generated code from clashing with other autogenerated GUI's from other plugins. In the case of USD maya, this is currently "alusd". 

## Logging

To help understand what happens we use TF_DEBUG quite extensively and one can turn it on with specific debug flags:
```
export TF_DEBUG="ALUSDMAYA* -ALUSDMAYA_EVALUATION -ALUSDMAYA_COMMANDS"
```

Existing TF_DEBUG flags are:
* ALUSDMAYA_TRANSLATORS
* ALUSDMAYA_EVENTS
* ALUSDMAYA_EVALUATION
* ALUSDMAYA_COMMANDS
* ALUSDMAYA_LAYERS
* ALUSDMAYA_DRAW
* ALUSDMAYA_SELECTION
* ALUSDMAYA_RENDERER

Within Maya itself, the MEL command AL_usdmaya_UsdDebugCommand allows you to list, disable, enable, and query the debug notices throughout USD and AL_USDMaya. 
To query the list of available notices, simply execute:

```c++   
print `AL_usdmaya_UsdDebugCommand -ls`;
```
To enable a particular notice:

```c++
AL_usdmaya_UseDebugCommand -en "ALUSDMAYA_TRANSLATORS";
```

To query the state of a particular notice, use the -st flag:

```c++
if(`AL_usdmaya_UseDebugCommand -st "ALUSDMAYA_TRANSLATORS"`)
	print "ALUSDMAYA_TRANSLATORS is enabled\n";
else
	print "ALUSDMAYA_TRANSLATORS is disabled\n";
``` 
To disable a particular notice:

```c++
AL_usdmaya_UseDebugCommand -ds "ALUSDMAYA_TRANSLATORS";
```

It should also be noted that the ```USD->Debug->TfDebug Options``` menu item will bring up a GUI that allows you to enable/disable all registered notices.

If you wish to add additional notices for debugging C++, add the new entry within the DebugCodes.h and DebugCodes.cpp. It is then possible to format messages
targeting that new flag using the standard printf formatting:

```c++
TF_DEBUG(ALUSDMAYA_MY_CUSTOM_FLAG).Msg("Hello world, this is an int %d, and this is a string \"$s\"\n", 42, "hellllllo!");
``` 


##  running in batch mode
@todo add doc
