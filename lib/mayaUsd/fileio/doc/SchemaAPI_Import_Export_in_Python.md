# SchemaAPI import export

There are many layers of schema API support in the MayaUSD import/export code. Let's review them from simplest to the most complex.

## SchemaAPI with no corresponding attributes in the Maya scene model:

These schemas can be created, exported, and imported using the base UsdMayaSchemaAdaptor.

### Applying a schema:

You can use the Adaptor class to create schemas and attributes on supported Maya primitive types. In the following example, we will add the GeomModelAPI on a Maya sphere and specify a "cross" cardGeometry mode.

```python
import mayaUsd.lib as mayaUsdLib
from maya import cmds
cmds.loadPlugin("mayaUsdPlugin")

sphereXform = cmds.polySphere()[0]
adaptor = mayaUsdLib.Adaptor(sphereXform)

# Apply GeomModelAPI to the sphere:
geomModelAPI = adaptor.ApplySchemaByName("GeomModelAPI")
print(geomModelAPI.GetAttributeNames())
# result: ['model:applyDrawMode', 'model:cardGeometry', 'model:cardTextureXNeg'...
print(geomModelAPI.GetAuthoredAttributeNames())
# result: []

# Set cardGeometry to "cross"
cardGeometryAttribute = geomModelAPI.CreateAttribute("model:cardGeometry")
cardGeometryAttribute.Set("cross")
print(geomModelAPI.GetAuthoredAttributeNames())
# result: ['model:cardGeometry']
print(cardGeometryAttribute.Get())
# result: cross
```

### Export the schema:

You need to explicitly specify the list of schema APIs to export using the `apiSchema` export option. So since we added a GeomModelAPI to that scene we will need:

```python
# Export: 
cmds.mayaUSDExport(mergeTransformAndShape=True, file="geom_api_on_sphere.usda", apiSchema=["GeomModelAPI"])
```

If we look in the resulting USD file, we will see that the schema API was exported:

```
def Mesh "pSphere1" (
    prepend apiSchemas = ["GeomModelAPI", "MaterialBindingAPI"]
)
{
    // Geometry data
    
    // GeomModelAPI:
    uniform token model:cardGeometry

    // More geometry data
}
```

### Import the schema:

This also requires an explicit list of schemas to import. Let's try re-importing the scene:

```python
# Import
cmds.file(new=True, f=True)
cmds.mayaUSDImport(file="geom_api_on_sphere.usda", apiSchema=["GeomModelAPI"])

# Check that the schema is still there:
adaptor = mayaUsdLib.Adaptor("pSphere1")
print(adaptor.GetAppliedSchemas())
# result: ['GeomModelAPI']

# Get the GeomModelAPI of the sphere:
geomModelAPI = adaptor.GetSchemaByName("GeomModelAPI")
print(geomModelAPI.GetAuthoredAttributeNames())
# result: ['model:cardGeometry']

# Get cardGeometry
cardGeometryAttribute = geomModelAPI.GetAttribute("model:cardGeometry")
print(cardGeometryAttribute.Get())
# result: cross
```

### Under the hood:

If you open `pSphere1` in the attribute editor and expand the "Extra Attributes" section, you will notice two extra dynamic attributes called "apiSchemas" and "model:cardGeometry" that are used by Maya to keep track of the applied schemas and the current values of authored attributes.

## SchemaAPI *with* corresponding attributes in the Maya scene model:

Sometimes, we want the properties to live in the data model, for example, we could want to rewrite the lights ShadowAPI as an adaptor on top of the regular light API. This is possible using a specialization of the basic adaptor.

Let's create a ShadowAPI adaptor to be used interactively since the import export code already handles these APIs natively.

This specialization will map the `inputs:shadow:color` attribute of the USD `ShadowAPI` directly to the `shadowColor` attribute of a Maya `pointLight` geometry.

