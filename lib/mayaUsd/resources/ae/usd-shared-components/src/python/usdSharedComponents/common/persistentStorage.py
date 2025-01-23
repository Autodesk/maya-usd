try:
    from PySide6.QtCore import QSettings  # type: ignore
except:
    from PySide2.QtCore import QSettings  # type: ignore


class PersistentStorage(object):
    '''
    Save and load preferences in persistent storage.

    Default implementation writes a settings.ini file in the current directory of the process.
    '''
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
        '''
        Save the given preference value to the given named preference group
        and item key in the DCC-specific preference system.
        '''
        settings: QSettings = self._settings()
        settings.beginGroup(group)
        settings.setValue(key, value)
        settings.endGroup()
        settings.sync()

    def get(self, group: str, key: str, default: object = None) -> object:
        '''
        Load the the given named preference item key from the given group
        from the DCC-specific preference system.

        Return the optional given default value if not found.
        '''
        settings: QSettings = self._settings()
        settings.beginGroup(group)
        return settings.value(key, default, type=type(default))