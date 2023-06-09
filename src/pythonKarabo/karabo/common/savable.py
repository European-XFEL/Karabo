#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on November 30, 2016
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
from traits.api import (
    Bool, Dict, HasStrictTraits, Instance, List, TraitDictEvent,
    TraitListEvent)

_CONTAINER_EVENT_SUFFIX = "_items"
_CONTAINER_EVENT_TYPES = (TraitDictEvent, TraitListEvent)
_CONTAINER_TYPES = (Dict, List)


def set_trait_value(model, **traits):
    if isinstance(model, BaseSavableModel):
        model.trait_set(**traits)

        for name in model.copyable_trait_names():
            attribute = getattr(model, name, None)
            if isinstance(attribute, BaseSavableModel):
                attribute.trait_set(**traits)
            elif isinstance(attribute, list):
                for child in attribute:
                    if isinstance(child, BaseSavableModel):
                        set_trait_value(child, **traits)
            elif isinstance(attribute, dict):
                for child in attribute.values():
                    if isinstance(child, BaseSavableModel):
                        set_trait_value(child, **traits)


def set_modified_flag(model, value=False):
    """Set the ``modified`` trait to the given key word argument ``value`` in
    an object tree.

    This recurses into all child models and sets the modified flag to ``value``
    whenever it is found.

    :param model: A BaseSavableModel (or subclass) instance
    :param value: States whether the flag should be set to ``False`` or
                  ``True``
    """
    set_trait_value(model, modified=value)


def set_initialized_flag(model, value=False):
    """Set the ``initialized`` trait to the given key word argument ``value``
    in an object tree.

    This recurses into all child models and sets the initialized flag to
    ``value`` whenever it is found.

    :param model: A BaseSavableModel (or subclass) instance
    :param value: States whether the flag should be set to ``False`` or
                  ``True``
    """
    set_trait_value(model, initialized=value)


class BaseSavableModel(HasStrictTraits):
    """A base class for all things which can be serialized.

    The key purpose of this class is to handle modification tracking in data
    models. A model is modified whenever one of its direct non-transient,
    non-property traits is modified. A model is also modified if it has an
    Instance trait (or List/Dict of Instance) whose object(s) is/are also a
    BaseSavableModel and THAT object receives a modification.
    """

    # When True, the object contains unsaved data
    modified = Bool(False, transient=True)

    # When False, the object has not been completely loaded
    initialized = Bool(False, transient=True)

    def _anytrait_changed(self, name, old, new):
        """Listen for changes to all non-transient, non-property traits and
        mark the object as modified accordingly.
        """
        if not self.traits_inited():
            # Calls during initialization will attach listeners for initial
            # container items
            self._manage_container_item_listeners(name, old, new)
            return

        # Detect *_items changes
        if name.endswith(_CONTAINER_EVENT_SUFFIX) and isinstance(
            new, _CONTAINER_EVENT_TYPES
        ):
            # Change name to the actual trait name
            name = name[: -len(_CONTAINER_EVENT_SUFFIX)]

        # copyable_trait_names() returns all the trait names which contain
        # data which should be persisted (or copied when making a deep copy).
        if name in self.copyable_trait_names():
            self.modified = self.initialized  # Iff we're initialized!
            self._manage_container_item_listeners(name, old, new)

    def _child_modification(self, modified):
        """Called when a child BaseSavableModel is modified"""
        # Flip self.modified to True if modified is True, but don't reset
        # self.modified if modified is False.
        self.modified = self.modified or modified

    def _manage_container_item_listeners(self, name, old, new):
        """When a container trait has Instance items inside, we want to be
        notified when those instances are modified. To that end, a notification
        handler needs to be attached to every child.
        """
        # Bail out quickly
        trait = self.trait(name)
        is_savable_instance = _is_savable_instance(trait)
        if not (_is_list_of_child_objs(trait) or is_savable_instance):
            return

        if isinstance(new, TraitListEvent):
            added = new.added
            removed = new.removed
        elif isinstance(new, TraitDictEvent):
            added = new.added.values()
            removed = new.removed.values()
        elif is_savable_instance:
            added = [new] if new is not None else []
            removed = [old] if old is not None else []
        elif isinstance(new, dict) or isinstance(old, dict):
            added = new.values() if new is not None else []
            removed = old.values() if old is not None else []
        else:  # list
            added = new or []
            removed = old or []

        for child in removed:
            child.on_trait_change(
                self._child_modification, "modified", remove=True
            )
        for child in added:
            child.on_trait_change(self._child_modification, "modified")


def _is_savable_instance(trait):
    """Is some trait object an Instance(BaseSavableModel) trait?"""
    if not isinstance(trait.trait_type, Instance):
        return False
    inner_type = trait.trait_type.klass
    if not issubclass(inner_type, BaseSavableModel):
        return False
    return True


def _is_list_of_child_objs(trait):
    """Is some trait object an List(Instance(BaseSavableModel))
    or Dict(<keytype>, Instance(BaseSavableModel)) trait?
    """
    if not isinstance(trait.trait_type, _CONTAINER_TYPES):
        return False
    # `-1` handles the value type of a Dict trait (and List too)
    inner_type = trait.inner_traits[-1].trait_type
    if not isinstance(inner_type, Instance):
        return False
    if not issubclass(inner_type.klass, BaseSavableModel):
        return False
    return True
