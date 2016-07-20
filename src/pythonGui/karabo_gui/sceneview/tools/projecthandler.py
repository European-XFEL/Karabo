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
            return self._create_project_device_box(device_id,
                                                   server_id,
                                                   class_id,
                                                   startup_behaviour,
                                                   position)
        else:
            # Overwrite existing
            pass

    def _get_project_device_box(self, device_id):
        """ Check whether a device configuration with the given ``device_id``
            already exists in the project and return it.
        """
        return self.project.getDevice(device_id)

    def _create_project_device_box(self, device_id, server_id, class_id,
                                   startup_behaviour, position):
        """ A new device box is created and add to the project.
        """
        self.project.newDevice(server_id, class_id, device_id,
                               startup_behaviour)
        traits = {'device_id': device_id,
                  'font': QFont().toString(),
                  'klass': 'WorkflowItem',
                  'x': position.x(), 'y': position.y()}
        return WorkflowItemModel(**traits)
