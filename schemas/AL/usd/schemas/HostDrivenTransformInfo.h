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
#ifndef AL_USDMAYASCHEMAS_GENERATED_HOSTDRIVENTRANSFORMINFO_H
#define AL_USDMAYASCHEMAS_GENERATED_HOSTDRIVENTRANSFORMINFO_H

/// \file AL_USDMayaSchemas/HostDrivenTransformInfo.h

#include "pxr/pxr.h"
#include "./api.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "./tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// ALHOSTDRIVENTRANSFORMINFO                                                  //
// -------------------------------------------------------------------------- //

/// \class AL_usd_HostDrivenTransformInfo
///
/// Connection info for transforms
///
class AL_usd_HostDrivenTransformInfo : public UsdTyped
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = true;

    /// Construct a AL_usd_HostDrivenTransformInfo on UsdPrim \p prim .
    /// Equivalent to AL_usd_HostDrivenTransformInfo::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit AL_usd_HostDrivenTransformInfo(const UsdPrim& prim=UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a AL_usd_HostDrivenTransformInfo on the prim held by \p schemaObj .
    /// Should be preferred over AL_usd_HostDrivenTransformInfo(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit AL_usd_HostDrivenTransformInfo(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    AL_USDMAYASCHEMAS_API
    virtual ~AL_usd_HostDrivenTransformInfo();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    AL_USDMAYASCHEMAS_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a AL_usd_HostDrivenTransformInfo holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// AL_usd_HostDrivenTransformInfo(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    AL_USDMAYASCHEMAS_API
    static AL_usd_HostDrivenTransformInfo
    Get(const UsdStagePtr &stage, const SdfPath &path);

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
    AL_USDMAYASCHEMAS_API
    static AL_usd_HostDrivenTransformInfo
    Define(const UsdStagePtr &stage, const SdfPath &path);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    AL_USDMAYASCHEMAS_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    AL_USDMAYASCHEMAS_API
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // TRANSFORMSOURCENODES 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<string>
    /// \n  Usd Type: SdfValueTypeNames->StringArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetTransformSourceNodesAttr() const;

    /// See GetTransformSourceNodesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateTransformSourceNodesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SCALEATTRIBUTES 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<string>
    /// \n  Usd Type: SdfValueTypeNames->StringArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetScaleAttributesAttr() const;

    /// See GetScaleAttributesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateScaleAttributesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SCALEATTRIBUTEINDICES 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetScaleAttributeIndicesAttr() const;

    /// See GetScaleAttributeIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateScaleAttributeIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ROTATEATTRIBUTES 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<string>
    /// \n  Usd Type: SdfValueTypeNames->StringArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetRotateAttributesAttr() const;

    /// See GetRotateAttributesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateRotateAttributesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ROTATEATTRIBUTEINDICES 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetRotateAttributeIndicesAttr() const;

    /// See GetRotateAttributeIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateRotateAttributeIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TRANSLATEATTRIBUTES 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<string>
    /// \n  Usd Type: SdfValueTypeNames->StringArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetTranslateAttributesAttr() const;

    /// See GetTranslateAttributesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateTranslateAttributesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TRANSLATEATTRIBUTEINDICES 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetTranslateAttributeIndicesAttr() const;

    /// See GetTranslateAttributeIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateTranslateAttributeIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ROTATEORDERATTRIBUTES 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<string>
    /// \n  Usd Type: SdfValueTypeNames->StringArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetRotateOrderAttributesAttr() const;

    /// See GetRotateOrderAttributesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateRotateOrderAttributesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ROTATEORDERATTRIBUTEINDICES 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetRotateOrderAttributeIndicesAttr() const;

    /// See GetRotateOrderAttributeIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateRotateOrderAttributeIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VISIBILITYSOURCENODES 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<string>
    /// \n  Usd Type: SdfValueTypeNames->StringArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetVisibilitySourceNodesAttr() const;

    /// See GetVisibilitySourceNodesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateVisibilitySourceNodesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VISIBILITYATTRIBUTES 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<string>
    /// \n  Usd Type: SdfValueTypeNames->StringArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetVisibilityAttributesAttr() const;

    /// See GetVisibilityAttributesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateVisibilityAttributesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // VISIBILITYATTRIBUTEINDICES 
    // --------------------------------------------------------------------- //
    /// 
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetVisibilityAttributeIndicesAttr() const;

    /// See GetVisibilityAttributeIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateVisibilityAttributeIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // TARGETTRANSFORMS 
    // --------------------------------------------------------------------- //
    /// 
    ///
    AL_USDMAYASCHEMAS_API
    UsdRelationship GetTargetTransformsRel() const;

    /// See GetTargetTransformsRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    AL_USDMAYASCHEMAS_API
    UsdRelationship CreateTargetTransformsRel() const;

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
