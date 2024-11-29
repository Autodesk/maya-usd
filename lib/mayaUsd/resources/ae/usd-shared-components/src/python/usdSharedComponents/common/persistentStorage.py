try:
    from PySide6.QtCore import QSettings  # type: ignore
except:
    from PySide2.QtCore import QSettings  # type: ignore


class PersistentStorage(object):
    _instance = None

    def __init__(self):
        raise RuntimeError("Call instance() instead")

    @classmethod
    def instance(cls):
        if cls._instance is None:
            cls._instance = cls.__new__(cls)
        return cls._instance

    @classmethod
    def injectInstance(cls, persistentStorage):
        cls._instance = persistentStorage

    def _settings(self) -> QSettings:
        return QSettings("settings.ini", QSettings.IniFormat)

    def set(self, group: str, key: str, value: object):
        settings: QSettings = self._settings()
        settings.beginGroup(group)
        settings.setValue(key, value)
        settings.endGroup()
        settings.sync()

    def get(self, group: str, key: str, default: object = None) -> object:
        settings: QSettings = self._settings()
        settings.beginGroup(group)
        return settings.value(key, default, type=type(default))
