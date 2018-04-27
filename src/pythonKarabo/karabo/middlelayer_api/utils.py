import os

from karabo.common.states import StateSignifier as SignifierBase

from .basetypes import wrap_methods


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


@wrap_methods
class StateSignifier(SignifierBase):
    """Wrapper of the StateSignifier to provide newest timestamp
    """
