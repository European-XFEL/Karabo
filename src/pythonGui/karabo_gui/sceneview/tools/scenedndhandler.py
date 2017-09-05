#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 30, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from abc import abstractmethod
import json

from PyQt4.QtCore import QPoint
from PyQt4.QtGui import QBoxLayout, QFont
from traits.api import ABCHasStrictTraits

from karabo.common.scenemodel.api import (
    BoxLayoutModel, LabelModel, WorkflowItemModel
)
from karabo_gui.enums import NavigationItemTypes
from karabo_gui.project.utils import add_device_to_server
from karabo_gui.sceneview.widget.utils import get_box
from .const import WIDGET_FACTORIES

_STACKED_WIDGET_OFFSET = 30


class SceneDnDHandler(ABCHasStrictTraits):

    @abstractmethod
    def can_handle(self, event):
        """ Check whether the drag event can be handled. """

    @abstractmethod
    def handle(self, scene_view, event):
        """ Handle the drop event. """


class ConfigurationDropHandler(SceneDnDHandler):
    """Scene D&D handler for drops originating from the configuration view.
    """
    def can_handle(self, event):
        source_type = event.mimeData().data('source_type')
        if source_type == 'ParameterTreeWidget':
            return True
        return False

    def handle(self, scene_view, event):
        mime_data = event.mimeData()
        items = []
        if mime_data.data('tree_items'):
            items_data = mime_data.data('tree_items').data()
            items = json.loads(items_data.decode())

        # Handle the case when dropped on an existing scene widget
        pos = event.pos()
        widget = scene_view.widget_at_position(pos)
        if widget is not None:
            boxes = [get_box(*item['key'].split('.', 1)) for item in items]
            if widget.add_boxes(boxes):
                event.accept()
                return

        # Handle the case when dropped as new scene widgets
        models = []
        for item in items:
            models.append(self._create_model_from_parameter_item(item, pos))
            pos += QPoint(0, _STACKED_WIDGET_OFFSET)
        scene_view.add_models(*models)
        event.accept()

    def _create_model_from_parameter_item(self, item, pos):
        """Create the scene models for a single item
        """
        # Horizonal layout
        layout_model = BoxLayoutModel(direction=QBoxLayout.LeftToRight,
                                      x=pos.x(), y=pos.y())
        # Add label to layout model
        label_model = LabelModel(text=item['label'], font=QFont().toString(),
                                 foreground='#000000')
        layout_model.children.append(label_model)

        def create_model(klass_name, key, parent_component, layout_model):
            klass = WIDGET_FACTORIES[klass_name]
            model = klass(keys=[key], parent_component=parent_component)
            if hasattr(model, 'klass'):
                model.klass = klass_name
            layout_model.children.append(model)

        # Add the display and editable components, as needed
        key = item['key']
        if 'display_widget_class' in item:
            klass_name = item['display_widget_class']
            create_model(klass_name, key, 'DisplayComponent', layout_model)
        if 'edit_widget_class' in item:
            klass_name = item['edit_widget_class']
            create_model(klass_name, key, 'EditableApplyLaterComponent',
                         layout_model)

        return layout_model


class NavigationDropHandler(SceneDnDHandler):
    """Scene D&D handler for drops originating from the navigation view.
    """

    def can_handle(self, event):
        """We can handle a drop if it contains at least one device or class
        """
        return len(self._extract_items(event.mimeData())) > 0

    def handle(self, scene_view, event):
        dropped_items = self._extract_items(event.mimeData())
        if len(dropped_items) == 0:
            return

        # We ONLY handle one dropped class at a time!
        item = dropped_items[0]
        device_id = add_device_to_server(item['serverId'],
                                         class_id=item['classId'])
        # Adding a device to the project can fail
        if device_id != '':
            position = event.pos()
            model = WorkflowItemModel(device_id=device_id,
                                      klass='WorkflowItem',
                                      x=position.x(), y=position.y())
            scene_view.add_models(model)
            event.accept()

    def _extract_items(self, mime_data):
        known_types = (NavigationItemTypes.CLASS,)  # XXX: Devices. Soon.
        items_data = mime_data.data('treeItems').data()
        if items_data:
            items = json.loads(items_data.decode())
            return [it for it in items if it['type'] in known_types]
        return []
