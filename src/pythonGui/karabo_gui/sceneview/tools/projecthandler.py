#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 20, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QFont
from traits.api import ABCHasStrictTraits, WeakRef

from karabo_gui.scenemodel.api import WorkflowItemModel


class ProjectSceneHandler(ABCHasStrictTraits):
    project = WeakRef('karabo_gui.guiproject.GuiProject')

    def device_dropped(self, device_id, server_id, class_id, startup_behaviour,
                       position):
        device = self._get_project_device_box(device_id)
        # Check, if device box already exists
        if device is None:
            # Create new device box in project
            device = self.project.newDevice(server_id, class_id, device_id,
                                            startup_behaviour)
        else:
            # Overwrite existing device box
            pass

        return self._create_device_model(device_id, server_id, class_id,
                                         startup_behaviour, position)

    def device_group_dropped(self, group_name, server_id, class_id,
                             startup_behaviour, display_prefix,
                             start_index, end_index, position):
        device_group = self._get_project_device_box(group_name)
        if device_group is None:
            # Create new device box in project
            device_group = self.project.newDeviceGroup(group_name,
                                                       server_id,
                                                       class_id,
                                                       startup_behaviour,
                                                       display_prefix,
                                                       start_index,
                                                       end_index)
        else:
            # Overwrite existing device group box
            pass

        return self._create_device_group_model(group_name,
                                               server_id,
                                               class_id,
                                               startup_behaviour,
                                               display_prefix,
                                               start_index,
                                               end_index,
                                               position)

    def _get_project_device_box(self, device_id):
        """ Checks whether a device configuration with the given ``device_id``
            already exists in the project and returns it.
        """
        return self.project.getDevice(device_id)

    def _create_device_model(self, device_id, server_id, class_id,
                             startup_behaviour, position):
        """ A new device box is created, added to the project and a workflow
            item model to show in the scene view is returned.
        """
        traits = {'device_id': device_id,
                  'font': QFont().toString(),
                  'klass': 'WorkflowItem',
                  'x': position.x(), 'y': position.y()}
        return WorkflowItemModel(**traits)

    def _create_device_group_model(self, group_name, server_id, class_id,
                                   startup_behaviour, display_prefix,
                                   start_index, end_index, position):
        traits = {'device_id': group_name,
                  'font': QFont().toString(),
                  'klass': 'WorkflowItem',
                  'x': position.x(), 'y': position.y()}
        return WorkflowItemModel(**traits)
