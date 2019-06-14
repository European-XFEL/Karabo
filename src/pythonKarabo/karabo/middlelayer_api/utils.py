from functools import reduce
import os


def get_karabo_version():
    """Return the karabo version from the KARABO VERSION file
    """
    try:
        path = os.path.join(os.environ['KARABO'], 'VERSION')
    except KeyError:
        print("ERROR: $KARABO is not defined. Make sure you have sourced "
              "the 'activate' script.")
        return ''
    with open(path, 'r') as fp:
        version = fp.read()
    return version


def get_property(device, path):
    """Return the property value from a proxy or device

    :param device: The device instance or proxy object
    :param path: The full path of the property as string

    This function is used with::

        prop = get_property(proxy, 'node.subnode.value')
    """
    return reduce(lambda obj, key: getattr(obj, key), path.split('.'), device)
