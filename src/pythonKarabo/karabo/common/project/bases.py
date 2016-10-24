#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import uuid

from traits.api import HasStrictTraits, Bool, Dict, Int, String


class BaseProjectObjectModel(HasStrictTraits):
    """ A base class for all things which can be serialized and sent to
    a Project server.
    """
    # A simple, human-readable name. Doesn't need to be unique
    simple_name = String

    # When True, the object contains unsaved data
    modified = Bool(False, transient=True)

    # Version and unique id
    version = Int(transient=True)
    uuid = String

    # Database-provided attributes which need to be preserved
    db_attrs = Dict(transient=True)

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

    def _anytrait_changed(self, name, old, new):
        """ Listen for changes to all non-transient, non-property traits and
        mark the object as modified accordingly.
        """
        if not self.traits_inited():
            return

        # copyable_trait_names() returns all the trait names which contain
        # data which should be persisted (or copied when making a deep copy).
        if name in self.copyable_trait_names():
            self.modified = True
