import shutil
import tempfile

class TemporaryDirectory:
    '''
    Context manager that creates a temporary directory and deletes it on exit,
    so it's usable with "with" statement.
    '''
    def __init__(self, suffix=None, prefix=None, ignore_errors=True, keep_files=False):
        # Note: the default for suffix and prefix changed between Python 2.7 and 3.X
        #       from empty strings to None. To be compatible with both, we won't
        #       pass values when we want the default, so we have these awkward if elif.
        if suffix and prefix:
            self.name = tempfile.mkdtemp(suffix=suffix, prefix=prefix)
        elif suffix:
            self.name = tempfile.mkdtemp(suffix=suffix)
        elif prefix:
            self.name = tempfile.mkdtemp(prefix=prefix)
        else:
            self.name = tempfile.mkdtemp()
        self.ignore_errors = ignore_errors
        self.keep_files = keep_files

    def __enter__(self):
        return self.name

    def __exit__(self, exc_type, exc_value, traceback):
        if self.keep_files:
            return
        try:
            shutil.rmtree(self.name)
        except Exception:
            if not self.ignore_errors:
                raise