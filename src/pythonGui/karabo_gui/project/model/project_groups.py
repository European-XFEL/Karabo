#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import Callable, Dict, Instance, List, String

from karabo.common.project.api import ProjectModel
from .base import BaseProjectTreeItem


class ProjectSubgroupItem(BaseProjectTreeItem):
    """ A wrapper for ProjectModel subgroups (devices, scenes, macros, etc.)
    """
    # Redefine model with the correct type
    model = Instance(ProjectModel)

    # The name of the group shown in the GUI
    group_name = String

    # The name of the trait on ``model`` which is shadowed by ``children``
    trait_name = String

    # A factory for shadow items wrapping children
    child_create = Callable

    # A callable which can gracefully destroy a child shadow object
    child_destroy = Callable

    # The child tree items
    children = List(Instance(BaseProjectTreeItem))
    _child_map = Dict

    def item_handler(self, event):
        """ Called for List-trait events on ``model`` (a ProjectModel)
        """
        for model in event.removed:
            item_model = self._child_map[model]
            self.children.remove(item_model)
            self.child_destroy(item_model)

        additions = [self.child_factory(model=model) for model in event.added]
        self.children.extend(additions)

    def _children_items_changed(self, event):
        """ Maintain ``_child_map`` by watching item events on ``children``
        """
        for item_model in event.removed:
            del self._child_map[item_model.model]

        for item_model in event.added:
            self._child_map[item_model.model] = item_model
