#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
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
#############################################################################
import uuid

from traits.api import Bool, String

from karabo.common.api import BaseSavableModel


class BaseProjectObjectModel(BaseSavableModel):
    """A base class for all things which can be serialized and sent to
    a Project server.
    """

    # A simple, human-readable name. Doesn't need to be unique
    simple_name = String

    # Version and unique id
    uuid = String

    # Last modified date as string
    date = String(transient=True)

    # Are we in conflict with items on the db
    conflict = Bool(False, transient=True)

    def _uuid_default(self):
        """If a uuid isn't supplied, generate one"""
        return str(uuid.uuid4())

    def _uuid_changed(self, old, new):
        """Validate user-supplied UUIDs.

        `uuid.UUID` will raise an exception if you give it a string which is
        not a proper UUID hex-string.
        """
        try:
            uuid.UUID(new)
        except ValueError:
            # Reset to a safe value
            self.uuid = old
            raise

    def reset_uuid(self):
        """Reset the ``uuid``"""
        self.uuid = str(uuid.uuid4())
