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
#ifndef MAYAUSD_SCHEMAS_GENERATED_ALMAYAREFERENCE_H
#define MAYAUSD_SCHEMAS_GENERATED_ALMAYAREFERENCE_H

/// \file mayaUsd_Schemas/ALMayaReference.h

#include <mayaUsd_Schemas/MayaReference.h>
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

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// ALMAYAREFERENCE                                                            //
// -------------------------------------------------------------------------- //

/// \class MayaUsd_SchemasALMayaReference
///
/// Data used to import a maya reference.
///
class MayaUsd_SchemasALMayaReference : public MayaUsd_SchemasMayaReference
{
public:
#if PXR_VERSION >= 2108
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::ConcreteTyped;
#else
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::ConcreteTyped;
#endif

    /// Construct a MayaUsd_SchemasALMayaReference on UsdPrim \p prim .
    /// Equivalent to MayaUsd_SchemasALMayaReference::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit MayaUsd_SchemasALMayaReference(const UsdPrim& prim = UsdPrim())
        : MayaUsd_SchemasMayaReference(prim)
    {
    }

    /// Construct a MayaUsd_SchemasALMayaReference on the prim held by \p schemaObj .
    /// Should be preferred over MayaUsd_SchemasALMayaReference(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit MayaUsd_SchemasALMayaReference(const UsdSchemaBase& schemaObj)
        : MayaUsd_SchemasMayaReference(schemaObj)
    {
    }

    /// Destructor.
    MAYAUSD_SCHEMAS_API
    virtual ~MayaUsd_SchemasALMayaReference();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    MAYAUSD_SCHEMAS_API
    static const TfTokenVector& GetSchemaAttributeNames(bool includeInherited = true);

    /// Return a MayaUsd_SchemasALMayaReference holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// MayaUsd_SchemasALMayaReference(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    MAYAUSD_SCHEMAS_API
    static MayaUsd_SchemasALMayaReference Get(const UsdStagePtr& stage, const SdfPath& path);

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
    static MayaUsd_SchemasALMayaReference Define(const UsdStagePtr& stage, const SdfPath& path);

protected:
#if PXR_VERSION >= 2108
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    MAYAUSD_SCHEMAS_API
    UsdSchemaKind _GetSchemaKind() const override;
#else
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    MAYAUSD_SCHEMAS_API
    UsdSchemaType _GetSchemaType() const override;
#endif

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
