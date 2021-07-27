//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef AL_USDMAYASCHEMASTEST_GENERATED_EXAMPLEPOLYCUBENODE_H
#define AL_USDMAYASCHEMASTEST_GENERATED_EXAMPLEPOLYCUBENODE_H

/// \file AL_USDMayaSchemasTest/ExamplePolyCubeNode.h

#include "api.h"
#include "tokens.h"

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/type.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/typed.h>

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// ALEXAMPLEPOLYCUBENODE                                                      //
// -------------------------------------------------------------------------- //

/// \class AL_usd_ExamplePolyCubeNode
///
/// Poly Cube Creator Example Node
///
class AL_usd_ExamplePolyCubeNode : public UsdTyped
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

    /// Construct a AL_usd_ExamplePolyCubeNode on UsdPrim \p prim .
    /// Equivalent to AL_usd_ExamplePolyCubeNode::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit AL_usd_ExamplePolyCubeNode(const UsdPrim& prim = UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a AL_usd_ExamplePolyCubeNode on the prim held by \p schemaObj .
    /// Should be preferred over AL_usd_ExamplePolyCubeNode(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit AL_usd_ExamplePolyCubeNode(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    AL_USDMAYASCHEMASTEST_API
    virtual ~AL_usd_ExamplePolyCubeNode();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    AL_USDMAYASCHEMASTEST_API
    static const TfTokenVector& GetSchemaAttributeNames(bool includeInherited = true);

    /// Return a AL_usd_ExamplePolyCubeNode holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// AL_usd_ExamplePolyCubeNode(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    AL_USDMAYASCHEMASTEST_API
    static AL_usd_ExamplePolyCubeNode Get(const UsdStagePtr& stage, const SdfPath& path);

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
    AL_USDMAYASCHEMASTEST_API
    static AL_usd_ExamplePolyCubeNode Define(const UsdStagePtr& stage, const SdfPath& path);

protected:
#if PXR_VERSION >= 2108
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    AL_USDMAYASCHEMASTEST_API
    virtual UsdSchemaKind _GetSchemaKind() const;
#else
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    AL_USDMAYASCHEMASTEST_API
    virtual UsdSchemaType _GetSchemaType() const;
#endif

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    AL_USDMAYASCHEMASTEST_API
    static const TfType& _GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    AL_USDMAYASCHEMASTEST_API
    virtual const TfType& _GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // WIDTH
    // --------------------------------------------------------------------- //
    /// the width of the cube
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMASTEST_API
    UsdAttribute GetWidthAttr() const;

    /// See GetWidthAttr(), and also
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMASTEST_API
    UsdAttribute
    CreateWidthAttr(VtValue const& defaultValue = VtValue(), bool writeSparsely = false) const;

public:
    // --------------------------------------------------------------------- //
    // HEIGHT
    // --------------------------------------------------------------------- //
    /// the height of the cube
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMASTEST_API
    UsdAttribute GetHeightAttr() const;

    /// See GetHeightAttr(), and also
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMASTEST_API
    UsdAttribute
    CreateHeightAttr(VtValue const& defaultValue = VtValue(), bool writeSparsely = false) const;

public:
    // --------------------------------------------------------------------- //
    // DEPTH
    // --------------------------------------------------------------------- //
    /// the depth of the cube
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMASTEST_API
    UsdAttribute GetDepthAttr() const;

    /// See GetDepthAttr(), and also
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMASTEST_API
    UsdAttribute
    CreateDepthAttr(VtValue const& defaultValue = VtValue(), bool writeSparsely = false) const;

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
