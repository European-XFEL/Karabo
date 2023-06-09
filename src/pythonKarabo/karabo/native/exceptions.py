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
import logging


class KaraboError(Exception):
    """A :class:`KaraboError` is raised if an error occurs which is
    specific to Karabo. This is mostly because things went wrong on
    the other end of a network connection.
    """

    def __init__(self, *args, loglevel=logging.ERROR, **kwargs):
        super().__init__(*args, **kwargs)
        self.loglevel = loglevel
