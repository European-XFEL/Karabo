from abc import abstractmethod

from PyQt4.QtGui import QBoxLayout
from traits.api import Callable, Int

from karabo_gui.scenemodel.api import (
    BaseLayoutModel, BoxLayoutModel, FixedLayoutModel,
    GridLayoutChildData, GridLayoutModel)
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


class BaseLayoutAction(BaseSceneAction):
    """ Base class for layout actions.

    This contains the boilerplate that all layout actions have in common.
    """

    @abstractmethod
    def create_layout(self, scene_view, models, selection_rect):
        """ Implemented by derived classes to create and add a layout.
        """

    def perform(self, scene_view):
        selection_model = scene_view.selection_model
        # It does not make sense to create a layout for less than 2 items
        if len(selection_model) < 2:
            return

        child_models = []
        for obj in selection_model:
            scene_view.remove_model(obj.model)
            child_models.append(obj.model)

        selection_rect = calc_bounding_rect(selection_model)
        selection_model.clear_selection()

        self.create_layout(scene_view, child_models, selection_rect)


class BoxSceneAction(BaseLayoutAction):
    """ A base class for actions which create a box layout
    """
    # What's the layout direction?
    direction = Int

    def create_layout(self, scene_view, models, selection_rect):
        x, y, width, height = selection_rect
        model = BoxLayoutModel(x=x, y=y, width=width, height=height,
                               children=models, direction=self.direction)
        scene_view.add_models(model)


class BoxVSceneAction(BoxSceneAction):
    """ Group vertically action
    """
    direction = QBoxLayout.TopToBottom


class BoxHSceneAction(BoxSceneAction):
    """ Group horizontally action
    """
    direction = QBoxLayout.LeftToRight


class GridSceneAction(BaseLayoutAction):
    """ Group in grid action
    """

    def create_layout(self, scene_view, models, selection_rect):
        x, y, width, height = selection_rect
        layout_model = GridLayoutModel(x=x, y=y, width=width, height=height)
        for row, model in enumerate(models):
            model.layout_data = GridLayoutChildData(
                row=row, col=0, rowspan=1, colspan=1)
            layout_model.children.append(model)
        scene_view.add_models(layout_model)


class GroupSceneAction(BaseLayoutAction):
    """ Group without layout action
    """

    def create_layout(self, scene_view, models, selection_rect):
        x, y, width, height = selection_rect
        model = FixedLayoutModel(x=x, y=y, width=width, height=height,
                                 children=models)
        scene_view.add_models(model)


class GroupEntireSceneAction(BaseSceneAction):
    """ Group entirely"""
    def perform(self, scene_view):
        """ Perform entire window grouping. """


class UngroupSceneAction(BaseSceneAction):
    """ Ungroup action
    """
    def perform(self, scene_view):
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
