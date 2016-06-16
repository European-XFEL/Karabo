from PyQt4.QtGui import QBoxLayout
from traits.api import Callable, Int

from karabo_gui.scenemodel.api import (
    BoxLayoutModel, FixedLayoutModel, GridLayoutChildData, GridLayoutModel)
from karabo_gui.sceneview.bases import BaseSceneAction
from karabo_gui.sceneview.utils import calc_bounding_rect


class CreateToolAction(BaseSceneAction):
    """ An action which simplifies the creation of actions which only change
    the currently active tool in the scene view.
    """
    # A factory for the tool to be added
    tool_factory = Callable

    def perform(self, scene_view):
        """ Perform the action on a SceneView instance.
        """
        tool = self.tool_factory()
        scene_view.set_tool(tool)


class GroupSceneAction(BaseSceneAction):
    """ Group without layout action"""

    def perform(self, scene_view):
        selection_model = scene_view.selection_model
        # It does not make sense to create a layout for less than 2 items
        if len(selection_model) < 2:
            return

        x, y, width, height = calc_bounding_rect(selection_model)
        layout_model = FixedLayoutModel(x=x, y=y, width=width, height=height)
        for obj in selection_model:
            scene_view.remove_model(obj.model)
            layout_model.children.append(obj.model)
        scene_view.add_model(layout_model)
        selection_model.clear_selection()


class BoxSceneAction(BaseSceneAction):
    direction = Int

    def perform(self, scene_view):
        """ Perform the box grouping. """
        selection_model = scene_view.selection_model
        # It does not make sense to create a layout for less than 2 items
        if len(selection_model) < 2:
            return

        model = BoxLayoutModel()
        model.direction = self.direction
        selection_rect = calc_bounding_rect(selection_model)
        model.x, model.y, model.width, model.height = selection_rect
        for obj in selection_model:
            scene_view.remove_model(obj.model)
            model.children.append(obj.model)
        scene_view.add_model(model)
        selection_model.clear_selection()


class BoxVSceneAction(BoxSceneAction):
    """ Group vertically action"""
    direction = QBoxLayout.TopToBottom


class BoxHSceneAction(BoxSceneAction):
    """ Group horizontally action"""
    direction = QBoxLayout.LeftToRight


class GridSceneAction(BaseSceneAction):
    """ Group in grid action"""

    def perform(self, scene_view):
        selection_model = scene_view.selection_model
        # It does not make sense to create a layout for less than 2 items
        if len(selection_model) < 2:
            return

        x, y, width, height = calc_bounding_rect(selection_model)
        layout_model = GridLayoutModel(x=x, y=y, width=width, height=height)
        for row, obj in enumerate(selection_model):
            model = obj.model
            scene_view.remove_model(model)

            model.layout_data = GridLayoutChildData(
                row=row, col=0, rowspan=1, colspan=1)
            layout_model.children.append(model)

        scene_view.add_model(layout_model)
        selection_model.clear_selection()


class UngroupSceneAction(BaseSceneAction):
    """ Ungroup action"""
    def perform(self, scene_view):
        """ Perform the ungrouping. """


class GroupEntireSceneAction(BaseSceneAction):
    """ Group entirely"""
    def perform(self, scene_view):
        """ Perform entire window grouping. """
