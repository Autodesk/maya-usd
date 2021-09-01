from pxr import Tf
if hasattr(Tf, 'PreparePythonModule'):
    Tf.PreparePythonModule('_mayaUsd')
else:
    from . import _mayaUsd
    Tf.PrepareModule(_mayaUsd, locals())
    del _mayaUsd
del Tf
