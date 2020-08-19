# Extra Data Translation Plugins 

Extra Data Translator Plugins are an additional plugin mechanism to allow import/export of data which is not tied to a specific USD Schema (Typed or API) or Maya node type.
It maps reasonably well to (for example) the Extension Attributes approach in Maya e.g "add these additional attributes to all Shapes"). 
These translators are executed when they encounter a Maya node based on a specific MFn::Type (or in the case of nodes derived from an MPxFoo type, the text string name of the type).

We wrote this system initially to allow import/export of Renderer-specific data for our proprietary Renderer "Glimpse", for example:
+ Subdivision Settings (Shapes only)
+ Visibility Settings
+ Custom User Data
+ Materials/Shaders and Material Bindings

For Glimpse, we registered different plugins for transforms and Shapes.

We think this is probably a good match for other Renderer use cases also.

Extra Data Plugins get called AFTER any other schema translator has been called (in any context) effectively "decorating" the existing Maya or USD data

They are registered with the AL_USDMAYA_DECLARE_EXTRA_DATA_PLUGIN and don't use the Pixar registration system (pluginfo.json) like the schema translator plugins

Here's an example header for an imaginary renderer, where we register separate plugins for transform and mesh (and types that derive from them)

```
#include "AL/usdmaya/fileio/translators/ExtraDataPlugin.h"
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"
#include <maya/MPlug.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
class TranslatorMyRenderMeshAttributes
  : public ExtraDataPluginBase
{
public:

  AL_USDMAYA_DECLARE_EXTRA_DATA_PLUGIN(TranslatorMyRenderMeshAttributes);

  MFn::Type getFnType() const override
    { return MFn::kMesh; };
  MStatus initialize() override;
  MStatus import(const UsdPrim& prim, const MObject& parent) override;
  MStatus postImport(const UsdPrim& prim) override;
  MStatus update(const UsdPrim& path) override;
  MStatus preTearDown(UsdPrim& prim) override;
  MStatus exportObject(UsdPrim& prim, const MObject& node, const ExporterParams& params) override;
  MStatus writeEdits(MObject object, UsdPrim prim, const ExporterParams& params);
};

//----------------------------------------------------------------------------------------------------------------------
class TranslatorMyRendererTransformAttributes
  : public ExtraDataPluginBase
{
public:

  AL_USDMAYA_DECLARE_EXTRA_DATA_PLUGIN(TranslatorMyRendererTransformAttributes);

  MFn::Type getFnType() const override
    { return MFn::kTransform; };
  MStatus initialize() override;
  MStatus import(const UsdPrim& prim, const MObject& parent) override;
  MStatus postImport(const UsdPrim& prim) override;
  MStatus update(const UsdPrim& path) override;
  MStatus preTearDown(UsdPrim& prim) override;
  MStatus exportObject(UsdPrim& prim, const MObject& node, const ExporterParams& params) override;
  MStatus writeEdits(MObject object, UsdPrim prim, const ExporterParams& params);
};

} // translators
} // fileio
} // usdmaya
} // AL
```

