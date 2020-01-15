from . import _mayaUsd
from pxr import Tf
Tf.PrepareModule(_mayaUsd, locals())
del _mayaUsd, Tf
