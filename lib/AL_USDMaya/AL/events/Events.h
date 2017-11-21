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
#include <functional>
#include "maya/MGlobal.h"

namespace AL {
namespace usdmaya {
namespace events {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Event types which are supported by the Event Manager.
//----------------------------------------------------------------------------------------------------------------------
enum MayaEventType
{
  kBeforeNew, ///< called prior to file new
  kAfterNew, ///< called after to file new
  kBeforeOpen, ///< called prior to file open
  kAfterOpen, ///< called after to file open
  kBeforeSave, ///< called prior to file save
  kAfterSave, ///< called after to file save
  kBeforeReference, ///< called prior to file reference
  kAfterReference, ///< called after to file reference
  kBeforeUnloadReference, ///< called prior to file reference unloaded
  kAfterUnloadReference, ///< called after to file reference unloaded
  kBeforeLoadReference, ///< called prior to file reference loaded
  kAfterLoadReference, ///< called after to file reference loaded
  kBeforeCreateReference, ///< called prior to file reference created
  kAfterCreateReference, ///< called after to file reference created
  kMayaInitialized, ///< called after maya has been initialised
  kMayaExiting, ///< called prior to maya exiting
  kSceneMessageLast
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief Converts supported event type into Maya's event type
/// \return The corresponding Maya event type, else MSceneMessage::kLast
//----------------------------------------------------------------------------------------------------------------------
MSceneMessage::Message EventToMayaEvent(MayaEventType internalEvent)
{
  switch(internalEvent)
  {
    case(kBeforeNew):
	return MSceneMessage::kBeforeNew;
    case(kAfterNew):
	return MSceneMessage::kAfterNew;
    case(kBeforeOpen):
	return MSceneMessage::kBeforeOpen;
    case(kAfterOpen):
	return MSceneMessage::kAfterOpen;
      case(kBeforeSave):
	return MSceneMessage::kBeforeSave;
      case(kAfterSave):
	return MSceneMessage::kAfterSave;
      case(kBeforeReference):
	return MSceneMessage::kBeforeReference;
      case(kAfterReference):
	return MSceneMessage::kAfterReference;
      case(kBeforeUnloadReference):
	return MSceneMessage::kBeforeUnloadReference;
      case(kAfterUnloadReference):
	return MSceneMessage::kAfterUnloadReference;
      case(kBeforeLoadReference):
	return MSceneMessage::kBeforeLoadReference;
      case(kAfterLoadReference):
	return MSceneMessage::kAfterLoadReference;
      case(kBeforeCreateReference):
	return MSceneMessage::kBeforeCreateReference;
      case(kAfterCreateReference):
	return MSceneMessage::kAfterCreateReference;
      case(kMayaInitialized):
	return MSceneMessage::kMayaInitialized;
      case(kMayaExiting):
	return MSceneMessage::kMayaExiting;
  }
  return MSceneMessage::kLast;
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief Converts Maya's event type into supported event type
/// \return The corresponding supported Event type, else MayaEventType::kSceneMessageLast
//----------------------------------------------------------------------------------------------------------------------
MayaEventType EventToMayaEvent(MSceneMessage::Message mayaEvent)
{
  switch(mayaEvent)
  {
    case(MSceneMessage::kBeforeNew):
        return kBeforeNew;
    case(MSceneMessage::kAfterNew):
        return kAfterNew;
    case(MSceneMessage::kBeforeOpen):
        return kBeforeOpen;
    case(MSceneMessage::kAfterOpen):
        return kAfterOpen;
      case(MSceneMessage::kBeforeSave):
        return kBeforeSave;
      case(MSceneMessage::kAfterSave):
        return kAfterSave;
      case(MSceneMessage::kBeforeReference):
        return kBeforeReference;
      case(MSceneMessage::kAfterReference):
        return kAfterReference;
      case(MSceneMessage::kBeforeUnloadReference):
        return kBeforeUnloadReference;
      case(MSceneMessage::kAfterUnloadReference):
        return kAfterUnloadReference;
      case(MSceneMessage::kBeforeLoadReference):
        return kBeforeLoadReference;
      case(MSceneMessage::kAfterLoadReference):
        return kAfterLoadReference;
      case(MSceneMessage::kBeforeCreateReference):
        return kBeforeCreateReference;
      case(MSceneMessage::kAfterCreateReference):
        return kAfterCreateReference;
      case(MSceneMessage::kMayaInitialized):
        return kMayaInitialized;
      case(MSceneMessage::kMayaExiting):
        return kMayaExiting;
  }
  return kSceneMessageLast;
}


// C++ standard had an "oversight" for make_unique. This is copied from Herb Sutter's blog
// See: https://herbsutter.com/gotw/_102/
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

//----------------------------------------------------------------------------------------------------------------------
} // events
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
