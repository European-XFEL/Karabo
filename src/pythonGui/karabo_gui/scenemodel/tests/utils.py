from contextlib import contextmanager
import os
from tempfile import mkstemp
from xml.etree.ElementTree import fromstring


@contextmanager
def temp_file(contents):
    """ Create a temporary file in a context manager. Returns the path of the
    file.
    """
    fd, filename = mkstemp()
    try:
        with open(filename, 'w') as fp:
            fp.write(contents)
        yield filename
    finally:
        os.close(fd)
        os.unlink(filename)


def xml_is_equal(xmlstr0, xmlstr1):
    """ Compare two chunks of XML
    """
    root0 = fromstring(xmlstr0)
    root1 = fromstring(xmlstr1)

    def compare_attrs(el0, el1):
        for k, v in el0.items():
            attr = el1.get(k)
            if attr is None or v != attr:
                return False

        return True

    def compare_tree(elem0, elem1):
        for child0, child1 in zip(list(elem0), list(elem1)):
            if child0.tag != child1.tag:
                return False
            if not compare_attrs(child0, child1):
                return False
            if not compare_tree(child0, child1):
                return False
        return True

    return compare_attrs(root0, root1) and compare_tree(root0, root1)
