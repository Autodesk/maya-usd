from typing import Sequence
from pxr import Usd


class Host(object):
    _instance = None

    def __init__(self):
        raise RuntimeError("Call instance() instead")

    @classmethod
    def instance(cls):
        if cls._instance is None:
            cls._instance = cls.__new__(cls)
        return cls._instance

    @classmethod
    def injectInstance(cls, host):
        cls._instance = host

    @property
    def canPick(self) -> bool:
        return False

    @property
    def canDrop(self) -> bool:
        return True

    def pick(self, stage: Usd.Stage, *, dialogTitle: str = "") -> Sequence[Usd.Prim]:
        return None