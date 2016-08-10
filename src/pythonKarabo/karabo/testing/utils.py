from contextlib import contextmanager
import os
import tempfile
from xml.dom import minidom
from zipfile import ZipFile


def compare_ndarray_data_ptrs(arr0, arr1):
    """ Return True if two numpy arrays are using the same data
    """
    ptr0 = arr0.__array_interface__['data'][0]
    ptr1 = arr1.__array_interface__['data'][0]

    return ptr0 == ptr1


def compare_zip_files(path0, path1):
    """ Given the paths of two zip files, compare their contents and return
    True if they match.
    """
    with ZipFile(path0, 'r') as zip0, ZipFile(path1, 'r') as zip1:
        names0, names1 = zip0.namelist(), zip1.namelist()
        names0.sort()
        names1.sort()
        if names0 != names1:
            print("names not equal", names0, names1)
            return False

        for name in names0:
            data0 = zip0.read(name)
            data1 = zip1.read(name)
            if data0 != data1:
                # Don't worry about whitespace added to the end of files
                if data0.strip() != data1.strip():
                    return False

    return True


def pretty_print_xml(data):
    """ Pretty print some XML
    """
    dom = minidom.parseString(data)
    print(dom.toprettyxml(indent=' '*2))


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
