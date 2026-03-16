import os

def initializeComponentCreator():
    """
    Look to see if the Component Creator Maya plugin is available and if so, do
    nothing since the plugin will initialize itself. If not, try to initialize the
    Component Creator directly.
    """

    haveCCPlugin = False
    if 'MAYA_PLUG_IN_PATH' in os.environ:
        pluginPath = os.environ['MAYA_PLUG_IN_PATH']
        for pp in pluginPath.split(os.path.pathsep):
            if os.path.exists(pp) and ('usd_component_creator.py'in os.listdir(pp)):
                haveCCPlugin = True
                break

    if not haveCCPlugin:
        try:
            from usd_component_creator_plugin import initialize as initializeCC
            initializeCC()
        except:
            print('Failed to initialize AdskUsdComponentCreator.')
