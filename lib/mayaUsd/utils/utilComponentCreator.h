#ifndef MAYAUSD_UTILS_UTILCOMPONENTCREATOR_H
#define MAYAUSD_UTILS_UTILCOMPONENTCREATOR_H

#include "mayaUsd/mayaUsd.h"

#include <mayaUsd/base/api.h>

#include <string>
#include <vector>

namespace MAYAUSD_NS_DEF {
namespace ComponentUtils {

MAYAUSD_CORE_PUBLIC
bool isAdskUsdComponent(const std::string& proxyPath);

MAYAUSD_CORE_PUBLIC
std::vector<std::string> getAdskUsdComponentLayersToSave(const std::string& proxyPath);

MAYAUSD_CORE_PUBLIC
void saveAdskUsdComponent(const std::string& proxyPath);

} // namespace ComponentUtils
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UTILS_UTILCOMPONENTCREATOR_H