```python
class shadowApiAdaptorLightShape(mayaUsdLib.SchemaApiAdaptor):
    def CanAdapt(self):
        """Can this adaptor adapt the light? Can run some extra tests and reject
        if there are issues with the light."""
        node = om.MFnDependencyNode(self.mayaObject)
        print("Yes, I can adapt", node.name())
        return True
    
    def GetAdaptedAttributeNames(self):
        """Return the list of USD attribute names that are directly mapped to a Maya attribute"""
        return ["inputs:shadow:color", "inputs:shadow:enable"]

    def GetMayaNameForUsdAttrName(self, usdName):
        """Return the Maya attribute name that fits the USD attribute name."""
        if usdName == "inputs:shadow:color":
            return "shadowColor"
        if usdName == "inputs:shadow:enable":
            # Two Maya enablers. Return the RayTrace one.
            return "useRayTraceShadows"
```

We then register the class for use in Maya with:

```python
mayaUsdLib.SchemaApiAdaptor.Register(shadowApiAdaptorLightShape, "light", "ShadowAPI")
```

_A note on Python registrations_

Python registrations are very flexible and keep a reference to the last registered class object. If you are actively implementing or debugging a schema API adaptor, you only need to register it again for MayaUSD to use the latest version. If you no longer need an exporter class, you can deregister it using:
```python
mayaUsdLib.SchemaApiAdaptor.Unregister(shadowApiAdaptorLightShape, "light", "ShadowAPI")
```

Once the class is registered, you can start using it directly to read Maya attributes as if they were USD attributes:

```python
lightShape1 = cmds.pointLight()
cmds.setAttr(lightShape1 + ".shadowColor", 0.5, 0.25, 0)

adaptor = mayaUsdLib.Adaptor(lightShape1)
print(adaptor.GetAppliedSchemas())
# result: Yes, I can adapt pointLightShape1
# result: ["ShadowAPI"]

schema = adaptor.GetSchemaByName("ShadowAPI")
# result: Yes, I can adapt pointLightShape1

print(schema.GetAuthoredAttributeNames())
# result: ["inputs:shadow:color", "inputs:shadow:enable"]

shadowColor = schema.GetAttribute("inputs:shadow:color")
print(shadowColor.Get())
# result: (0.21763764, 0.047366142, 0)
print(mayaUsdLib.ConvertMayaToLinear((0.5, 0.25, 0))
# result: (0.21763764, 0.047366142, 0)

shadowColor.Set((1,0,0))
print(cmds.getAttr("pointLightShape1.shadowColor"))
# result: [(1.0, 0.0, 0.0)]
```

If the `apiSchema` list on the export includes "ShadowAPI" and this custom adaptor is found while exporting, the values saved in the USD file will come from the Maya attributes. Of course the native exporters already process the shadow API, so this example is not for real life.

The reason the shadow color values looked a bit dim is because they were automatically gamma corrected. This happens on all color attributes.

You also saw that we explicitly chose `useRayTraceShadows` to represent `inputs:shadow:enable`, leaving `useDepthMapShadows` unaccounted for. The current API does not allow controlling multiple plugs from a single USD value, but can easily be extended if necessary.

## Extending to nearby Maya objects

It is possible that some Maya attributes could be found on a nearby Maya object. The API allows for that by overriding the `GetMayaObjectForSchema()` callback. Let's switch to another USD API to illustrate this. The PhysicsMassAPI has values that overlap what can be found on a Bullet simulation, but we need to navigate to this simulation if we want to adapt the API.

Let's start with the adaptor itself:

```python
import mayaUsd.lib as mayaUsdLib
from pxr import Tf, Gf, Usd, UsdPhysics
from maya import cmds
import maya.api.OpenMaya as om
import maya.app.mayabullet.BulletUtils as BulletUtils
import maya.app.mayabullet.RigidBody as RigidBody

cmds.loadPlugin("mayaUsdPlugin")
cmds.loadPlugin("bullet")

class BulletMassShemaAdaptor(mayaUsdLib.SchemaApiAdaptor):
    _nameMapping = {
        "physics:mass": "mass",
        "physics:centerOfMass": "centerOfMass"
    }

    def CanAdapt(self):
        if not self.mayaObject:
            return False

        # We do not want to process the bullet node shape itself. It adds nothing of interest.
        node = om.MFnDependencyNode(self.mayaObject)
        if not node or node.typeName == "bulletRigidBodyShape":
            return False

        # Navigate to bullet shape if it exists:
        return self.GetMayaObjectForSchema() is not None

    def GetMayaObjectForSchema(self):
        path = om.MDagPath.getAPathTo(self.mayaObject)
        if not path or not path.pop():
            return None

        for i in range(path.numberOfShapesDirectlyBelow()):
            path.extendToShape(i)
            depFn = om.MFnDependencyNode(path.node())
            if depFn.typeName == "bulletRigidBodyShape":
                return path.node()
            path.pop()

        return None

    def GetAdaptedAttributeNames(self):
        return list(BulletMassShemaAdaptor._nameMapping.keys())

    def GetMayaNameForUsdAttrName(self, usdName):
        return BulletMassShemaAdaptor._nameMapping.get(usdName, "")
```

This one can be registered as follows:

```python
mayaUsdLib.SchemaApiAdaptor.Register(BulletMassShemaAdaptor, "shape", "PhysicsMassAPI")
```

So if we have a shape that is part of a bullet simulation:

```python
sphere = cmds.polySphere()[0]
rbT, rbShape = RigidBody.CreateRigidBody().command(
    autoFit=True,
    colliderShapeType= RigidBody.eShapeType.kColliderSphere,
    meshes=[sphere],
    radius=1.0,
    mass=5.0,
    centerOfMass=(0.9, 0.8, 0.7))
```

We can adapt it interactively and read write some PhysicsMassAPI attributes:

```python
sl = om.MSelectionList()
sl.add(sphere)
dagPath = sl.getDagPath(0)
dagPath.extendToShape()

adaptor = mayaUsdLib.Adaptor(dagPath.fullPathName())
print(adaptor.GetAppliedSchemas())
# Returns: ['PhysicsMassAPI']
physicsMass = adaptor.GetSchemaByName("PhysicsMassAPI")
print(physicsMass.GetAuthoredAttributeNames())
# Returns: ['physics:centerOfMass', 'physics:mass']
```

## Applying schemas

As you noticed, the schema adapts only if there is already a bullet simulation. We now want to have `ApplySchemaByName()` work as designed. This requires adding the shape to a Bullet simulation. We can automate this using these two extra callbacks:

```python
class BulletMassShemaAdaptor(mayaUsdLib.SchemaApiAdaptor):
    #
    # These two extra function are added to the
    # BulletMassShemaAdaptor previously defined:
    #
    def ApplySchema(self, dgModifier):
        """Creates a bullet simulation containing the Maya shape"""
        if self.GetMayaObjectForSchema() is not None:
            return True

        path = om.MDagPath.getAPathTo(self.mayaObject)

        if not path or not path.pop():
            return False

        BulletUtils.checkPluginLoaded()
        RigidBody.CreateRigidBody.command(transformName=path.fullPathName(), bAttachSelected=False)

        return self.GetMayaObjectForSchema() is not None

    def UnapplySchema(self, dgModifier):
        """Remove the Maya shape from a simulation"""
        if self.GetMayaObjectForSchema() is None:
            # Already unapplied?
            return False

        path = om.MDagPath.getAPathTo(self.mayaObject)
        if not path or not path.pop():
            return False

        BulletUtils.checkPluginLoaded()
        BulletUtils.removeBulletObjectsFromList([path.fullPathName()])

        return self.GetMayaObjectForSchema() is None
```

Now we can apply and unapply the PhysicsMassAPI schema interactively:

```python
sphere2 = cmds.polySphere()[0]
sl = om.MSelectionList()
sl.add(sphere2)
dagPath = sl.getDagPath(0)
dagPath.extendToShape()

adaptor = mayaUsdLib.Adaptor(dagPath.fullPathName())
print(adaptor.GetAppliedSchemas())
# Returns: []
physicsMass = adaptor.ApplySchemaByName("PhysicsMassAPI")
print(adaptor.GetAppliedSchemas())
# Returns: ['PhysicsMassAPI']
adaptor.UnapplySchemaByName("PhysicsMassAPI")
print(adaptor.GetAppliedSchemas())
# Returns: []
```

