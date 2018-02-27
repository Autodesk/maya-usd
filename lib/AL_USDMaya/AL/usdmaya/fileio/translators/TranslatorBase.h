//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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
#pragma once

#include "maya/MDagPath.h"

#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/usd/usd/prim.h"

#include <iostream>
#include <unordered_map>
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The base class interface of all translator plugins. The absolute minimum a translator plugin must implement
///         are the following 3 methods:
///
///           \li \b initialize : Each translator has an opportunity to initialize itself. This should be used to
///               perform any internal initialization (for example, using an MNodeClass to extract MObjects for the
///               attributes you intend to translate, rather than using MFnDependencyNode::findPlug())
///           \li \b import : This should be used to create all node(s) that your translator will generate.
///           \li \b tearDown : If a variant switch occurs that removes/replaces your node type, your translator will
///               be asked to remove the nodes it created from the scene. In this method, delete any nodes you created.
///
///         This will enable your plug-in to work correctly with variant switching. In addition, you may wish to
///         overload the following methods:
///
///           \li \b needsTransformParent : If the maya node you will be creating is a DAG shape or custom transform,
///               then by default this method returns true, which will provide you with a transform node under which
///               you can parent your DAG shape.
///           \li \b postImport : The order in which the usd prims are imported into the scene is undefined, so
///               if you wish to make any connections between Maya nodes, then that should be done here (i.e. when you
///               are sure that all nodes will exist within the scene).
///           \li \b preTearDown : Prior to being notified of a tear down notification (as a result of a variant switch)
///               if your plugin needs to save some state (e.g. layout or animation tweaks), then that data should be
///               serialised in the preTearDown event. This method should not delete any nodes or make changes to the
///               Maya scene. It is your last chance to inspect and serialise data prior to any nodes being deleted
///               as a result of the tearDown.
///           \li \b update : If a variant switch occurs that leaves a prim intact (but may have changed some attribute
///               values), then you can override this method to simply copy the attributes values from the prim onto the
///               existing maya nodes. This is often faster than destroying and recreating the nodes. If you implement
///               this method, you must override \b supportsUpdate to return true.
///
///         Do not inherit from this class directly - use the TranslatorBase instead.
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
class TranslatorAbstract
  : public TfRefBase, public TfWeakBase
{
public:
  typedef TranslatorAbstract This; ///< this type
  typedef TfRefPtr<This> RefPtr; ///< the type of a reference this type
  typedef TfWeakPtr<This> Ptr; ///< weak pointer to this type

  /// \brief  dtor
  virtual ~TranslatorAbstract() {}

  /// \brief  Override to specify the schema type of the prim that this translator plugin is responsible for.
  /// \return the custom schema type associated with this prim
  virtual TfType getTranslatedType() const = 0;

  /// \brief  if the custom node type you are importing requires a parent transform (e.g. you are importing a shape node),
  ///         then this method should return true. If however you do not need a parent transform (e.g. you are importing
  ///         a texture!), then you can return false here.
  /// \return true if this type of node requires a transform as a parent node (i.e. this is a DAG node), false if the node
  ///         if a dependency node
  virtual bool needsTransformParent() const
    { return true; }

  /// \brief  Override this to do a one time initialization of your translator. Primarily this is to allow you to
  ///         extract some MObject attribute handles from an MNodeClass, to avoid the need for calling findPlug at
  ///         runtime (and the inherent cost of the strcmps/hash lookup that entails)
  /// \return MS::kSuccess if all ok
  virtual MStatus initialize()
    { return MS::kSuccess; }

  /// \brief  Override this method to import a prim into your scene.
  /// \param  prim the usd prim to be imported into maya
  /// \param  parent a handle to an MObject that represents an AL_usd_Transform node. You should parent your DAG
  ///         objects under this node. If the prim you are importing is NOT a DAG object (e.g. surface shader, etc),
  ///         then you can ignore this parameter.
  /// \return MS::kSuccess if all ok
  virtual MStatus import(const UsdPrim& prim, MObject& parent)
    { return MS::kSuccess; }

  /// \brief  If your node needs to set up any relationships after import (for example, adding the node to a set, or
  ///         making attribute connections), then all of that work should be performed here.
  /// \param  prim the prim we are importing.
  /// \return MS::kSuccess if all ok
  virtual MStatus postImport(const UsdPrim& prim)
    { return MS::kSuccess; }

  /// \brief  This method will be called prior to the tear down process taking place. This is the last chance you have
  ///         to do any serialisation whilst all of the existing nodes are available to query.
  /// \param  prim the prim that may be modified or deleted as a result of a variant switch
  /// \return MS::kSuccess if all ok
  virtual MStatus preTearDown(UsdPrim& prim)
    { return MS::kSuccess; }

  /// \brief  If your plugin creates any nodes within Maya, then this method should be overridden to remove those
  ///         nodes. This forms a central role within the variant switching system.
  /// \param  path  the path to the prim that should be removed. \b CRASH_ALERT : It is vitally important that you do
  ///         not attempt to access the prim from the current usd stage. It is entirely possible that a variant switch
  ///         has removed the prim from the stage, so any attempts to access the UsdPrim via this path, is more than
  ///         likely to crash Maya.
  /// \return MS::kSuccess if all ok
  virtual MStatus tearDown(const SdfPath& path)
    { return MStatus::kNotImplemented; }

  /// \brief  override this method and return true if the translator supports update
  /// \return true if your plugin supports update, false otherwise.
  virtual bool supportsUpdate() const
    { return false; }

  /// \brief If a translator is importableByDefault=true, it will always be automatically imported on ProxyShape
  /// initialisation.
  /// \return returns true if the Translator doesn't automatically import the Prim.
  virtual bool importableByDefault() const
    { return true; }

  /// \brief  Optionally override this method to copy the attribute values from the prim onto the Maya nodes you have
  ///         created.
  /// \param  prim  the prim
  /// \return MS::kSuccess if all ok
  virtual MStatus update(const UsdPrim& prim)
    { return MStatus::kNotImplemented; }

};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Base class for maya translator usd plugins. The TfType of these plugins has to be derived from the base
///         TfType, TranslatorBase.
  /// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
