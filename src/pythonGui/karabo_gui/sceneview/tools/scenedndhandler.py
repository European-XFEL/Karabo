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
from karabo_gui.project.api import add_device_to_server
from karabo_gui.schema import ChoiceOfNodes
from karabo_gui.singletons.api import get_topology
from karabo_gui.widget import DisplayWidget, EditableWidget
from .const import WIDGET_FACTORIES

_STACKED_WIDGET_OFFSET = 30


def getDeviceBox(box):
    """Return a box that belongs to an active device

    if the box already is part of a running device, return it,
    if it is from a class in a project, return the corresponding
    instantiated device's box.
    """
    if box.configuration.type == "projectClass":
        return get_topology().get_device(box.configuration.id).getBox(box.path)
    return box


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
        sourceType = event.mimeData().data("sourceType")
        if sourceType == "ParameterTreeWidget":
            source = event.source()
            if (source is not None and not (source.conf.type == "class")
                    and not isinstance(source.currentItem().box.descriptor,
                                       ChoiceOfNodes)):
                return True
        return False

    def handle(self, scene_view, event):
        source = event.source()

        pos = event.pos()
        widget = scene_view.widget_at_position(pos)
        if widget is not None:
            boxes = [item.box for item in source.selectedItems()]
            if widget.add_boxes(boxes):
                event.accept()
                return

        models = []
        for item in source.selectedItems():
            model = self._create_model_from_parameter_item(item, pos)
            models.append(model)
            pos += QPoint(0, _STACKED_WIDGET_OFFSET)
        scene_view.add_models(*models)
        event.accept()

    def _create_model_from_parameter_item(self, item, pos):
        """ The given ``item`` which is a TreeWidgetItem is used to create
            the model for the view.
        """
        # Horizonal layout
        layout_model = BoxLayoutModel(direction=QBoxLayout.LeftToRight,
                                      x=pos.x(), y=pos.y())
        # Add label to layout model
        label_model = LabelModel(text=item.text(0), font=QFont().toString(),
                                 foreground='#000000')
        layout_model.children.append(label_model)

        # Get Boxes. "box" is in the project, "realbox" the
        # one on the device. They are the same if not from a project
        box = item.box
        realbox = getDeviceBox(box)
        if realbox.descriptor is not None:
            box = realbox

        def create_model(factory, box, parent_component, layout_model):
            klass = WIDGET_FACTORIES[factory.__name__]
            model = klass(keys=[box.key()],
                          parent_component=parent_component)
            if hasattr(model, 'klass'):
                model.klass = factory.__name__
            layout_model.children.append(model)

        # Add the display and editable components, as needed
        if item.displayComponent:
            factory = DisplayWidget.getClass(box)
            create_model(factory, box, 'DisplayComponent', layout_model)
        if item.editableComponent:
            factory = EditableWidget.getClass(box)
            create_model(factory, box, 'EditableApplyLaterComponent',
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
