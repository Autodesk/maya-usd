//
// Copyright 2024 Autodesk
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

#ifndef USDLAYEREDITOR_ABSTRACTLAYEREDITORWINDOW_H
#define USDLAYEREDITOR_ABSTRACTLAYEREDITORWINDOW_H

#include <string>
#include <vector>

#include "layerEditorAPI.h"
#include "sessionState.h"

namespace UsdLayerEditor {

class AbstractLayerEditorWindow;

/**
 * @brief Abstract class used by layer editor window command to create and get
 * the layer editor windows.
 */
class LayerEditorAPI AbstractLayerEditorCreator
{
public:
    AbstractLayerEditorCreator();
    virtual ~AbstractLayerEditorCreator();

    typedef std::vector<std::string> PanelNamesList;

    /*
     * @brief returns a pointer to the registered  singleton
     *
     * @return AbstractLayerEditorCreator*
     */
    static AbstractLayerEditorCreator* instance();

    /**
     * @brief Create the layer editor window with the given name
     */
    virtual AbstractLayerEditorWindow* createWindow(const char* panelName) = 0;

    /**
     * @brief returns the panel with a given name if it already exists
     */
    virtual AbstractLayerEditorWindow* getWindow(const char* panelName) const = 0;

    /**
     * @brief Gets an array of all the panels that exists
     *
     * @return std::vector<std::string>
     */
    virtual PanelNamesList getAllPanelNames() const = 0;

private:
    static AbstractLayerEditorCreator* _instance;
};

/**
 * @brief Abstract class used to implement the layer editor commands
 */
class LayerEditorAPI AbstractLayerEditorWindow
{
public:
    /**
     * @brief Constructor implemented by the DCC usd layer editor
     *
     * @param panelName this is the name of the control in mel, not the title of the window
     */
    AbstractLayerEditorWindow(const char* panelName);

    /**
     * @brief Virtual Destructor
     */
    virtual ~AbstractLayerEditorWindow();

    // queries about the current selection
    virtual int         selectionLength() = 0;
    virtual bool        isInvalidLayer() = 0;
    virtual bool        isSessionLayer() = 0;
    virtual bool        isLayerDirty() = 0;
    virtual bool        isSubLayer() = 0;
    virtual bool        isAnonymousLayer() = 0;
    virtual bool        isIncomingLayer() = 0;
    virtual bool        layerNeedsSaving() = 0;
    virtual bool        layerAppearsMuted() = 0;
    virtual bool        layerIsMuted() = 0;
    virtual bool        layerAppearsLocked() = 0;
    virtual bool        layerIsLocked() = 0;
    virtual bool        layerAppearsSystemLocked() = 0;
    virtual bool        layerIsSystemLocked() = 0;
    virtual bool        layerIsReadOnly() = 0;
    virtual bool        layerHasSubLayers() = 0;
    virtual std::string dccObjectName() const = 0;

    virtual void removeSubLayer() = 0;
    virtual void saveEdits() = 0;
    virtual void discardEdits() = 0;
    virtual void addAnonymousSublayer() = 0;
    virtual void addParentLayer() = 0;
    virtual void loadSubLayers() = 0;
    virtual void muteLayer() = 0;
    virtual void printLayer() = 0;
    virtual void clearLayer() = 0;
    virtual void mergeWithSublayers() = 0;
    virtual void selectPrimsWithSpec() = 0;
    virtual void updateLayerModel() = 0;
    virtual void lockLayer() = 0;
    virtual void lockLayerAndSubLayers() = 0;
    virtual void stitchLayers() = 0;

    virtual void selectDccObject(const char* objectPath) = 0;

    virtual SessionState* getSessionState() = 0;
};

} // namespace UsdLayerEditor
#endif