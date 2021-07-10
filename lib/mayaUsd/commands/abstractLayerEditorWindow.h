#ifndef ABSTRACTLAYEREDITORWINDOW_H
#define ABSTRACTLAYEREDITORWINDOW_H

#include <mayaUsd/base/api.h>

#include <string>
#include <vector>

namespace MAYAUSD_NS_DEF {

class AbstractLayerEditorWindow;

/**
 * @brief Abstract class used by layer editor window command to create and get
 * the layer editor windows.
 * This allows breaking the circular dependency between between maya usd lib
 * and UI libraries.
 *
 */
class MAYAUSD_CORE_PUBLIC AbstractLayerEditorCreator
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
     * @brief Create the maya panel with the given name
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
 * @brief Abstract class used to break dependency between core lib and maya UI,
 * used to implement the layer editor commands
 */
class MAYAUSD_CORE_PUBLIC AbstractLayerEditorWindow
{
public:
    /**
     * @brief Constructor implemented by the maya usd layer editor
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
    virtual bool        layerIsReadOnly() = 0;
    virtual std::string proxyShapeName() const = 0;

    virtual void removeSubLayer() = 0;
    virtual void saveEdits() = 0;
    virtual void discardEdits() = 0;
    virtual void addAnonymousSublayer() = 0;
    virtual void addParentLayer() = 0;
    virtual void loadSubLayers() = 0;
    virtual void muteLayer() = 0;
    virtual void printLayer() = 0;
    virtual void clearLayer() = 0;
    virtual void selectPrimsWithSpec() = 0;

    virtual void selectProxyShape(const char* shapePath) = 0;
};

} // namespace MAYAUSD_NS_DEF

#endif // ABSTRACTLAYEREDITORWINDOW_H
