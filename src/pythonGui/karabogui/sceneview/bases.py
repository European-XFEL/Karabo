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
from abc import abstractmethod

from qtpy.QtGui import QIcon, QKeySequence
from traits.api import ABCHasStrictTraits, Bool, Either, Instance, Int, String


class BaseSceneAction(ABCHasStrictTraits):
    """Base class for actions in a SceneView
    """
    # The icon for this action
    icon = Instance(QIcon)
    # The text label for the action
    text = String
    # The tooltip text shown when hovering over the action
    tooltip = String
    # A keyboard shortcut for the action (QKeySequence::StandardKey value)
    # or a QKeySequence
    shortcut = Either(Int, Instance(QKeySequence))
    # Whether or not this action is checkable
    checkable = Bool(False)

    @abstractmethod
    def perform(self, scene_view):
        """Perform the action on a SceneView instance.
        """

    def _shortcut_default(self):
        """The default value for the shortcut"""
        return QKeySequence.UnknownKey


class BaseSceneTool(ABCHasStrictTraits):
    """Base class for tools which within a SceneView.
    """
    # If True, this tool will be drawn whenever the scene is drawn
    visible = Bool(False)

    def draw(self, scene_view, painter):
        """The method which is responsible for drawing this tool.
        The tool for a SceneView will be drawn after everything else in
        the view has been drawn.

        This method is optional.
        """
        raise NotImplementedError

    @abstractmethod
    def mouse_down(self, scene_view, event):
        """A callback which is fired whenever the user clicks in the
        SceneView.
        """

    @abstractmethod
    def mouse_move(self, scene_view, event):
        """A callback which is fired whenever the user moves the mouse
        in the SceneView.
        """

    @abstractmethod
    def mouse_up(self, scene_view, event):
        """A callback which is fired whenever the user ends a mouse click
        in the SceneView.
        """
