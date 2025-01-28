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
            return func(self, *args, **kwargs)
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
            return func(self, *args, **kwargs)
        return wrapper
    return validator

def detectDataConflict(func: Callable) -> Callable:
    '''
    Check if the collection include/exclude conflicts with the expression
    before and after the call, emitting the signal if the conflict status changed.
    '''
    def wrapper(self, *args, **kwargs):
        wasConflicted = self.hasDataConflict()
        result = func(self, *args, **kwargs)
        isConflicted = self.hasDataConflict()
        if wasConflicted != isConflicted:
            self.reportDataConflict(isConflicted)
        return result
    return wrapper
