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
#ifndef AL_USDMAYASCHEMAS_GENERATED_FRAMERANGE_H
#define AL_USDMAYASCHEMAS_GENERATED_FRAMERANGE_H

/// \file AL_USDMayaSchemas/FrameRange.h

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
// ALFRAMERANGE                                                               //
// -------------------------------------------------------------------------- //

/// \class AL_usd_FrameRange
///
/// Maya Frame Range
///
class AL_usd_FrameRange : public UsdTyped
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaType
    static const UsdSchemaType schemaType = UsdSchemaType::ConcreteTyped;

    /// Construct a AL_usd_FrameRange on UsdPrim \p prim .
    /// Equivalent to AL_usd_FrameRange::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit AL_usd_FrameRange(const UsdPrim& prim = UsdPrim())
        : UsdTyped(prim)
    {
    }

    /// Construct a AL_usd_FrameRange on the prim held by \p schemaObj .
    /// Should be preferred over AL_usd_FrameRange(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit AL_usd_FrameRange(const UsdSchemaBase& schemaObj)
        : UsdTyped(schemaObj)
    {
    }

    /// Destructor.
    AL_USDMAYASCHEMAS_API
    virtual ~AL_usd_FrameRange();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    AL_USDMAYASCHEMAS_API
    static const TfTokenVector& GetSchemaAttributeNames(bool includeInherited = true);

    /// Return a AL_usd_FrameRange holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// AL_usd_FrameRange(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    AL_USDMAYASCHEMAS_API
    static AL_usd_FrameRange Get(const UsdStagePtr& stage, const SdfPath& path);

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
    static AL_usd_FrameRange Define(const UsdStagePtr& stage, const SdfPath& path);

protected:
    /// Returns the type of schema this class belongs to.
    ///
    /// \sa UsdSchemaType
    AL_USDMAYASCHEMAS_API
    virtual UsdSchemaType _GetSchemaType() const;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    AL_USDMAYASCHEMAS_API
    static const TfType& _GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    AL_USDMAYASCHEMAS_API
    virtual const TfType& _GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // ANIMATIONSTARTFRAME
    // --------------------------------------------------------------------- //
    /// The start animation frame in Maya.
    ///
    /// \n  C++ Type: double
    /// \n  Usd Type: SdfValueTypeNames->Double
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetAnimationStartFrameAttr() const;

    /// See GetAnimationStartFrameAttr(), and also
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateAnimationStartFrameAttr(
        VtValue const& defaultValue = VtValue(),
        bool           writeSparsely = false) const;

public:
    // --------------------------------------------------------------------- //
    // STARTFRAME
    // --------------------------------------------------------------------- //
    /// The min frame in Maya.
    ///
    /// \n  C++ Type: double
    /// \n  Usd Type: SdfValueTypeNames->Double
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetStartFrameAttr() const;

    /// See GetStartFrameAttr(), and also
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute
    CreateStartFrameAttr(VtValue const& defaultValue = VtValue(), bool writeSparsely = false) const;

public:
    // --------------------------------------------------------------------- //
    // ENDFRAME
    // --------------------------------------------------------------------- //
    /// The max frame in Maya.
    ///
    /// \n  C++ Type: double
    /// \n  Usd Type: SdfValueTypeNames->Double
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetEndFrameAttr() const;

    /// See GetEndFrameAttr(), and also
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute
    CreateEndFrameAttr(VtValue const& defaultValue = VtValue(), bool writeSparsely = false) const;

public:
    // --------------------------------------------------------------------- //
    // ANIMATIONENDFRAME
    // --------------------------------------------------------------------- //
    /// The end animation frame in Maya.
    ///
    /// \n  C++ Type: double
    /// \n  Usd Type: SdfValueTypeNames->Double
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetAnimationEndFrameAttr() const;

    /// See GetAnimationEndFrameAttr(), and also
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateAnimationEndFrameAttr(
        VtValue const& defaultValue = VtValue(),
        bool           writeSparsely = false) const;

public:
    // --------------------------------------------------------------------- //
    // CURRENTFRAME
    // --------------------------------------------------------------------- //
    /// The current frame in Maya.
    ///
    /// \n  C++ Type: double
    /// \n  Usd Type: SdfValueTypeNames->Double
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    AL_USDMAYASCHEMAS_API
    UsdAttribute GetCurrentFrameAttr() const;

    /// See GetCurrentFrameAttr(), and also
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    AL_USDMAYASCHEMAS_API
    UsdAttribute CreateCurrentFrameAttr(
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
