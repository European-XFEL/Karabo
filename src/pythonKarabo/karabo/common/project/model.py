#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import Instance, List

from karabo.common.scenemodel.api import SceneModel
from .bases import BaseProjectObjectModel
from .device import DeviceConfigurationModel
from .macro import MacroModel
from .monitor import MonitorModel
from .server import DeviceServerModel


class ProjectModel(BaseProjectObjectModel):
    """ An object representing a Karabo project.
    """
    # All the things that can be part of a project...
    devices = List(Instance(DeviceConfigurationModel))
    macros = List(Instance(MacroModel))
    monitors = List(Instance(MonitorModel))
    scenes = List(Instance(SceneModel))
    servers = List(Instance(DeviceServerModel))
    subprojects = List(Instance('ProjectModel'))
