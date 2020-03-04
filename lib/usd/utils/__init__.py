from pxr import Tf
import _mayaUsdUtils
Tf.PrepareModule(_mayaUsdUtils, locals())
del Tf

try:
    import __DOC
    __DOC.Execute(locals())
    del __DOC
except Exception:
    try:
        import __tmpDoc
        __tmpDoc.Execute(locals())
        del __tmpDoc
    except:
        pass

