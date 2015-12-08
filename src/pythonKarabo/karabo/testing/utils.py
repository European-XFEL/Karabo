from contextlib import contextmanager
import os
import tempfile


@contextmanager
def temp_file(suffix='', prefix='tmp', dir=None):
    """ Create a temporary file wrapped in a context manager.
    Usage is straightforward:
        with temp_file() as path:
            # Write a file to path

        # All traces of path are now gone
    """
    fd, filename = tempfile.mkstemp(suffix=suffix, prefix=prefix, dir=dir)
    try:
        yield filename
    finally:
        os.close(fd)
        os.unlink(filename)
