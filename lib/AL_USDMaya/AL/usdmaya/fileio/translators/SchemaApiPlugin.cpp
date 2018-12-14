#include "maya/MFnAttribute.h"
#include "maya/MNodeClass.h"
#include "maya/MObjectArray.h"
#include "maya/MString.h"
#include "AL/usdmaya/fileio/translators/SchemaApiPlugin.h"

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
TF_REGISTRY_FUNCTION(TfType)
{
  TfType::Define<SchemaPluginBase>();
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
