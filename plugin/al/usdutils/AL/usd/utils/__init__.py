from pxr import Tf
import _AL_USDUtils
Tf.PrepareModule(_AL_USDUtils, locals())
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

