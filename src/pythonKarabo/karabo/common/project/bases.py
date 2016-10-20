#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import uuid

from traits.api import HasStrictTraits, Dict, Int, String


class BaseProjectObjectModel(HasStrictTraits):
    """ A base class for all things which can be serialized and sent to
    a Project server.
    """
    # A simple, human-readable name. Doesn't need to be unique
    simple_name = String

    # Version and unique id
    version = Int
    uuid = String

    # Database-provided attributes which need to be preserved
    db_attrs = Dict

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
