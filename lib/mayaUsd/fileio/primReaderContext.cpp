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
#include "primReaderContext.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaPrimReaderContext::UsdMayaPrimReaderContext(ObjectRegistry* pathNodeMap)
    : _prune(false)
    , _timeSampleMultiplier(1.0)
    , _pathNodeMap(pathNodeMap)
{
}

MObject UsdMayaPrimReaderContext::GetMayaNode(const SdfPath& path, bool findAncestors) const
{
    // Get Node parent
    if (_pathNodeMap) {
        for (SdfPath parentPath = path; !parentPath.IsEmpty();
             parentPath = parentPath.GetParentPath()) {
            // retrieve from a registry since nodes have not yet been put into DG
            ObjectRegistry::iterator it = _pathNodeMap->find(parentPath.GetString());
            if (it != _pathNodeMap->end()) {
                return it->second;
            }

            if (!findAncestors) {
                break;
            }
        }
    }
    return MObject::kNullObj; // returning MObject::kNullObj indicates that the parent is the root
                              // for the scene
}

void UsdMayaPrimReaderContext::StartNewMayaNodeTracking()
{
    _trackedNewMayaNodes.reset(new ObjectRegistry);
}

const UsdMayaPrimReaderContext::ObjectRegistry&
UsdMayaPrimReaderContext::GetTrackedNewMayaNodes() const
{
    if (_trackedNewMayaNodes) {
        return *_trackedNewMayaNodes;
    } else {
        static ObjectRegistry _emptyRegistry;
        return _emptyRegistry;
    }
}

void UsdMayaPrimReaderContext::StopNewMayaNodeTracking() { _trackedNewMayaNodes.reset(); }

void UsdMayaPrimReaderContext::RegisterNewMayaNode(const std::string& path, const MObject& mayaNode)
    const
{
    if (_pathNodeMap) {
        _pathNodeMap->insert(std::make_pair(path, mayaNode));
    }
    if (_trackedNewMayaNodes) {
        _trackedNewMayaNodes->insert(std::make_pair(path, mayaNode));
    }
}

bool UsdMayaPrimReaderContext::GetPruneChildren() const { return _prune; }

/// Sets whether traversal should automatically continue into this prim's
/// children. This only has an effect if set during the
/// UsdMayaPrimReader::Read() step, and not in the
/// UsdMayaPrimReader::PostReadSubtree() step, since in the latter, the
/// children have already been processed.
void UsdMayaPrimReaderContext::SetPruneChildren(bool prune) { _prune = prune; }

double UsdMayaPrimReaderContext::GetTimeSampleMultiplier() const { return _timeSampleMultiplier; };

void UsdMayaPrimReaderContext::SetTimeSampleMultiplier(double multiplier)
{
    _timeSampleMultiplier = multiplier;
};

PXR_NAMESPACE_CLOSE_SCOPE
