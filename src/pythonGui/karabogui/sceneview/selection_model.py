# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from qtpy.QtCore import QRect
from traits.api import HasStrictTraits, List

from .tools.api import is_resizable, is_selectable
from .utils import calc_bounding_rect


class SceneSelectionModel(HasStrictTraits):
    """A selection model for the SceneView.
    """
    # The list of selected scene objects (shapes, widgets, layouts)
    _selection = List

    def __iter__(self):
        """Implement the Python iterator interface.
        """
        return iter(self._selection)

    def __len__(self):
        return len(self._selection)

    def clear_selection(self):
        """Remove all objects from the selection
        """
        self._selection = []

    def deselect_object(self, obj):
        """Deselect an object.
        """
        self._selection.remove(obj)

    def get_selection_bounds(self):
        """Return the bounding rectangle for the objects which are selected.
        """
        return self.get_item_rect()

    def get_item_rect(self):
        """Return the item rectangle for the objects which are selected.

        This rect is used for position determination and not for drawing!
        """
        return QRect(*calc_bounding_rect(self._selection))

    def has_selection(self):
        return len(self._selection) > 0

    def has_multiple_selection(self):
        return len(self._selection) > 1

    def select_object(self, obj):
        """Select an object that can be handled by the scene tools"""
        if is_selectable(obj):
            self._selection.append(obj)

    def child_in_rect(self, rect):
        for child in self._selection:
            if child.geometry().intersects(rect):
                return child

    def child_in_pos(self, pos):
        for child in self._selection:
            if child.geometry().contains(pos):
                return child

    def get_single_selection(self):
        if len(self._selection) == 1:
            return self._selection[0]

    def has_resizable_selection(self):
        """Evaluates if the selection is resizable. The rule is:
           1. There is only one selection object in the scene
           2. That object can be resized."""
        gui_obj = self.get_single_selection()
        return gui_obj is not None and is_resizable(gui_obj)
