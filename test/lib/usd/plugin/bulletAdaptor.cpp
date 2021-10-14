//
// Copyright 2021 Autodesk
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
#include <mayaUsd/fileio/exportContextRegistry.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/schemaApiAdaptor.h>
#include <mayaUsd/fileio/schemaApiAdaptorRegistry.h>
#include <mayaUsd/fileio/utils/writeUtil.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/usdPhysics/rigidBodyAPI.h>

#include <maya/MDGModifier.h>
#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MObjectHandle.h>
#include <maya/MString.h>

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // USD
    (PhysicsMassAPI)
    ((Mass, "physics:mass"))
    ((CenterOfMass, "physics:centerOfMass"))
    (PhysicsRigidBodyAPI)

    // Maya
    (mass)
    (centerOfMass)
);
// clang-format on

REGISTER_EXPORT_CONTEXT_FCT(
    Bullet,
    "Bullet Physics API Export",
    "Test export of USD Physics APIs on a Bullet simulation")
{
    VtDictionary extraArgs;
    extraArgs[UsdMayaJobExportArgsTokens->apiSchema]
        = VtValue(std::vector<VtValue> { VtValue(_tokens->PhysicsRigidBodyAPI.GetString()),
                                         VtValue(_tokens->PhysicsMassAPI.GetString()) });
    return extraArgs;
}

class UsdPrimDefinition;

class TestBulletMassShemaAdaptor : public UsdMayaSchemaApiAdaptor
{
    using _baseClass = UsdMayaSchemaApiAdaptor;

public:
    TestBulletMassShemaAdaptor(
        const MObjectHandle&     object,
        const TfToken&           schemaName,
        const UsdPrimDefinition* schemaPrimDef)
        : UsdMayaSchemaApiAdaptor(object, schemaName, schemaPrimDef)
    {
    }

    ~TestBulletMassShemaAdaptor() override { }

    bool CanAdapt() const override
    {
        // We do not want to process the bullet node shape itself. It adds nothing of interest.
        if (!_handle.isValid()) {
            return false;
        }

        MFnDependencyNode depFn;
        if (depFn.setObject(_handle.object()) != MS::kSuccess
            || depFn.typeName() == "bulletRigidBodyShape") {
            return false;
        }

        return !GetMayaObjectForSchema().isNull();
    }

    bool CanAdaptForExport(const UsdMayaJobExportArgs& jobArgs) const override
    {
        if (jobArgs.includeAPINames.find(_tokens->PhysicsMassAPI)
            != jobArgs.includeAPINames.end()) {
            return CanAdapt();
        }
        return false;
    }

    bool ApplySchema(MDGModifier&)
    {
        // Check if already applied:
        if (!GetMayaObjectForSchema().isNull()) {
            return true;
        }

        // Make this object a rigid body:
        // Need to call some Python as this is the Bullet way...
        // Which makes the MDGModifier kinda moot...
        MDagPath path;
        if (!MDagPath::getAPathTo(_handle.object(), path)) {
            return false;
        }
        if (!path.pop()) {
            return false;
        }
        const char* bulletCmd = "import maya.app.mayabullet.BulletUtils as BulletUtils; "
                                "BulletUtils.checkPluginLoaded(); "
                                "import maya.app.mayabullet.RigidBody as RigidBody; "
                                "RigidBody.CreateRigidBody.command(meshes=['^1s'])";

        MString bulletRigidBody;
        bulletRigidBody.format(bulletCmd, path.fullPathName());
        MGlobal::executePythonCommand(bulletRigidBody);

        return !GetMayaObjectForSchema().isNull();
    }

    bool UnapplySchema(MDGModifier&) override
    {
        if (GetMayaObjectForSchema().isNull()) {
            // Already unapplied?
            return false;
        }

        MDagPath path;
        if (!MDagPath::getAPathTo(_handle.object(), path)) {
            return false;
        }
        if (!path.pop()) {
            return false;
        }
        const char* bulletCmd = "import maya.app.mayabullet.BulletUtils as BulletUtils; "
                                "BulletUtils.checkPluginLoaded(); "
                                "BulletUtils.removeBulletObjectsFromList(['^1s']) ";

        MString bulletRigidBody;
        bulletRigidBody.format(bulletCmd, path.fullPathName());
        MGlobal::executePythonCommand(bulletRigidBody);

        return GetMayaObjectForSchema().isNull();
    }

    MObject GetMayaObjectForSchema() const override
    {
        // The bullet shape can be found as another prim colocated with the geometry shape
        MDagPath path;
        if (!MDagPath::getAPathTo(_handle.object(), path)) {
            return {};
        }

        if (!path.pop()) {
            return {};
        }

        unsigned int numShapes = 0;
        if (!path.numberOfShapesDirectlyBelow(numShapes)) {
            return {};
        }

        for (unsigned int i = 0; i < numShapes; ++i) {
            path.extendToShapeDirectlyBelow(i);

            MFnDependencyNode depFn(path.node());
            if (depFn.typeName() == "bulletRigidBodyShape") {
                return path.node();
            }

            path.pop();
        }

        return {};
    }

    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override
    {
        if (usdAttrName == _tokens->Mass) {
            return _tokens->mass;
        } else if (usdAttrName == _tokens->CenterOfMass) {
            return _tokens->centerOfMass;
        }
        return {};
    }

