#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 30, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from abc import abstractmethod

from PyQt4.QtCore import QPoint
from PyQt4.QtGui import QApplication, QBoxLayout, QDialog, QFont
from traits.api import ABCHasStrictTraits, String

from karabo_gui.enums import NavigationItemTypes
from karabo_gui.dialogs.devicedialogs import DeviceGroupDialog
from karabo_gui.scenemodel.api import BoxLayoutModel, LabelModel
from karabo_gui.schema import ChoiceOfNodes
from karabo_gui.topology import getDeviceBox, Manager
from karabo_gui.widget import DisplayWidget, EditableWidget
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
        mimeData = event.mimeData()
        sourceType = mimeData.data("sourceType")
        source = event.source()

        pos = event.pos()
        widget = scene_view.widget_at_position(pos)
        if widget is not None:
            boxes = [item.box for item in source.selectedItems()]
            if widget.add_boxes(boxes):
                event.accept()
                return

        if sourceType == "ParameterTreeWidget":
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

        # Add the display and editable components, as needed
        if item.displayComponent:
            factory = DisplayWidget.getClass(box)
            klass = WIDGET_FACTORIES[factory.__name__]
            model = klass(keys=[box.key()],
                          parent_component='DisplayComponent')
            layout_model.children.append(model)
        if item.editableComponent:
            factory = EditableWidget.getClass(box)
            klass = WIDGET_FACTORIES[factory.__name__]
            model = klass(keys=[box.key()],
                          parent_component='EditableApplyLaterComponent')
            layout_model.children.append(model)

        return layout_model


class NavigationDropHandler(SceneDnDHandler):
    server_id = String
    class_id = String

    def can_handle(self, event):
        """ Check whether the drag event can be handled. """
        sourceType = event.mimeData().data("sourceType")
        if sourceType == "NavigationTreeView":
            source = event.source()
            index_info = source.indexInfo()
            type = index_info.get("type")
            if type == NavigationItemTypes.CLASS:
                self.server_id = index_info.get("serverId")
                self.class_id = index_info.get("classId")
                return True
        return False

    def handle(self, scene_view, event):
        """ Handle the drop event. """
        # Restore cursor for dialog input
        QApplication.restoreOverrideCursor()
        # Open dialog to set up new device (group)
        dialog = DeviceGroupDialog(Manager().systemHash)
        # Set server and class id
        dialog.serverId = self.server_id
        dialog.classId = self.class_id

        if dialog.exec_() == QDialog.Rejected:
            return

        device_id = dialog.deviceId
        server_id = dialog.serverId
        class_id = dialog.classId
        startup_behaviour = dialog.startupBehaviour
        position = event.pos()
        if not dialog.deviceGroup:
            # Device
            scene_view.device_dropped(device_id, server_id, class_id,
                                      startup_behaviour, position)
        else:
            # Device Group
            scene_view.device_group_dropped(device_id, server_id, class_id,
                                            startup_behaviour, position)