## Enabling export of adapted API

If you add `PhysicsMassAPI` to the `apiList` export option, you will notice that the values on the simulation are not exported. The `BulletMassShemaAdaptor` class must explicitly enable this capability:

```python
class BulletMassShemaAdaptor(mayaUsdLib.SchemaApiAdaptor):
    #
    # Cumulative update to the existing BulletMassShemaAdaptor
    #
    def CanAdaptForExport(self, jobArgs):
        if "PhysicsMassAPI" in jobArgs.includeAPINames:
            # Re-using previously defined code:
            return self.CanAdapt()
        return False
```

This new callback returns `True` if the schema is in the list and a bullet simulation is detected using the interactive `CanAdapt()` callback.

```python
sphere = cmds.polySphere()[0]
rbT, rbShape = RigidBody.CreateRigidBody().command(
    autoFit=True,
    colliderShapeType= RigidBody.eShapeType.kColliderSphere,
    meshes=[sphere],
    radius=1.0,
    mass=5.0)

cmds.mayaUSDExport(mergeTransformAndShape=True, file="physics_api_on_sphere.usda", apiSchema=["PhysicsMassAPI"])
```

Will result in `float physics:mass = 5` to appear in the exported USD file.

## Importing back the simulation:

This requires enabling the capability, but also requires applying the schema on-the-fly since we are rebuilding from the start:

```python
class BulletMassShemaAdaptor(mayaUsdLib.SchemaApiAdaptor):
    #
    # More cumulative updates to existing BulletMassShemaAdaptor
    #
    def CanAdaptForImport(self, jobArgs):
        return "PhysicsMassAPI" in jobArgs.includeAPINames

    def ApplySchemaForImport(self, primReaderArgs, context):
        # Check if already applied:
        if self.GetMayaObjectForSchema() is not None:
            return True

        retVal = self.ApplySchema(om.MDGModifier())

        if retVal:
            newObject = self.GetMayaObjectForSchema()
            if newObject is None:
                return False

            # Register the new node:
            context.RegisterNewMayaNode(primReaderArgs.GetUsdPrim().GetPath().pathString,
                                        newObject)

        return retVal
```

The `CanAdaptForImport()` could simply return `True`, but we still recheck the `apiSchema` list to make sure the required schema is in the list. We have a special `ApplySchemaForImport()` that receives a bit more context about the import in progress. The semantics are the same as regular `ApplySchema()`, but we also need to add the new bullet object to the list of items created at import time.

Let's reload the previously exported file:

```python
cmds.file(new=True, f=True)
cmds.mayaUSDImport(file="physics_api_on_sphere.usda", apiSchema=["PhysicsMassAPI"])

sl = om.MSelectionList()
# pSphereShape1 is the transform, since the bullet shape prevented
# merging the mesh and the transform.
# The shape will be named pSphereShape1Shape...
sl.add("pSphereShape1")
dagPath = sl.getDagPath(0)
dagPath.extendToShape()

adaptor = mayaUsdLib.Adaptor(dagPath.fullPathName())
print(adaptor.GetAppliedSchemas())
# Returns: ['PhysicsMassAPI']
```

And we see the bullet simulation is back.

## What is the minimal code necessary for exporting a schema API

This is the _TL;DR;_ of export. We will completely skip per-attribute management and provide hooks for streamlined I/O operations. The minimal API is:

```python
class BarebonesSchemaApiExporter(mayaUsdLib.SchemaApiAdaptor):
    def CanAdaptForExport(self, jobArgs):
        """Can always export if the data is there (or can be found nearby)."""
        return _ContainsExportableDataForApi(self.mayaObject)

    def CopyToPrim(self, prim, usdTime, valueWriter):
        """Barebones export callback. You have the Maya object and the USD prim,
        and all latitude to edit the USD prim at will. Please resist the
        temptation to write attributes that are not part of the schema."""

        # expect to be called at default time and once per sample time if
        # exporting animation.

        return True
```

Here is an example for another Physics API to see this in action. First we declare the exporter and register it:

