#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 27, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import ABCHasStrictTraits, Instance

from karabo.common.project.api import BaseProjectObjectModel


class BaseProjectTreeItem(ABCHasStrictTraits):
    """ A base class for all objects which wrap ProjectModel objects for use
    in the ProjectTreeView.
    """
    # The project model object represented here
    model = Instance(BaseProjectObjectModel)
