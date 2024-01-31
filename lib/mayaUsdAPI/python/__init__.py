from pxr import Tf
if hasattr(Tf, 'PreparePythonModule'):
    Tf.PreparePythonModule('_mayaUsdAPI')
else:
    from . import _mayaUsdAPI
    Tf.PrepareModule(_mayaUsdAPI, locals())
    del _mayaUsdAPI
del Tf