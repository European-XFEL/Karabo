# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import logging


class KaraboError(Exception):
    """A :class:`KaraboError` is raised if an error occurs which is
    specific to Karabo. This is mostly because things went wrong on
    the other end of a network connection.
    """

    def __init__(self, *args, loglevel=logging.ERROR, **kwargs):
        super().__init__(*args, **kwargs)
        self.loglevel = loglevel
