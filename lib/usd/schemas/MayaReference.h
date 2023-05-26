//
// Copyright 2016 Pixar
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
#ifndef MAYAUSD_SCHEMAS_GENERATED_MAYAREFERENCE_H
#define MAYAUSD_SCHEMAS_GENERATED_MAYAREFERENCE_H

/// \file mayaUsd_Schemas/MayaReference.h

#include <mayaUsd_Schemas/api.h>
#include <mayaUsd_Schemas/tokens.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/type.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/xformable.h>

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// MAYAREFERENCE                                                              //
// -------------------------------------------------------------------------- //

/// \class MayaUsd_SchemasMayaReference
///
/// Data used to import a maya reference.
///
class MayaUsd_SchemasMayaReference : public UsdGeomXformable
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;

    /// Construct a MayaUsd_SchemasMayaReference on UsdPrim \p prim .
    /// Equivalent to MayaUsd_SchemasMayaReference::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit MayaUsd_SchemasMayaReference(const UsdPrim& prim = UsdPrim())
        : UsdGeomXformable(prim)
    {
    }

    /// Construct a MayaUsd_SchemasMayaReference on the prim held by \p schemaObj .
    /// Should be preferred over MayaUsd_SchemasMayaReference(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit MayaUsd_SchemasMayaReference(const UsdSchemaBase& schemaObj)
        : UsdGeomXformable(schemaObj)
    {
    }

    /// Destructor.
    MAYAUSD_SCHEMAS_API
    virtual ~MayaUsd_SchemasMayaReference();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    MAYAUSD_SCHEMAS_API
    static const TfTokenVector& GetSchemaAttributeNames(bool includeInherited = true);

    /// Return a MayaUsd_SchemasMayaReference holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// MayaUsd_SchemasMayaReference(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    MAYAUSD_SCHEMAS_API
    static MayaUsd_SchemasMayaReference Get(const UsdStagePtr& stage, const SdfPath& path);

    /// Attempt to ensure a \a UsdPrim adhering to this schema at \p path
    /// is defined (according to UsdPrim::IsDefined()) on this stage.
    ///
    /// If a prim adhering to this schema at \p path is already defined on this
    /// stage, return that prim.  Otherwise author an \a SdfPrimSpec with
    /// \a specifier == \a SdfSpecifierDef and this schema's prim type name for
    /// the prim at \p path at the current EditTarget.  Author \a SdfPrimSpec s
    /// with \p specifier == \a SdfSpecifierDef and empty typeName at the
    /// current EditTarget for any nonexistent, or existing but not \a Defined
    /// ancestors.
    ///
    /// The given \a path must be an absolute prim path that does not contain
    /// any variant selections.
    ///
    /// If it is impossible to author any of the necessary PrimSpecs, (for
    /// example, in case \a path cannot map to the current UsdEditTarget's
    /// namespace) issue an error and return an invalid \a UsdPrim.
    ///
    /// Note that this method may return a defined prim whose typeName does not
    /// specify this schema class, in case a stronger typeName opinion overrides
    /// the opinion at the current EditTarget.
    ///
    MAYAUSD_SCHEMAS_API
    static MayaUsd_SchemasMayaReference Define(const UsdStagePtr& stage, const SdfPath& path);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    MAYAUSD_SCHEMAS_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    MAYAUSD_SCHEMAS_API
    static const TfType& _GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    MAYAUSD_SCHEMAS_API
    const TfType& _GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // MAYAREFERENCE
    // --------------------------------------------------------------------- //
    /// Path to the maya reference.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `asset mayaReference` |
    /// | C++ Type | SdfAssetPath |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Asset |
    MAYAUSD_SCHEMAS_API
    UsdAttribute GetMayaReferenceAttr() const;

    /// See GetMayaReferenceAttr(), and also
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    MAYAUSD_SCHEMAS_API
    UsdAttribute CreateMayaReferenceAttr(
        VtValue const& defaultValue = VtValue(),
        bool           writeSparsely = false) const;

public:
    // --------------------------------------------------------------------- //
    // MAYANAMESPACE
    // --------------------------------------------------------------------- //
    /// Namespace which the maya reference will be imported under.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `string mayaNamespace` |
    /// | C++ Type | std::string |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->String |
    MAYAUSD_SCHEMAS_API
    UsdAttribute GetMayaNamespaceAttr() const;

    /// See GetMayaNamespaceAttr(), and also
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    MAYAUSD_SCHEMAS_API
    UsdAttribute CreateMayaNamespaceAttr(
        VtValue const& defaultValue = VtValue(),
        bool           writeSparsely = false) const;

public:
    // --------------------------------------------------------------------- //
    // MAYAAUTOEDIT
    // --------------------------------------------------------------------- //
    /// When an instance of this schema will be discovered in Maya, should it be auto-pulled.
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `bool mayaAutoEdit = 0` |
    /// | C++ Type | bool |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Bool |
    MAYAUSD_SCHEMAS_API
    UsdAttribute GetMayaAutoEditAttr() const;

    /// See GetMayaAutoEditAttr(), and also
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    MAYAUSD_SCHEMAS_API
    UsdAttribute CreateMayaAutoEditAttr(
        VtValue const& defaultValue = VtValue(),
        bool           writeSparsely = false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by
    // the code generator.
    //
    // Just remember to:
    //  - Close the class declaration with };
    //  - Close the namespace with PXR_NAMESPACE_CLOSE_SCOPE
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
