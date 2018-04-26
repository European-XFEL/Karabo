import os

from karabo.common.states import State, StateSignifier

from .basetypes import newest_timestamp


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


def mostSignificantState(iterable, signifier=None):
    """Return the most significant state from the iterable with KaraboValues

    Attaches the newest timestamp to the most significant state

    :param iterable: iterable with KaraboValues (States)
    :param signifier: Optional state signifier, otherwise default is used.
    """
    if signifier is None:
        signifier = StateSignifier(
            staticMoreSignificant=State.PASSIVE,
            changingMoreSignificant=State.DECREASING)

    state = signifier.returnMostSignificant(iterable)
    state.timestamp = newest_timestamp(iterable)

    return state