class TranslatorBase
  : public TranslatorAbstract
{
public:
  typedef TranslatorBase This;
  typedef TfRefPtr<This> RefPtr;

  /// \brief  dtor
  virtual ~TranslatorBase()
    {}

  /// \brief  returns the translated prim type
  virtual TfType getTranslatedType() const override
    { return m_translatedType; }

  /// \brief  returns the context currently being used to translate the USD prims. The context can be used to add
  ///         references to prims you have created in your translator plugins (see:
  virtual TranslatorContextPtr context() const
    { return m_context; }

  /// \brief  Internal method used to create a new instance of a plugin translator
  /// \param  primType the type of translator to manufacture
  /// \param  context the translation context
  /// \return a handle to the newly created plugin translator
  static RefPtr manufacture(const std::string& primType, TranslatorContextPtr context) = delete;

  /// \brief  Internal method used to create a new instance of a plugin translator
  /// \param  primType the type of translator to manufacture
  /// \param  context the translation context
  /// \return a handle to the newly created plugin translator
  virtual MStatus preTearDown(UsdPrim& prim)
    {
      m_isTearingDown = true;
      return MS::kSuccess;
    }

  /// \brief  return the usd stage associated with this context
  /// \return the usd stage
  UsdStageRefPtr getUsdStage() const
    { return context()->getUsdStage(); }

  /// \brief If the translator has had pretearDown called on it then this will return true.
  /// \return true if this prim has had the pretearDown called on it.
  bool isTearingDown() const
    { return true; }

protected:

  /// \brief  internal method. Used within AL_USDMAYA_DEFINE_TRANSLATOR macro to set the schema type of the node we
  ///         translate.
  /// \param  translatedType the schema type name
  virtual void setTranslatedType(const TfType& translatedType)
    { m_translatedType = translatedType; }

  /// \brief  internal method. Used within AL_USDMAYA_DEFINE_TRANSLATOR macro to set the translation context
  /// \param  context the internal context
  virtual void setContext(const TranslatorContextPtr context)
    { m_context = context; }

private:
  bool m_isTearingDown = false;

  TfType m_translatedType;
  TranslatorContextPtr m_context;
};

typedef TfRefPtr<TranslatorBase> TranslatorRefPtr;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Forms a registry of all plug-in translator types registered
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
class TranslatorManufacture
{
public:
  typedef TfRefPtr<TranslatorBase> RefPtr; ///< handle to a plug-in translator

  /// \brief  constructs a registry of translator plugins that are currently registered within usd maya. This construction
  ///         should only happen once per-proxy shape.
  /// \param  context the translator context for this registry
  TranslatorManufacture(TranslatorContextPtr context);

  /// \brief  returns a translator for the specified prim type.
  /// \param  type_name the scheman name
  /// \return returns the requested translator type
  RefPtr get(const TfToken type_name);

private:
  std::unordered_map<std::string, TranslatorRefPtr> m_translatorsMap;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  the factory interface, used to create an instance of a particular translator type
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
class TranslatorFactoryBase
  : public TfType::FactoryBase
{
public:
  /// \brief  overridden by the TranslatorFactory to create a new translator for a given type
  /// \param  ctx the current translator context
  /// \return the plugin translator
  virtual TfRefPtr<TranslatorBase> create(TranslatorContextPtr ctx) const = 0;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  the factory instance for a given translator type
  /// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
template <typename T>
class TranslatorFactory : public TranslatorFactoryBase
{
public:
  /// \brief  creates a new translator for a given type T
  /// \param  ctx the current translator context
  /// \return the plugin translator associated with type T
  virtual TfRefPtr<TranslatorBase> create(TranslatorContextPtr ctx) const
    { return T::create(ctx); }
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  a macro to declare a plug-in translator
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
#define AL_USDMAYA_DECLARE_TRANSLATOR(PlugClass)                                \
typedef PlugClass This;                                                         \
typedef TfRefPtr<This> RefPtr;                                                  \
typedef TfWeakPtr<This> Ptr;                                                    \
static RefPtr create(TranslatorContextPtr context);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  a macro to define a plug-in translator
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
#define AL_USDMAYA_DEFINE_TRANSLATOR(PlugClass, TranslatedType)                 \
TfRefPtr<PlugClass>                                                             \
PlugClass::create(TranslatorContextPtr context) {                               \
  TfType const &type = TfType::Find<TranslatedType>();                          \
  if(not type.IsUnknown()) {                                                    \
    TfRefPtr<PlugClass> plugin = TfCreateRefPtr(new This());                    \
    plugin->setTranslatedType(type);                                            \
    plugin->setContext(context);                                                \
    if(!plugin->initialize()) return TfRefPtr<PlugClass>();                     \
    return plugin;                                                              \
  }                                                                             \
  else {                                                                        \
    TF_CODING_ERROR(                                                            \
      "Failed to get %s usd type, maybe the needed plugin is not loaded",       \
      typeid(TranslatedType).name());                                           \
    return TfNullPtr;                                                           \
  }                                                                             \
}                                                                               \
                                                                                \
TF_REGISTRY_FUNCTION(TfType)                                                    \
{                                                                               \
    TfType::Define<PlugClass, TfType::Bases<TranslatorBase>>()                  \
        .SetFactory<TranslatorFactory<PlugClass>>();                            \
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------

