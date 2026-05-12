import os
import sys

def _isCCAvailable():
    # Check to see if the AdskUsdComponentCreator exist in python path. If so it means
    # we have the Component Creator installed with this MayaUsd.
    for pp in sys.path:
        if not os.path.exists(pp):
            continue
        with os.scandir(path=pp) as entries:
            for entry in entries:
                if entry.is_dir() and entry.name == 'AdskUsdComponentCreator':
                    return True
    return False


# The USD component creator Maya plugin will only be there in certain circumstances so
# only try and load it if the CC Maya plugin actually exists.
def haveMayaCCPlugin():
    if 'MAYA_PLUG_IN_PATH' in os.environ:
        pluginPath = os.environ['MAYA_PLUG_IN_PATH']
        for pp in pluginPath.split(os.path.pathsep):
            if os.path.exists(pp) and ('usd_component_creator.py' in os.listdir(pp)):
                return True
        return False


_CC_AVAILABLE = _isCCAvailable()
_HAVE_CC_MAYA_PLUGIN = haveMayaCCPlugin()