    TfTokenVector GetAdaptedAttributeNames() const override
    {
        return { _tokens->Mass, _tokens->CenterOfMass };
    }
};

PXRUSDMAYA_REGISTER_SCHEMA_API_ADAPTOR(shape, PhysicsMassAPI, TestBulletMassShemaAdaptor);

class TestBulletRigidBodyShemaAdaptor : public UsdMayaSchemaApiAdaptor
{
    using _baseClass = UsdMayaSchemaApiAdaptor;

public:
    TestBulletRigidBodyShemaAdaptor(
        const MObjectHandle&     object,
        const TfToken&           schemaName,
        const UsdPrimDefinition* schemaPrimDef)
        : UsdMayaSchemaApiAdaptor(object, schemaName, schemaPrimDef)
    {
    }

    ~TestBulletRigidBodyShemaAdaptor() override { }

    bool CanAdapt() const override
    {
        // This class does not adapt in a freeform context.
        return false;
    }

    bool CanAdaptForExport(const UsdMayaJobExportArgs& jobArgs) const override
    {
        // This class is specialized for export:
        if (!_handle.isValid()) {
            return false;
        }

        MFnDependencyNode depFn;
        if (depFn.setObject(_handle.object()) != MS::kSuccess
            || depFn.typeName() == "bulletRigidBodyShape") {
            return false;
        }

        if (jobArgs.includeAPINames.find(_tokens->PhysicsMassAPI)
            != jobArgs.includeAPINames.end()) {
            return !GetMayaObjectForSchema().isNull();
        }
        return false;
    }

    bool ApplySchema(MDGModifier&)
    {
        // Check if already applied:
        if (!GetMayaObjectForSchema().isNull()) {
            return true;
        }

        // Make this object a rigid body:
        // Need to call some Python as this is the Bullet way...
        // Which makes the MDGModifier kinda moot...
        MDagPath path;
        if (!MDagPath::getAPathTo(_handle.object(), path)) {
            return false;
        }
        if (!path.pop()) {
            return false;
        }
        const char* bulletCmd = "import maya.app.mayabullet.BulletUtils as BulletUtils; "
                                "BulletUtils.checkPluginLoaded(); "
                                "import maya.app.mayabullet.RigidBody as RigidBody; "
                                "RigidBody.CreateRigidBody.command(meshes=['^1s'])";

        MString bulletRigidBody;
        bulletRigidBody.format(bulletCmd, path.fullPathName());
        MGlobal::executePythonCommand(bulletRigidBody);

        return !GetMayaObjectForSchema().isNull();
    }

    MObject GetMayaObjectForSchema() const override
    {
        // The bullet shape can be found as another prim colocated with the geometry shape
        MDagPath path;
        if (!MDagPath::getAPathTo(_handle.object(), path)) {
            return {};
        }

        if (!path.pop()) {
            return {};
        }

        unsigned int numShapes = 0;
        if (!path.numberOfShapesDirectlyBelow(numShapes)) {
            return {};
        }

        for (unsigned int i = 0; i < numShapes; ++i) {
            path.extendToShapeDirectlyBelow(i);

            MFnDependencyNode depFn(path.node());
            if (depFn.typeName() == "bulletRigidBodyShape") {
                return path.node();
            }

            path.pop();
        }

        return {};
    }

    bool CopyToPrim(
        const UsdPrim&             prim,
        const UsdTimeCode&         usdTime,
        UsdUtilsSparseValueWriter* valueWriter) const override
    {
        std::string whyNot;
        if (!UsdPhysicsRigidBodyAPI::CanApply(prim, &whyNot)) {
            TF_CODING_ERROR("Invalid prim: %s", whyNot.c_str());
            return true;
        }

        // Export one attribute
        auto rbSchema = UsdPhysicsRigidBodyAPI::Apply(prim);
        auto velAttr = rbSchema.CreateVelocityAttr();

        MFnDependencyNode depFn(GetMayaObjectForSchema());
        MPlug             velocityPlug = depFn.findPlug("initialVelocity", false);

        float          x, y, z;
        MFnNumericData numericDataFn(velocityPlug.asMObject());
        numericDataFn.getData(x, y, z);

        GfVec3f velValue(x, y, z);

        UsdMayaWriteUtil::SetAttribute(velAttr, velValue, usdTime, valueWriter);

        return true;
    }
};

PXRUSDMAYA_REGISTER_SCHEMA_API_ADAPTOR(shape, PhysicsRigidBodyAPI, TestBulletRigidBodyShemaAdaptor);

PXR_NAMESPACE_CLOSE_SCOPE