```python
# We have multiple adaptors using this function, so it makes sense to extract it
def _GetBullet(mayaNode):
    """Navigate to a bullet shape if it exists"""
	path = om.MDagPath.getAPathTo(mayaNode)
	if not path or not path.pop():
		return None

	for i in range(path.numberOfShapesDirectlyBelow()):
		path.extendToShape(i)
		depFn = om.MFnDependencyNode(path.node())
		if depFn.typeName == "bulletRigidBodyShape":
			return path.node()
		path.pop()

	return None

class BulletRigidBodyShemaAdaptor(mayaUsdLib.SchemaApiAdaptor):
    def CanAdaptForExport(self, jobArgs):
        """Can always export if bullet is found."""
        return _GetBullet(self.mayaObject) is not None

    def CopyToPrim(self, prim, usdTime, valueWriter):
        """Barebones export callback."""
        rbSchema = UsdPhysics.RigidBodyAPI.Apply(prim)
        velAttr = rbSchema.CreateVelocityAttr()
        depFn = om.MFnDependencyNode(_GetBullet(self.mayaObject))
        velocityPlug = depFn.findPlug("initialVelocity", False)
        mayaUsdLib.WriteUtil.SetUsdAttr(velocityPlug, velAttr, usdTime, valueWriter)
        return True

mayaUsdLib.SchemaApiAdaptor.Register(BulletRigidBodyShemaAdaptor,
                                     "mesh", "PhysicsRigidBodyAPI")
```

We will be using the animation-aware `mayaUsdLib.WriteUtil.SetUsdAttr()` function. Now we can prepare a scene:

```python
cmds.file(new=True, f=True)
s1T = cmds.polySphere()[0]
rbT, rbShape = RigidBody.CreateRigidBody().command(
	autoFit=True,
	colliderShapeType= RigidBody.eShapeType.kColliderSphere,
	meshes=[s1T],
	radius=1.0,
	mass=5.0)
sl = om.MSelectionList()
sl.add(s1T)
bulletPath = sl.getDagPath(0)
bulletPath.extendToShape(1)
cmds.setKeyframe(bulletPath, at="initialVelocityX", t=1, v=5.0)
cmds.setKeyframe(bulletPath, at="initialVelocityX", t=10, v=50.0)
cmds.setKeyframe(bulletPath, at="initialVelocityY", t=1, v=6.0)
cmds.setKeyframe(bulletPath, at="initialVelocityY", t=10, v=60.0)
cmds.setKeyframe(bulletPath, at="initialVelocityZ", t=1, v=7.0)
cmds.setKeyframe(bulletPath, at="initialVelocityZ", t=10, v=70.0)
```

Animating the "initial" velocity does not make much sense from a simulation point of view, but let's export that anyway:

```python
cmds.mayaUSDExport(mergeTransformAndShape=True, file="RBExport.usda",
				   apiSchema=["PhysicsRigidBodyAPI"], frameRange=(1, 10))
```

If you open the resulting file, you will see that the PhysicsRigidBodyAPI was applied and that the animation on the velocity was exported:

```
        vector3f physics:velocity.timeSamples = {
            1: (5, 6, 7),
            2: (6.54321, 7.851852, 9.160494),
            3: (10.679012, 12.814815, 14.950617),
            4: (16.666666, 20, 23.333334),
            5: (23.765432, 28.518518, 33.271606),
            6: (31.234568, 37.48148, 43.728394),
            7: (38.333332, 46, 53.666668),
            8: (44.320988, 53.185184, 62.04938),
            9: (48.45679, 58.148148, 67.83951),
            10: (50, 60, 70),
        }
```

## What is the minimal code necessary for importing a schema API

This other _TL;DR;_ class is very similar to the exporter, but, unless the API is exporting something that is native in Maya, we will also need some way to create the necessary data structures Maya-side.

