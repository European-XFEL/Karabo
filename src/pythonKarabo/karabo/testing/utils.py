# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import os
import tempfile
from contextlib import contextmanager
from xml.dom import minidom
from xml.etree.ElementTree import fromstring
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
def temp_cwd(path):
    """ Change the current working directory temporarily using a context
    manager.
    """
    orig_path = os.getcwd()
    try:
        os.chdir(path)
        yield
    finally:
        os.chdir(orig_path)


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


@contextmanager
def temp_xml_file(contents):
    """ Just like temp_file(), but writes ``contents`` to the file first.
    """
    with temp_file() as filename:
        with open(filename, 'w', encoding="utf-8") as fp:
            fp.write(contents)
        yield filename


def xml_is_equal(xmlstr0, xmlstr1):
    """ Compare two chunks of XML
    """
    root0 = fromstring(xmlstr0)
    root1 = fromstring(xmlstr1)

    def compare_attrs(el0, el1):
        for k, v in el0.items():
            attr = el1.get(k)
            if attr is None or v != attr:
                return False  # pragma: no cover

        return True

    def compare_tree(elem0, elem1):
        for child0, child1 in zip(list(elem0), list(elem1)):
            if child0.tag != child1.tag:
                return False  # pragma: no cover
            if not compare_attrs(child0, child1):
                return False  # pragma: no cover
            if not compare_tree(child0, child1):
                return False  # pragma: no cover
        return True

    return compare_attrs(root0, root1) and compare_tree(root0, root1)
