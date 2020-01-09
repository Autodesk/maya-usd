from . import _PyMayaUsd
from pxr import Tf
Tf.PrepareModule(_PyMayaUsd, locals())
del _PyMayaUsd, Tf
