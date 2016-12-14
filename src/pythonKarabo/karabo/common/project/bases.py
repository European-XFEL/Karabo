#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import uuid

from traits.api import Bool, Int, String

from karabo.common.savable import BaseSavableModel
from .const import EXISTDB_INITIAL_VERSION


class BaseProjectObjectModel(BaseSavableModel):
    """ A base class for all things which can be serialized and sent to
    a Project server.
    """
    # A simple, human-readable name. Doesn't need to be unique
    simple_name = String

    # A description for this object
    description = String

    # When False, the object is known to be uninitialized
    initialized = Bool(False, transient=True)

    # Version and unique id
    revision = Int(EXISTDB_INITIAL_VERSION, transient=True)
    uuid = String
    # Per-revision alias (for the humans!)
    alias = String(transient=True)

    def _uuid_default(self):
        """If a uuid isn't supplied, generate one
        """
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

    def reset_uuid_and_version(self):
        """Reset the ``uuid`` and ``revision``
        """
        self.revision = EXISTDB_INITIAL_VERSION
        self.uuid = str(uuid.uuid4())
