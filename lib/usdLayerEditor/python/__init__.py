from pxr import Tf
if hasattr(Tf, 'PreparePythonModule'):
    Tf.PreparePythonModule('_UsdLayerEditor')
else:
    from . import _UsdLayerEditor
    Tf.PrepareModule(_UsdLayerEditor, locals())
    del _UsdLayerEditor
del Tf