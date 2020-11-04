
/**

\page  translator_plugins  Custom Plugin Translators

The plug-in translator system that is the core of AL_USDMaya provides a number of ways to integrate
USD with existing maya nodes (including custom Maya plug-ins). Any particular type can be integrated
in one (or all) of these ways:

* File Import
* File Export
* Importing as part of an AL_usdmaya_ProxyShape
* Handling the changes to a AL_usdmaya_ProxyShape as a result of a variant switch

The API for defining your own plug-in translator may at first seems a little convoluted (especially
if you were expecting to override a pair of import/export methods), however this API has evolved
over time to ensure it works correctly with live variant switches, prim activation/de-activation,
and a number of other live changes to the underlying USD scene.

To try to explain how this all works, let's start of with an extremely silly plug-in example that
will create a custom translator plugin to represent a polygon cube node in Maya.

*PolyCubeNodeTranslator.h*
\code
#pragma once
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"

class PolyCubeNodeTranslator
  : public AL::usdmaya::fileio::translators::TranslatorBase
{
public:

  AL_USDMAYA_DECLARE_TRANSLATOR(PolyCubeNodeTranslator);

  MStatus initialize() override;

  bool needsTransformParent() override;
  MStatus writeEdits(MObject node, const UsdPrim& prim); ///< helper method
  MStatus import(const UsdPrim& prim, MObject& parent) override;
  MStatus postImport(const UsdPrim& prim) override;
  MStatus preTearDown(const UsdPrim& prim) override;
  MStatus update(const UsdPrim& prim) override;
  MStatus tearDown(const SdfPath& primPath) override;
  bool supportsUpdate() const override;
  bool importableByDefault() const override;

  // new to AL_USDMaya 0.29.0
  ExportFlag canExport(const MObject& obj) override;
  UsdPrim exportObject(UsdStageRefPtr stage, MDagPath dagPath, const SdfPath& usdPath, const
ExporterParams& params) override;

private:

  // handles to the polycube attributes
  MObject m_width;
  MObject m_height;
  MObject m_depth;
  MObject m_subdivisionsWidth;
  MObject m_subdivisionsHeight;
  MObject m_subdivisionsDepth;
  MObject m_outputMesh;

  // handles to the mesh attributes
  MObject m_inputMesh;
};
\endcode

As an absolute minimum, you'll need to implement the update and tearDown methods. The following is
an explanation of what those methods do, and how to implement them correctly.


\b General \b Setup

*PolyCubeNodeTranslator.cpp*
\code
#include "AL/usd/schemas/maya/PolyCube.h"  //< this is the custom schema you have created
#include "PolyCubeNodeTranslator.h"

// Some macro magic that generates some boiler plate code.
AL_USDMAYA_DEFINE_TRANSLATOR(PolyCubeNodeTranslator, AL_usd_PolyCube);
\endcode


\b initialize

The initialize method is a one time initialisation step for your translator plug-in. Now we all want
to ensure our plug-ins operate as quickly as possible right? So the initialize step is really to
help improve the performance when accessing data via MPlugs (i.e. Ideally we don't want to be
accessing MPlugs by constantly calling findPlug("someString"))

\code
MStatus PolyCubeNodeTranslator::initialize()
{
  // If you need to load any plugins at this point, feel free!
  // Luckily we don't need to load a plug-in for our poly cube.


  // assign a node class to the polyCube
  MNodeClass polyCube("polyCube");

  // now do a one time look up of the attributes. This now means we can access the attributes
directly
  // without needing to call MFnDependencyNode::findPlug() (and the string compares that implies!)
  m_width = polyCube.attribute("width");
  m_height = polyCube.attribute("height");
  m_depth = polyCube.attribute("depth");
  m_subdivisionsWidth = polyCube.attribute("subdivisionsWidth");
  m_subdivisionsHeight = polyCube.attribute("subdivisionsHeight");
  m_subdivisionsDepth = polyCube.attribute("subdivisionsDepth");
  m_outputMesh = polyCube.attribute("output");

  // assign a node class to mesh
  MNodeClass mesh("mesh");
  m_inputMesh = mesh.attribute("input");


  // Now obviously you're a careful developer, and have been checking all MStatus values in the code
above right???
  // Just returning success isn't something we're advocating here, it's just a silly tutorial after
all! return MS::kSuccess;
}
\endcode


\b needsTransformParent

One function you may want to overload is the needsTransformParent() function.

\code
bool PolyCubeNodeTranslator::needsTransformParent() const
{
  return true;
}
\endcode

If your node is a DAG node (i.e. a shape or custom transform), it will need to have a transform
created for it, so return true in this case. If however you node is a simple DG node (e.g. surface
shader, texture etc), then you should return false from this method.

If you return true, a new transform will be generated within Maya to which you can parent your shape
on creation. If you return false, no transform will be generated.


\b import

The Import method should only *really* be used to create the Maya nodes that will represent your
custom prim. Now there is a small caveat to this. If the contents of your prim does not have any
relationships to other prims in the stage, then you may as well do all of the setup you need within
Import.

This example will create a simple polyCubeCreator node, a mesh, and connect them together. To do
this will not require information from any other prim (for example, if there was another prim that
contained a surface material, or a mesh deformation, then there would be a second step involved here
to make those relationships in the Maya DG).

\code
MStatus PolyCubeNodeTranslator::import(const UsdPrim& prim, MObject& parent)
{
  MFnDependencyNode fnDep;
  MFnDagNode fnDag;

  // create the two maya nodes we need (parent the shape under the transform node that's been
created for us) MObject oPolyCube = fnDep.createNode(); MObject oMesh = fnDag.createNode("mesh",
parent);

  // we need to register the nodes we create with the
  context()->insertItem(prim.GetPath(), oPolyCube);
  context()->insertItem(prim.GetPath(), oMesh);

  float width = 1.0f;
  float height = 1.0f;
  float depth = 1.0f;
  int32_t subdivisionsWidth = 1;
  int32_t subdivisionsHeight = 1;
  int32_t subdivisionsDepth = 1;

  // Now gather the parameters from the schema node
  AL_usd_PolyCube schema(prim);
  if(auto attr = schema.GetWidthAttr()) attr.Get(&width);
  if(auto attr = schema.GetHeightAttr()) attr.Get(&height);
  if(auto attr = schema.GetDepthAttr()) attr.Get(&depth);
  if(auto attr = schema.GetSubdivisionsWidthAttr()) attr.Get(&subdivisionsWidth);
  if(auto attr = schema.GetSubdivisionsHeightAttr()) attr.Get(&subdivisionsHeight);
  if(auto attr = schema.GetSubdivisionsDepthAttr()) attr.Get(&subdivisionsDepth);

  // set the values on the poly cube creator node
  MPlug(oPolyCube, m_width).setValue(width);
  MPlug(oPolyCube, m_height).setValue(height);
  MPlug(oPolyCube, m_depth).setValue(depth);
  MPlug(oPolyCube, m_subdivisionsWidth).setValue(subdivisionsWidth);
  MPlug(oPolyCube, m_subdivisionsHeight).setValue(subdivisionsHeight);
  MPlug(oPolyCube, m_subdivisionsDepth).setValue(subdivisionsDepth);

  // please check errors, and don't just return success! :)
  return MS::kSuccess;
}
\endcode

\b Post \b Import

Having generated all of the nodes you need to, you might end up needing to hook those nodes to other
prims. This is admittedly a bit of a bad example (because in this case the node connections could
have all been made within import itself).

However, in cases where the scene involves relationships between prims (e.g. one prim is a material,
the other is the shape), it won't be possible to make those connections within import (because the
other Maya node may not have been created yet). In those cases, you will need to make use of the
postImport method to perform the connection of the maya nodes to other prims.

\code
MStatus PolyCubeNodeTranslator::postImport(const UsdPrim& inputPrim, MObject& parent)
{
  // Previously we created two Maya nodes for our inputPrim (the mesh and the polycube).
  // Whenever you need to retrieve those Maya nodes, you can retrieve them from the translator
context
  // by passing the prim, and the type of node you are searching for, into the getMObject function.
  //
  // If you have a situation where your inputPrim has a relationship to another prim (e.g. the other
prim
  // may be a surface material, geometry deformer, etc), and you wish to extract the MObject for
that related
  // prim, then just pass the related prim in as the first argument, and it will be returned to you.

  MObjectHandle handleToMesh;
  if(!context()->getMObject(inputPrim, handleToMesh, MFn::kMesh))
  {
    MGlobal::displayError("unable to locate mesh");
    return MS::kFailure;
  }

  MObjectHandle handleToPolyCube;
  if(!context()->getMObject(inputPrim, handleToPolyCube, MFn::kPolyCube))
  {
    MGlobal::displayError("unable to locate polycube");
    return MS::kFailure;
  }

  // now connect the output of the polycube to the input of the mesh.
  MDGModifier mod;
  mod.connect(MPlug(handleToPolyCube.object(), m_outputMesh), MPlug(handleToMesh.object(),
m_inputMesh)); mod.doIt();

  // please check and log any errors rather than simply returning success!
  return MS::kSuccess;
}
\endcode


\b Variant \b Switching

If you've only supported the methods previously discussed, then your custom prim type should now be
imported when you load a usd scene with the proxy shape.

If however you want to be able to respond to variant switches, and swap in or out nodes as a result,
there is a little bit more work to do.

When a variant is switched, the proxy shape intercepts an event generated by USD that indicates that
a variant is about to switch on a specific prim. At this point, the AL maya plugin will traverse the
hierarchy under the prim on which the variant switched, and call a preTearDown() method. This method
can be used to copy any values from your maya nodes into a layer within the usd stage.

\code
// This is just a helper method (to avoid some code duplication later on)
// It simply reads the node values from Maya, and sets writes those values
// to the USD file
MStatus PolyCubeNodeTranslator::writeEdits(MObject oPolyCube, UsdPrim& prim)
{
  float width = 1.0f;
  float height = 1.0f;
  float depth = 1.0f;
  int32_t subdivisionsWidth = 1;
  int32_t subdivisionsHeight = 1;
  int32_t subdivisionsDepth = 1;

  // get the values from the poly cube creator node
  MPlug(oPolyCube, m_width).getValue(width);
  MPlug(oPolyCube, m_height).getValue(height);
  MPlug(oPolyCube, m_depth).getValue(depth);
  MPlug(oPolyCube, m_subdivisionsWidth).getValue(subdivisionsWidth);
  MPlug(oPolyCube, m_subdivisionsHeight).getValue(subdivisionsHeight);
  MPlug(oPolyCube, m_subdivisionsDepth).getValue(subdivisionsDepth);

  // Now set the parameters on the schema node (if they differ from the defaults)
  AL_usd_PolyCube schema(prim);
  if(width != 1.0f) schema.CreateWidthAttr().Set(width);
  if(height != 1.0f) schema.CreateHeightAttr().Set(height);
  if(depth != 1.0f) schema.CreateDepthAttr().Set(depth);
  if(subdivisionsWidth != 1) schema.CreateSubdivisionsWidthAttr().Set(subdivisionsWidth);
  if(subdivisionsHeight != 1) schema.CreateSubdivisionsHeightAttr().Set(subdivisionsHeight);
  if(subdivisionsDepth != 1) schema.CreateSubdivisionsDepthAttr().Set(subdivisionsDepth);

  // please check errors, and don't just return success! :)
  return MS::kSuccess;
}

// This method is called prior to a variant switch occurring (that may destroy our node)
MStatus PolyCubeNodeTranslator::preTearDown(UsdPrim& prim)
{
  MObjectHandle handleToPolyCube;
  if(!context()->getMObject(prim, handleToPolyCube, MFn::kPolyCube))
  {
    MGlobal::displayError("unable to locate polycube");
    return MS::kFailure;
  }

  return writeEdits(handleToPolyCube.object(), prim);
}
\endcode

After the variant switch has occurred, the AL USD plugin will do a quick sanity check comparing the
prims that were there previously, and the ones that are there now.

For each prim, if a corresponding prim still exists after the variant switch, AND the prim type is
the same, then it call an update() method on your translator. Adding this method is optional,
however it can improve the speed of a variant switch, so it is recommended!

If you wish to provide an update method to your translator, you will first need to opt in to this
mechanism. By returning true from supportsUpdate (by default it returns false), you will now be able
to provide a slightly quicker way for handling prims that do not change as a result of the switch.
If however you return false here, your node will always be destroyed (via tear down), before being
re-imported.

\code
bool PolyCubeNodeTranslator::supportsUpdate() const
{
  return true;
}
\endcode

Once you have notified AL usd maya that your translator can update, simply provide your update
function (which should simply copy the values from the prim and onto the maya nodes you previously
created)

\code
MStatus PolyCubeNodeTranslator::update(const UsdPrim& prim)
{
  MObjectHandle handleToPolyCube;
  if(!context()->getMObject(prim, handleToPolyCube, MFn::kPolyCube))
  {
    MGlobal::displayError("unable to locate polycube");
    return MS::kFailure;
  }

  MObject oPolyCube = handleToPolyCube.object();

  float width = 1.0f;
  float height = 1.0f;
  float depth = 1.0f;
  int32_t subdivisionsWidth = 1;
  int32_t subdivisionsHeight = 1;
  int32_t subdivisionsDepth = 1;

  // Now gather the parameters from the schema node
  AL_usd_PolyCube schema(prim);
  if(auto attr = schema.GetWidthAttr()) attr.Get(&width);
  if(auto attr = schema.GetHeightAttr()) attr.Get(&height);
  if(auto attr = schema.GetDepthAttr()) attr.Get(&depth);
  if(auto attr = schema.GetSubdivisionsWidthAttr()) attr.Get(&subdivisionsWidth);
  if(auto attr = schema.GetSubdivisionsHeightAttr()) attr.Get(&subdivisionsHeight);
  if(auto attr = schema.GetSubdivisionsDepthAttr()) attr.Get(&subdivisionsDepth);

  // set the values on the poly cube creator node
  MPlug(oPolyCube, m_width).setValue(width);
  MPlug(oPolyCube, m_height).setValue(height);
  MPlug(oPolyCube, m_depth).setValue(depth);
  MPlug(oPolyCube, m_subdivisionsWidth).setValue(subdivisionsWidth);
  MPlug(oPolyCube, m_subdivisionsHeight).setValue(subdivisionsHeight);
  MPlug(oPolyCube, m_subdivisionsDepth).setValue(subdivisionsDepth);

  // please check errors, and don't just return success! :)
  return MS::kSuccess;
}
\endcode

Now the eagle eyed reader may notice that the above function looks very similar to the import()
function we initially wrote. To save yourself from a boiler plate code explosion, one option would
be to simply call update from import:

\code
MStatus PolyCubeNodeTranslator::import(const UsdPrim& prim, MObject& parent)
{
  MFnDependencyNode fnDep;
  MFnDagNode fnDag;

  // create the two maya nodes we need (parent the shape under the transform node that's been
created for us) MObject oPolyCube = fnDep.createNode(); MObject oMesh = fnDag.createNode("mesh",
parent);

  // we need to register the nodes we create with the
  context()->insertItem(prim.GetPath(), oPolyCube);
  context()->insertItem(prim.GetPath(), oMesh);

  // Just call update to set the parameters!
  return update(prim);
}
\endcode

Now, if the variant switch results in the prim type changing, or the prim being removed, then a
final method will be called, which is tearDown. The simplest implementation of this method is the
following:

\code
MStatus PolyCubeNodeTranslator::tearDown(const SdfPath& primPath)
{
  // delete all the maya nodes currently associated with the prim path
  context()->removeItems(primPath);
  return MStatus::kSuccess;
}
\endcode

In most cases that is probably enough. In some cases however, there may be times when you need to
ensure the nodes are deleted in a specific order, or you have some other book keeping exercise to
perform. Feel free to do so here!

It should be noted that whilst preTearDown and update are optional, tearDown is NOT. You must
implement this method in order to support variant switching!

\b Importable \b by \b Default

When a USD file is imported into a proxy shape node, if you \a always want that node to be imported
immediately, then you should return true from the importableByDefault method (which is the default).
This will cause the translator to be run as soon as the matching prim type has been encountered. In
some cases, you might not want those prims to be immediately imported. One example of this is with
mesh data.

If you are importing a very geometry heavy scene with a large number of dense meshes, you would want
to keep those meshes within USD/Hydra for as long as possible for performance reasons. If you return
false from importableByDefault, then that particular node type can only be manually imported by
calling the AL_usdmaya_TranslatePrim command. This means that importing and displaying the data will
be quick by default, however if you need to make modifications to that particular prim, you'll be
able to selectively import the data when needed.

\code
bool PolyCubeNodeTranslator::importableByDefault() const
{
  return false;
}
\endcode

\b canExport

If you wish to provide support for the standard file export operations (e.g. export some Maya data
as a USD file), then there is a two step process you need to adhere to. The first step is to
determine whether this translator can handle the export of a given Maya node, and the second step is
to implement the actual export of that data.

\code
ExportFlag PolyCubeNodeTranslator::canExport(const MObject& obj)
{
  // test to see if we have a mesh object
  if(obj.hasFn(MFn::kMesh))
  {
    // check the 'input' attr to see if it connected to a polyCube node
    MPlug input(obj, m_inputMesh);
    MPlugArray plugs;
    input.connectedTo(plugs, true, false);
    if(plugs.length() > 0 && plugs[0].node().hasFn(MFn::kPolyCube))
    {
      // If it is, return 'kSupported'. By default, all translators shipped
      // with the OSS build of AL_USDMaya return 'kFallbackSupport' if supported.
      // kSupported always takes precedence over kFallbackSupport, which means
      // you can override the default support without your studio.
      return ExportFlag::kSupported;
    }
  }
  return ExportFlag::kNotSupported;
}
\endcode

\b exportObject

Finally, if the object can be exported by your translator, and it is the best translator available,
then AL_USDMaya will call the exportObject method to export the data into USD. It's worth noting
that the preTearDown and exportObject methods are likely to share a significant amount of code,
hence the reason for utilising a common 'writeEdits' method.

\code
UsdPrim PolyCubeNodeTranslator::exportObject(
  UsdStageRefPtr stage,
  MDagPath dagPath,
  const SdfPath& usdPath,
  const ExporterParams& params)
{
  // check the 'input' attr to see if it connected to a polyCube node
  MPlug input(dagPath.node(), m_inputMesh);
  MPlugArray plugs;
  input.connectedTo(plugs, true, false);
  if(plugs.length() > 0 && plugs[0].node().hasFn(MFn::kPolyCube))
  {
    // create the new USD prim
    AL_usd_PolyCube cube = AL_usd_PolyCube::Define(stage, usdPath);

    // copy values into the cube prim
    writeEdits(plugs[0].node(), cube.GetPrim());
    return cube.GetPrim();
  }
  return UsdPrim();
}
\endcode

*/
