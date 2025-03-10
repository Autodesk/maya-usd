from typing import Callable

def validatePrim(defaultReturnValue = None) -> Callable:
    '''
    Validate that the prim kept in the data is still valid.
    If invalid, we will report the error and return the given
    defaultReturnValue.
    '''
    def validator(func: Callable) -> Callable:
        def wrapper(self, *args, **kwargs):
            if not self._prim or not self._prim.IsValid():
                return defaultReturnValue
            try:
                return func(self, *args, **kwargs)
            except AttributeError:
                return defaultReturnValue
        return wrapper
    return validator

def validateCollection(defaultReturnValue = None) -> Callable:
    '''
    Validate that the prim and collection kept in the data is still valid.
    If invalid, we will report the error and return the given
    defaultReturnValue.
    '''
    def validator(func: Callable) -> Callable:
        def wrapper(self, *args, **kwargs):
            if not self._prim or not self._collection or not self._prim.IsValid():
                return defaultReturnValue
            try:
                return func(self, *args, **kwargs)
            except AttributeError:
                return defaultReturnValue
        return wrapper
    return validator

