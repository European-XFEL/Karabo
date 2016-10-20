#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 20, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import Instance, List

from .bases import BaseProjectObjectModel


class ProjectObjectReference(BaseProjectObjectModel):
    """ A project object refence that can be transformed into a proper model
    object at a later point in time.
    """


class LazyDeviceGroupModel(BaseProjectObjectModel):
    """ A device group with object references which must be later resolved.
    """
    devices = List(Instance(ProjectObjectReference))


class LazyDeviceServerModel(BaseProjectObjectModel):
    """ A device server with object references which must be later resolved.
    """
    devices = List(Instance(ProjectObjectReference))


class LazyProjectModel(BaseProjectObjectModel):
    """ A project filled with object references which must be later resolved.
    """
    devices = List(Instance(ProjectObjectReference))
    macros = List(Instance(ProjectObjectReference))
    monitors = List(Instance(ProjectObjectReference))
    scenes = List(Instance(ProjectObjectReference))
    servers = List(Instance(ProjectObjectReference))
    subprojects = List(Instance(ProjectObjectReference))
