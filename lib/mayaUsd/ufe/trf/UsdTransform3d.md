# Editing USD Transformable Objects with maya-usd

Draft version 0.3, 19-Aug-2020, 4:52 P.M.

## Background

Transformable USD prims are derived fom [UsdGeomXformable](https://graphics.pixar.com/usd/docs/api/class_usd_geom_xformable.html#details), which supports
arbitrary sequences of component affine transformations.  These transform
operations, or transform ops, are implemented in 
[UsdGeomXformOp](https://graphics.pixar.com/usd/docs/api/class_usd_geom_xform_op.html#details),
a schema wrapper on a UsdAttribute to read and write transform operations.

Because the list of transform operations is arbitrary, it can be authored in
different ways by different producers of USD data.  The existing maya-usd
implementation for the UFE Transform3d API, which requires conversion to the
very restrictive USD common transform API
[UsdGeomXformCommonAPI](https://graphics.pixar.com/usd/docs/api/class_usd_geom_xform_common_a_p_i.html#details)
is inadequate and destructive.

## Requirements

In discussions with partners, Autodesk has identified a set of requirements for
editing USD transformable prim transform stacks.  We focus on two requirements
here, identified as per the discussion documents:
- 2 Manipulation code will use an interface that takes care of transformation
operations management. Pipelines may choose to change this interface at runtime
to improve interop between DCCs.

- 6 UFE API should provide UFE-USD plugin with an interface to handle the
in-place editing of any arbitrary transformation stacks, i.e. target any
transformation op.

## Assumptions

Autodesk is making a proposal to meet these requirements under the following
assumptions.
- Users of maya-usd want to direct editing of transformable prims with an
arbitrary stack of transform ops to one or more of these transform ops which
are relevant for their workflow.
- It must be possible for a plugin to completely override what Autodesk
proposes as the default maya-usd transform editing capability.
- Transform ops can be conceptually seen as transformable objects in their own
right, and therefore can be targeted as the destination of 3D transform edits
through viewport manipulation.

## Existing Interfaces

Three UFE interfaces are involved when dealing with reading or writing to
transformable USD objects, which must all be implemented by the UFE run-time.
- The Ufe::Transform3dHandler: this is the factory object that creates
Ufe::Transform3d interfaces.  This factory object can be replaced at run-time.
- The Ufe::Transform3d: this is the interface object that is used to read the
transform data.  When writing, either the write is done without undo capability
directly through the Transform3d interface, or the Transform3d interface is
asked to create an undoable command (see below)
- The Ufe::TranslateUndoableCommand, Ufe::RotateUndoableCommand, and
UfeScaleUndoableCommand: these deal with the complete interactive manipulation,
and obviously undo / redo.  During manipulation, they are repeatedly updated
with the latest value of the attribute being changed.

## Proposal

### Targeting TRS Edits Inside the Transform Stack

The `Transform3dHandler::transform3d()` call must be used to create an
interface object to read or write the complete object's local transform.
Reading is done for example when transforming the UFE Object3d interface object
space bounding box.  Writing is done for example when performing the Maya
`parent -absolute` command, where the object's world space transformation must
not change, and therefore the object's complete local transform must be
changed.  This conflicts with the requirement to edit an individual transform
op.

To solve this, a key is the realization that the Transform3d interface is
appropriate for editing arbitrary transform ops inside a USD transformable
prim.  The Transform3d interface applies to an object, whatever the implementer
of the interface decides that object to be.  Therefore, we can apply the
Transform3d interface **to USD transform ops**.  In that case, we can implement
the `Transform3d::segmentExclusiveMatrix()` and 
`Transform3d::segmentInclusiveMatrix()` to return values appropriate to a USD
transform op.  The stack of transform ops is then treated as representing
objects to be manipulated using a Transform3d interface.

What is needed is a way for the implementer of the UFE run-time to
return an interface object that appropriately targets a single transform op, or
a logical group of transform ops (e.g. the ops in the USD common transform API,
or the ops in the Maya transform API).  This **replaces the need for a
dedicated coordinate frame interface**: interpreting one or more transform ops
as a logical "local transform", the coordinate frame is that of the transform
op (or group of transform ops), and we can re-use the existing Transform3d
interface.

We implement this by adding a new virtual to the Ufe::Transform3dHandler
factory interface.  Its signature and documentation are the following:
```C++
    /*! Return an interface to be used to edit the 3D transformation of the
        object.  By default, returns the normal Transform3d interface.  The
        edit transform object may have a different local transformation and a
        different \ref Ufe::Transform3d::segmentInclusiveMatrix() and
        \ref Ufe::Transform3d::segmentExclusiveMatrix() than the normal
        Transform3d interface associated with a scene item.  All changes made
        through the edit transform 3D object will be visible through the normal
        \ref Ufe::Transform3d::transform3d() interface.
        \param item SceneItem to use to retrieve its Transform3d interface.
        \param hint Contextual information for Transform3d interface creation.
        \return Transform3d interface of given SceneItem. Returns a null
        pointer if no Transform3d interface can be created for the item.
    */
    virtual Transform3d::Ptr editTransform3d(
        const SceneItem::Ptr&      item,
        const EditTransform3dHint& hint = EditTransform3dHint()
    ) const {
        return transform3d(item);
    }
```
The rest of the Transform3dHandler, and the Transform3d interface, are
unchanged.  The Maya move, rotate and scale commands call `editTransform3d`,
not `transform3d`, when performing move, rotate, or scale manipulation.  The
implementer can then return an interface object that is appropriate for editing
a transform op, or group of transform ops.

### Using Transform3dHandler Customization Capability

Plugins can replace the USD run-time
Transform3dHandler (the factory object) with a custom factory.  A plugin is
therefore free to completely replace the Autodesk Transform3dHandler at
run-time with its own Transform3dHandler.  This allows the creation of
Transform3d interface objects created by the plugin.  This does not require any
change to maya-usd core code, or any derivation from core code, only derivation
from the UFE base classes.  Plugin-created Transform3dHandler classes can
therefore be created and live side by side with Autodesk core
Transform3dHandler classes, avoiding conflicting updates.

Autodesk uses to capability in the prototype
transform edit code to set up a [chain of
responsibility](https://en.wikipedia.org/wiki/Chain-of-responsibility_pattern)
with multiple Transform3dHandler objects, each capable of recognizing whether
the transformable prim can be edited by its associated Transform3d interface.
If it can handle the transformable prim, it creates the Transform3d interface
object, and if cannot, it delegates to the next Transform3dHandler in the chain.

In the prototype code for example, there is a matrix op handler that inspects
whether the transformable prim has a single matrix op, and the fallback
Transform3Handler, which is the existing forced conversion to the USD common
transform API.  More Transform3dHandler's can be devised, for example to
support the Autodesk Maya Transform API, or by plugin writers to support their
own transform API.  The transform handlers in the prototype code may or may not
survive the transition to production code, but the fundamental idea is that
these handlers can support focused code that is easier to mix and match and
maintain, through clear separation of concerns.

### Using Transform Command Customization Capability

By implementing a Transform3d interface class, the USD run-time undoable
commands can be customized.  The Transform3d interface is accessed by the Maya
move, rotate and scale commands by being asked to create commands for the
translation, rotation, and scale.  From that point on the Transform3d interface
plays no part in setting the translation, rotation, and scale: the commands
perform all the work.
```C++
    //! Create an undoable translate command.  The command is not executed.
    //! \param x X-axis translation.
    //! \param y Y-axis translation.
    //! \param z Z-axis translation.
    //! \return Undoable command whose undo restores the original translation.
    virtual TranslateUndoableCommand::Ptr translateCmd(
        double x, double y, double z) = 0;

    //! Create an undoable rotate command.  The command is not executed.
    //! \param x value to rotate on the X-axis, in degrees.
    //! \param y value to rotate on the Y-axis, in degrees.
    //! \param z value to rotate on the Z-axis, in degrees.
    //! \return Undoable command whose undo restores the original rotation.
    virtual RotateUndoableCommand::Ptr rotateCmd(
        double x, double y, double z) = 0;

    //! Create an undoable scale command.  The command is not executed.
    //! \param x value to scale on the X-axis.
    //! \param y value to scale on the Y-axis.
    //! \param z value to scale on the Z-axis.
    //! \return Undoable command whose undo restores the scale's value
    virtual ScaleUndoableCommand::Ptr scaleCmd(
        double x, double y, double z) = 0;
```
This is a clear way by which Maya indicates what is under manipulation: when
the Transform3d interface object is asked to create a translate command, the
plugin knows that the object's translation will be edited, and so on.

## Analysis

Autodesk believes that the proposed Transform3dHandler addition above, and the
use of existing facilities, allows moving forward in meeting the requirements:
- 2 *Manipulation code will use an interface that takes care of transformation
operations management. Pipelines may choose to change this interface at runtime
to improve interop between DCCs.*  This is met by the existing customizable
Transform3dHandler system, which can create custom Transform3d interfaces.
Pipelines can leverage this mechanism to introduce studio-specific overrides
without modifying the default implementation or having to maintain and resolve
conflicts when the default implementation changes. In other words, the default
implementation and studio overrides will be leveraging a stable common
interface.
- 6 *UFE API should provide UFE-USD plugin with an interface to handle the
in-place editing of any arbitrary transformation stacks, i.e. target any
transformation op.*  The foundations for meeting this requirement is the
addition of the `Transform3dHandler::editTransform3d()` method.  As presented
in the proof of concept MatrixOp handler implementation, this allows writing
custom Transform3d interfaces targeting specific transform ops.  We are open to
feedback to identify potential other UFE API gaps when it comes to
requirement 6.