```python
class BarebonesSchemaApiImporter(mayaUsdLib.SchemaApiAdaptor):
    def CanAdaptForImport(self, jobArgs):
        """Can always import."""
        return True

    def ApplySchemaForImport(self, primReaderArgs, context):
        """Called to create objects/attributes/connections required on import."""
        return True

    def CopyFromPrim(self, prim, args, context):
        """Barebones import callback. You have the USD prim and the Maya object, and
        all latitude to edit the USD prim at will. Please resist the temptation to
        write attributes that are not part of the schema."""
        return True
```

Let's try re-importing the bullet velocities we previously exported:

```python
# We will be re-using the _GetBullet() function from one section above

def _ApplyBulletSchema(mayaShape):
    """Creates a bullet simulation containing the "mayaShape"."""
    if _GetBulletShape(mayaShape) is not None:
        return True

    path = om.MDagPath.getAPathTo(mayaShape)
    if not path or not path.pop():
        return False

    BulletUtils.checkPluginLoaded()
    RigidBody.CreateRigidBody.command(transformName=path.fullPathName(),
									  bAttachSelected=False)

    return True

class BulletRigidBodyShemaImporter(mayaUsdLib.SchemaApiAdaptor):
    #
    # Not an incremental class this time. Import and export can be split in two
    # separate classes as needed.
    #
    def CanAdaptForImport(self, jobArgs):
        """Can always import."""
        return True

    def ApplySchemaForImport(self, primReaderArgs, context):
        """Called to create objects/attributes/connections required on import."""
        # Check if already applied:
        if _GetBullet(self.mayaObject) is not None:
            return True

        retVal = _ApplyBulletSchema(self.mayaObject)
        newObject = _GetBullet(self.mayaObject)
        if newObject is None:
            return False

        # Register the new node so we can know which ones were created on import:
        context.RegisterNewMayaNode(
                primReaderArgs.GetUsdPrim().GetPath().pathString,
                newObject)
        return True

    def CopyFromPrim(self, prim, args, context):
        """Barebones import callback."""
        rbSchema = UsdPhysics.RigidBodyAPI(prim)
        if not rbSchema:
            return False

        velAttr = rbSchema.GetVelocityAttr()
        if velAttr:
            mayaUsdLib.ReadUtil.ReadUsdAttribute(
                velAttr, _GetBullet(self.mayaObject),
                "initialVelocity", args, context)

        return True
```

Using the animation aware `mayaUsdLib.ReadUtil.ReadUsdAttribute()` function, so we can now try importing the scene we exported in the previous section:

```python
mayaUsdLib.SchemaApiAdaptor.Register(BulletRigidBodyShemaImporter,
                                     "mesh", "PhysicsRigidBodyAPI")
cmds.file(new=True, f=True)
cmds.mayaUSDImport(file="RBExport.usda", apiSchema=["PhysicsRigidBodyAPI"],
                   readAnimData=True)
```

If you expand under pSphere1|pSphereShape1 you will see the imported bullet simulation node bulletRigidBodyShape1 with all the previously exported animation.

## How to handle relationships

The schema API adaptors are called as each object in the source datamaodel is traversed, so it is quite possible that objects that are referenced in relationships do not yet appear in the target datamodel that is being created. It is recommended to process relationships post-translation using chasers.

## How about C++?

All the API discussed here are available in C++ as well (except class unregistration). All the information presented here for Python was extracted directly from the [testSchemaApiAdaptor](https://github.com/Autodesk/maya-usd/blob/dev/test/lib/mayaUsd/fileio/testSchemaApiAdaptor.py) unit test. There is an equivalent unit test on the C++ side where you can see a [Mass API adaptor](https://github.com/Autodesk/maya-usd/blob/dev/test/lib/usd/plugin/bulletAdaptor.cpp#L147) implementation being registered [here](https://github.com/Autodesk/maya-usd/blob/dev/test/lib/usd/plugin/bulletAdaptor.cpp#L274). This requires [registering the plugin](https://github.com/Autodesk/maya-usd/blob/dev/test/lib/usd/plugin/nullApiExporter_plugInfo.json#L7) to be loaded when scanning for adaptors and is exercised by another [testUsdExportSchemaApi](https://github.com/Autodesk/maya-usd/blob/dev/test/lib/usd/translators/testUsdExportSchemaApi.py) that follows closely the Python version.