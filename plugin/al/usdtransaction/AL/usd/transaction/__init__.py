from pxr import Tf
import _AL_USDTransaction
Tf.PrepareModule(_AL_USDTransaction, locals())
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


class ScopedTransaction(object):

    def __init__(self, stage, layer):
        self.transaction = _AL_USDTransaction.Transaction(stage, layer)

    def __enter__(self):
        return self.transaction.Open()

    def __exit__(self, type, value, traceback):
        return self.transaction.Close()
