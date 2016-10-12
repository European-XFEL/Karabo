#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 12, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import Instance

from .bases import BaseProjectObjectModel


class MonitorModel(BaseProjectObjectModel):
    """ An object representing a monitor
    """
    config = Instance(object)
