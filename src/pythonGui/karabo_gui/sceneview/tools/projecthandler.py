#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 20, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QFont
from traits.api import ABCHasStrictTraits, WeakRef

from karabo_gui.scenemodel.api import WorkflowItemModel


class ProjectSceneHandler(ABCHasStrictTraits):
    project = WeakRef('karabo_gui.guiproject.GuiProject', allow_none=True)

    def get_scene_names(self):
        return self.project.getSceneNames()

    def create_device(self, device_id, server_id, class_id, ifexists,
                      position):
        # Check, if device box already exists
        if self.project.getDevice(device_id) is None:
            # Create new device box in project
            self.project.newDevice(server_id, class_id, device_id, ifexists)
        else:
            # TODO: Overwrite existing device box
            pass

        return self._create_model(device_id, 'WorkflowItem', position)

    def create_device_group(self, group_name, server_id, class_id, ifexists,
                            display_prefix, start_index, end_index, position):
        if self.project.getDevice(group_name) is None:
            # Create new device box in project
            self.project.newDeviceGroup(group_name, server_id, class_id,
                                        ifexists, display_prefix, start_index,
                                        end_index)
        else:
            # TODO: Overwrite existing device group box
            pass

        return self._create_model(group_name, 'WorkflowGroupItem', position)

    def _create_model(self, device_id, klass, position):
        traits = {'device_id': device_id,
                  'font': QFont().toString(),
                  'klass': klass,
                  'x': position.x(), 'y': position.y()}
        return WorkflowItemModel(**traits)
