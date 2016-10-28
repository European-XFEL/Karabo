#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import Instance

from karabo.common.scenemodel.api import SceneModel
from .base import BaseProjectTreeItem


class SceneModelItem(BaseProjectTreeItem):
    """ A wrapper for SceneModel objects
    """
    # Redefine model with the correct type
    model = Instance(SceneModel)
