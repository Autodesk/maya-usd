//
// Copyright 2017 Animal Logic
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
#pragma once

#include "AL/event/EventHandler.h"
#include "AL/usdmaya/Api.h"

#include <mayaUsdUtils/ForwardDeclares.h>

namespace AL {
namespace usdmaya {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This class wraps all of the global state/mechanisms needed to integrate USD and Maya.
///         This mainly handles things such as onFileNew, preFileSave, etc.
/// \ingroup usdmaya
//----------------------------------------------------------------------------------------------------------------------
class Global
{
public:
    /// \brief  initialise the global state
    AL_USDMAYA_PUBLIC
    static void onPluginLoad();

    /// \brief  uninitialise the global state
    AL_USDMAYA_PUBLIC
    static void onPluginUnload();

    /// pre save callback
    static AL::event::CallbackId preSave() { return m_preSave; }

    /// post save callback
    static AL::event::CallbackId postSave() { return m_postSave; }

    /// pre open callback
    static AL::event::CallbackId preRead() { return m_preRead; }

    /// post open callback
    static AL::event::CallbackId postRead() { return m_postRead; }

    /// callback used to flush the USD caches after a file new
    static AL::event::CallbackId fileNew() { return m_fileNew; }

    static void openingFile(bool val);

private:
    static AL::event::CallbackId
                                 m_preSave; ///< callback prior to saving the scene (so we can store the session layer)
    static AL::event::CallbackId m_postSave; ///< callback after saving
    static AL::event::CallbackId m_preRead;  ///< callback executed before opening a maya file
    static AL::event::CallbackId m_postRead; ///< callback executed after opening a maya file -
                                             ///< needed to re-hook up the UsdPrims
    static AL::event::CallbackId
        m_fileNew; ///< callback used to flush the USD caches after a file new
    static AL::event::CallbackId
                                 m_preExport; ///< callback prior to exporting the scene (so we can store the session layer)
    static AL::event::CallbackId m_postExport; ///< callback after exporting

#if defined(WANT_UFE_BUILD)
    class UfeSelectionObserver;
    static std::shared_ptr<UfeSelectionObserver> m_ufeSelectionObserver;
#endif
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
