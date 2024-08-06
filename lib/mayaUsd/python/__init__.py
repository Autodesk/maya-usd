from pxr import Tf
if hasattr(Tf, 'PreparePythonModule'):
    Tf.PreparePythonModule('_mayaUsd')
else:
    from . import _mayaUsd
    Tf.PrepareModule(_mayaUsd, locals())
    del _mayaUsd
del Tf

# Import some of the names from the usdUfe module into this (mayaUsd.lib) module.
# During the transition this helps by not having to update all the maya-usd tests.
from usdUfe import registerEditRouter
from usdUfe import registerStageLayerEditRouter
from usdUfe import restoreDefaultEditRouter
from usdUfe import restoreAllDefaultEditRouters
from usdUfe import OperationEditRouterContext
from usdUfe import AttributeEditRouterContext
from usdUfe import registerUICallback
from usdUfe import unregisterUICallback