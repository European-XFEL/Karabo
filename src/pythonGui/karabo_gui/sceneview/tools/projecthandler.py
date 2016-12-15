#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 20, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QFont
from traits.api import ABCHasStrictTraits, WeakRef

from karabo.common.project.api import ProjectModel
from karabo.common.scenemodel.api import WorkflowItemModel


class ProjectSceneHandler(ABCHasStrictTraits):
    project = WeakRef(ProjectModel, allow_none=True)

    def get_scene_names(self):
        return [s.uuid for s in self.project.scenes]

    def create_device(self, device_id, server_id, class_id, ifexists,
                      position):
        return self._create_model(device_id, 'WorkflowItem', position)

    def create_device_group(self, group_name, server_id, class_id, ifexists,
                            display_prefix, start_index, end_index, position):
        return self._create_model(group_name, 'WorkflowGroupItem', position)

    def _create_model(self, device_id, klass, position):
        traits = {'device_id': device_id,
                  'font': QFont().toString(),
                  'klass': klass,
                  'x': position.x(), 'y': position.y()}
        return WorkflowItemModel(**traits)
