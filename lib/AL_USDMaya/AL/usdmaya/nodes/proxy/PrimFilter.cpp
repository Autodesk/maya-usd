#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/proxy/PrimFilter.h"
#include "AL/usdmaya/fileio/SchemaPrims.h"

namespace AL {
namespace usdmaya {
namespace nodes {
namespace proxy {

//----------------------------------------------------------------------------------------------------------------------
PrimFilter::PrimFilter(const SdfPathVector& previousPrims, const std::vector<UsdPrim>& newPrimSet, nodes::ProxyShape* proxy)
  : m_newPrimSet(newPrimSet), m_transformsToCreate(), m_updatablePrimSet(), m_removedPrimSet()
{
  // copy over original prims
  m_removedPrimSet.assign(previousPrims.begin(), previousPrims.end());
  std::sort(m_removedPrimSet.begin(), m_removedPrimSet.end(),  [](const SdfPath& a, const SdfPath& b){ return b < a; } );

  // to see if no prims are found
  TfToken nullToken;

  auto manufacture = proxy->translatorManufacture();
  auto context = proxy->context();
  fileio::SchemaPrimsUtils schemaPrimUtils(manufacture);
  for(auto it = m_newPrimSet.begin(); it != m_newPrimSet.end(); )
  {
    UsdPrim prim = *it;
    auto lastIt = it;
    ++it;

    SdfPath path = prim.GetPath();

    // check previous prim type (if it exists at all?)
    TfToken type = context->getTypeForPath(path);
    TfToken newType = prim.GetTypeName();
    fileio::translators::TranslatorRefPtr translator = manufacture.get(newType);

    if(nullToken == type)
    {
      std::cout << "not found" << std::endl;
      // all good, prim will remain in the new set (we have no entry for it)
      if(translator->needsTransformParent())
      {
        m_transformsToCreate.push_back(prim);
      }
    }
    else
    {
      std::cout << "found" << std::endl;
      // if the type remains the same, and the type supports update
      if(translator->supportsUpdate())
      {
        std::cout << "supports update" << std::endl;
        if(type == prim.GetTypeName())
        {
          std::cout << "prim remains the same" << std::endl;
          // add to updatable prim list
          m_updatablePrimSet.push_back(prim);

          // locate the path and delete from the removed set (we do not want to delete this prim!
          auto iter = std::lower_bound(m_removedPrimSet.begin(), m_removedPrimSet.end(), path, [](const SdfPath& a, const SdfPath& b){ return b < a; } );
          if(iter != removedPrimSet().end() && *iter == path)
          {
            std::cout << "removing from removed set" << std::endl;
            m_removedPrimSet.erase(iter);
            it = m_newPrimSet.erase(lastIt);
          }
        }
      }
      else
      {
        // should always be the case?
        if(translator)
        {
          // if we need a transform, make a note of it now
          if(translator->needsTransformParent())
          {
            m_transformsToCreate.push_back(prim);
          }
        }
        else
        {
          // we should be able to delete this prim from the set??
          // We shouldn't really get here however!
          it = m_newPrimSet.erase(lastIt);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
} // proxy
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
