
class KaraboError(Exception):
    """A :class:`KaraboError` is raised if an error occurs which is
    specific to Karabo. This is mostly because things went wrong on
    the other end of a network connection.
    """
