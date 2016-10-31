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
from .server import DeviceServerModel


class ProjectModel(BaseProjectObjectModel):
    """ An object representing a Karabo project.
    """
    # All the things that can be part of a project...
    devices = List(Instance(DeviceConfigurationModel))
    macros = List(Instance(MacroModel))
    scenes = List(Instance(SceneModel))
    servers = List(Instance(DeviceServerModel))

# This is outside the class because `ProjectModel` isn't available until here
ProjectModel.add_class_trait('subprojects', List(Instance(ProjectModel)))


def visit_project_objects(project, visitor_func):
    """ Recursively visit all objects in a project model tree using a pre-order
    traversal.

    :param project: A project model instance
    :param visitor_func: A callable which takes a single BaseProjectObjectModel
                         instance as its only argument.
    """
    def _find_iterables(obj):
        """ Find all traits which are lists of BaseProjectObjectModel instances
        """
        iterables = []
        for name, t in obj.traits().items():
            if isinstance(t.trait_type, List):
                inner_type = t.inner_traits[0].trait_type
                if (isinstance(inner_type, Instance) and
                        issubclass(inner_type.klass, BaseProjectObjectModel)):
                    iterables.append(name)
        return iterables

    def _tree_iter(obj):
        # Yield the root
        yield obj
        # Then iteratively yield the children
        iterables = _find_iterables(obj)
        for name in iterables:
            children = getattr(obj, name)
            for child in children:
                for subchild in _tree_iter(child):
                    yield subchild

    for model in _tree_iter(project):
        visitor_func(model)
