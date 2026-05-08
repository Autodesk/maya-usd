import maya.cmds as cmds
from functools import partial
import AdskAssetResolver as ar
from pxr import Ar as pxrAr
import usdUfe

# Identifiers for various Maya UI elements that need to be known globally
# to be removed later when shutting down the plugin.
_asset_resolver_dialog_menu_item_id = '_asset_resolver_dialog_menu_item'
_asset_resolver_dialog_full_menu_item_id = None

def get_maya_version():
    """
    Returns the Maya release year as an integer
    derived from the Maya API version. 
    Note : cmds.about(version=True) would be more direct, but it doesn't
    return the year in preview releases.
    """
    api_version = cmds.about(apiVersion=True)
    return api_version // 10000

def get_resolver_version():
    ''' Get the version of the Autodesk Asset Resolver. '''
    try:
        adskResolver = pxrAr.GetUnderlyingResolver()
        if adskResolver is not None:
            version = ar.VersionInfo()
            arVersion = version.split("\n")[0]
            arVersion = arVersion.split(" ")[0]
            print("Autodesk Asset Resolver Version:", arVersion)
            return arVersion
    except Exception as e:
        # The Autodesk Asset Resolver is not available
        print("Error getting Autodesk Asset Resolver version:", e)
        return None

def meets_min_version(targetVersion):
    ''' Compare two version strings of the Resolver. '''
    def version_tuple(v):
        return tuple(map(int, (v.split("."))))

    currentVersion = get_resolver_version()
    if currentVersion is None:
        return False

    return (version_tuple(currentVersion) >= version_tuple(targetVersion))

def initialize_menus():
    '''
    Add menus and menu items to the Maya UI.
    Registers a callback for the mayaUsdWindowsMenuLoaded event to ensure
    the Asset Resolver menu is added after maya-usd creates the USD Tools menu.
    '''
    usdUfe.registerUICallback('mayaUsdWindowsMenuLoaded', _on_maya_usd_windows_menu_loaded)

def cleanup_menus():
    '''
    Remove menus and menu items from the Maya UI.
    '''
    usdUfe.unregisterUICallback('mayaUsdWindowsMenuLoaded', _on_maya_usd_windows_menu_loaded)

    global _asset_resolver_dialog_full_menu_item_id
    if _asset_resolver_dialog_full_menu_item_id and cmds.menuItem(
        _asset_resolver_dialog_full_menu_item_id, exists=True
    ):
        cmds.deleteUI(_asset_resolver_dialog_full_menu_item_id, mi=True)
    _asset_resolver_dialog_full_menu_item_id = None

def _on_maya_usd_windows_menu_loaded(context, data):
    '''
    Callback triggered when maya-usd has loaded its Windows menu items.
    This is called via the mayaUsdWindowsMenuLoaded UI callback event.
    '''
    if not data or not isinstance(data, dict) or "usdToolsMenu" not in data:
        return

    _add_asset_resolver_menu_item(data["usdToolsMenu"])

def _add_asset_resolver_menu_item(usd_tools_menu: str):
    # The "USD Tools" menu is created with `menuItem -subMenu true`, so it is a
    # menuItem (not a top-level menu). `cmds.menu(..., exists=True)` only returns
    # True for top-level menus; submenus must be probed with `cmds.menuItem`.
    if not usd_tools_menu or not cmds.menuItem(usd_tools_menu, exists=True):
        return

    global _asset_resolver_dialog_full_menu_item_id

    # Guard against double-add (e.g. on plugin reload during a session).
    if _asset_resolver_dialog_full_menu_item_id and cmds.menuItem(
        _asset_resolver_dialog_full_menu_item_id, exists=True
    ):
        return

    _asset_resolver_dialog_full_menu_item_id = cmds.menuItem(
        _asset_resolver_dialog_menu_item_id,
        version=str(get_maya_version()),
        parent=usd_tools_menu,
        enableCommandRepeat=False,
        label='USD Path Editor',
        annotation='Resolve Paths',
        command=partial(_open_asset_resolver_path_editor_dialog),
    )
    cmds.setParent('..', menu=True)

def _open_asset_resolver_path_editor_dialog(*args):
    # Opens the Asset Resolver dialog's paths tab.
    # *args absorbs the menu item state Maya passes to menuItem command callbacks.
    cmds.assetResolverDialog(tab="paths")