#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
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
