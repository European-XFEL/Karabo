#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from traits.api import Bool, Instance, List

from karabo.common.scenemodel.api import SceneModel

from .bases import BaseProjectObjectModel
from .macro import MacroModel
from .server import DeviceServerModel


class ProjectModel(BaseProjectObjectModel):
    """An object representing a Karabo project."""

    # All the things that can be part of a project...
    macros = List(Instance(MacroModel))
    scenes = List(Instance(SceneModel))
    servers = List(Instance(DeviceServerModel))

    # Flag which can be set to enable the user to delete a project
    is_trashed = Bool(False)


# This is outside the class because `ProjectModel` isn't available until here
ProjectModel.add_class_trait("subprojects", List(Instance(ProjectModel)))
