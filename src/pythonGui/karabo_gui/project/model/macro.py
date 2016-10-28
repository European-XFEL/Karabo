#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import Instance

from karabo.common.project.api import MacroModel
from .base import BaseProjectTreeItem


class MacroModelItem(BaseProjectTreeItem):
    """ A wrapper for MacroModel objects
    """
    # Redefine model with the correct type
    model = Instance(MacroModel)
