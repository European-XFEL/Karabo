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
from bisect import bisect

from qtpy.QtWidgets import QBoxLayout
from traits.api import Callable, Int

from karabo.common.scenemodel.api import (
    BaseLayoutModel, BoxLayoutModel, FixedLayoutModel, GridLayoutChildData,
    GridLayoutModel)
from karabogui.sceneview.bases import BaseSceneAction
from karabogui.sceneview.utils import calc_bounding_rect


class CreateToolAction(BaseSceneAction):
    """An action which simplifies the creation of actions which only change
    the currently active tool in the scene view.
    """
    # A factory for the tool to be added
    tool_factory = Callable

    def perform(self, scene_view):
        """ Perform the action on a SceneView instance.
        """
        tool = self.tool_factory()
        scene_view.set_tool(tool)
        scene_view.set_cursor('cross')


class BaseLayoutAction(BaseSceneAction):
    """Base class for layout actions.

    This contains the boilerplate that all layout actions have in common.
    """

    @abstractmethod
    def create_layout(self, gui_objects, models, selection_rect):
        """Implemented by derived classes to create and add a layout.
        """

    def keyfunc(self, gui_obj):
        """A function to be passed as the key= argument of the sort() fuction.
        when sorting the GUI objects before passing their models to
        `create_layout`.

        THIS METHOD IS OPTIONAL.
        """
        raise NotImplementedError

    def perform(self, scene_view):
        selection_model = scene_view.selection_model
        # It does not make sense to create a layout for less than 2 items
        if len(selection_model) < 2:
            return

        # Get child objects from selection model
        child_objects = list(selection_model)
        try:
            child_objects.sort(key=self.keyfunc)
        except NotImplementedError:
            pass

        child_models = [obj.model for obj in child_objects]
        # Get the new layout model
        selection_rect = calc_bounding_rect(selection_model)
        layout_model = self.create_layout(child_objects, child_models,
                                          selection_rect)

        # Apply changes by removing the previous child models and add the
        # models after.
        for model in child_models:
            scene_view.remove_model(model)
        selection_model.clear_selection()

        scene_view.add_models(layout_model)
        scene_view.select_model(layout_model)


class BoxSceneAction(BaseLayoutAction):
    """A base class for actions which create a box layout
    """
    # What's the layout direction?
    direction = Int

    def create_layout(self, gui_objects, models, selection_rect):
        x, y, *_ = selection_rect
        width, height = self._calculate_size(gui_objects)
        layout_model = BoxLayoutModel(x=x, y=y, width=width, height=height,
                                      children=models,
                                      direction=self.direction)
        return layout_model

    def _calculate_size(self, gui_objects):
        # Collate widths and heights
        widths, heights = [], []
        for obj in gui_objects:
            size = obj.sizeHint()
            width, height = size.width(), size.height()
            widths.append(width)
            heights.append(height)

        # Calculate effective size by using sum and max on the widths and
        # heights depending on the direction
        get_width, get_height = sum, max
        if self.direction == QBoxLayout.TopToBottom:
            get_width, get_height = get_height, get_width

        return get_width(widths), get_height(heights)


class BoxHSceneAction(BoxSceneAction):
    """Group horizontally action
    """
    direction = QBoxLayout.LeftToRight

    def keyfunc(self, gui_obj):
        # Order by x first, then use y for tiebreaker
        geometry = gui_obj.geometry()
        return geometry.x(), geometry.y()


class BoxVSceneAction(BoxSceneAction):
    """Group vertically action
    """
    direction = QBoxLayout.TopToBottom

    def keyfunc(self, gui_obj):
        # Order by y first, then use x for tiebreaker
        geometry = gui_obj.geometry()
        return geometry.y(), geometry.x()


class GridSceneAction(BaseLayoutAction):
    """Group in grid action
    """

    def create_layout(self, gui_objects, models, selection_rect):
        x, y, width, height = selection_rect
        layout_model = GridLayoutModel(x=x, y=y, width=width, height=height)
        layout_model.children = self._collect_models(gui_objects)
        return layout_model

    @staticmethod
    def _collect_models(gui_objects):
        """Get a list of scene object models with the correct layout data
        attached.
        """

        def _reduce(positions):
            positions.sort()
            i = 1
            while i < len(positions):
                if positions[i] - positions[i - 1] > 10:
                    i += 1
                else:
                    del positions[i]
            return positions

        xs = _reduce([o.geometry().x() for o in gui_objects])
        ys = _reduce([o.geometry().y() for o in gui_objects])

        models = []
        for obj in gui_objects:
            rect = obj.geometry()
            col = bisect(xs, rect.x())
            row = bisect(ys, rect.y())
            colspan = bisect(xs, rect.right()) - col + 1
            rowspan = bisect(ys, rect.bottom()) - row + 1

            model = obj.model
            model.layout_data = GridLayoutChildData(row=row, col=col,
                                                    rowspan=rowspan,
                                                    colspan=colspan)
            models.append(model)
        return models


class GroupSceneAction(BaseLayoutAction):
    """Group without layout action
    """

    def create_layout(self, gui_objects, models, selection_rect):
        x, y, width, height = selection_rect
        model = FixedLayoutModel(x=x, y=y, width=width, height=height,
                                 children=models)
        return model


class GroupEntireSceneAction(BaseSceneAction):
    """Group entirely"""

    def perform(self, scene_view):
        """Perform entire window grouping. """


class UngroupSceneAction(BaseSceneAction):
    """Ungroup action
    """

    def perform(self, scene_view):
        ungroup(scene_view)


class SceneBringToFrontAction(BaseSceneAction):
    """Bring to front action"""

    def perform(self, scene_view):
        send_to_front(scene_view)


class SceneSendToBackAction(BaseSceneAction):
    """Send to back action"""

    def perform(self, scene_view):
        send_to_back(scene_view)


# ----------------------------------------------------------------------------
# scene_view actions.

def send_to_front(scene_view):
    """ Sends the selection_model models to the front and updates
        the scene_view """
    selection_model = scene_view.selection_model
    if len(selection_model) == 0:
        return

    for o in selection_model:
        scene_view.bring_to_front(o.model)
    scene_view.update()


def send_to_back(scene_view):
    """ Sends the selection_model models to the back and updates
        the scene_view """
    selection_model = scene_view.selection_model
    if len(selection_model) == 0:
        return
    for o in selection_model:
        scene_view.send_to_back(o.model)
    scene_view.update()


def ungroup(scene_view):
    """ Ungroups the selection_model models and add the ungrouped
        models to the view """
    selection_model = scene_view.selection_model
    unparented_models = []
    for obj in selection_model:
        model = obj.model
        if not isinstance(model, BaseLayoutModel):
            continue

        scene_view.remove_model(model)
        unparented_models.extend(model.children)

    # Clear the layout_data for all the now unparented models
    for child in unparented_models:
        child.layout_data = None

    scene_view.add_models(*unparented_models)
    selection_model.clear_selection()
