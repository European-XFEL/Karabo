from traits.api import HasTraits, Instance, List

from .model import ProjectModel


def find_parent_project(model, root_project):
    """ Given a project child object and a project model which is the child's
    ancestor, find the immediate parent of the child.

    :param model: A project object instance
    :param root_project: A ProjectModel which is the ancestor of ``model``.
    """
    class _Visitor(object):
        last_project = None
        parent = None

        def __call__(self, obj):
            if obj is model:
                if self.parent is not None:
                    msg = "Object {} is in the project more than once!"
                    raise RuntimeError(msg.format(obj))
                self.parent = self.last_project
            if isinstance(obj, ProjectModel):
                self.last_project = obj

    visitor = _Visitor()
    walk_traits_object(root_project, visitor)
    return visitor.parent


def find_parent_object(model, project):
    """ Given a project child object and a project model which is the child's
    ancestor, find the immediate parent of the child.

    :param model: A project object instance
    :param root_project: A ProjectModel which is the ancestor of ``model``.
    """
    class _Visitor(object):
        last_object = None
        parent = None

        def __call__(self, obj):
            if obj is model:
                if self.parent is not None:
                    msg = "Object {} is in the project more than once!"
                    raise RuntimeError(msg.format(obj))
                self.parent = self.last_object
            else:
                self.last_object = obj

    visitor = _Visitor()
    walk_traits_object(project, visitor)
    return visitor.parent


def walk_traits_object(traits_obj, visitor_func):
    """ Walk a Traits object by recursing into List(Instance(HasTraits))
    child traits.
    """
    def _is_list_of_has_traits(trait):
        if not isinstance(trait.trait_type, List):
            return False
        inner_type = trait.inner_traits[0].trait_type
        if not isinstance(inner_type, Instance):
            return False
        if not issubclass(inner_type.klass, HasTraits):
            return False
        return True

    def _find_iterables(obj):
        return [name for name in obj.copyable_trait_names()
                if _is_list_of_has_traits(obj.trait(name))]

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

    for leaf in _tree_iter(traits_obj):
        visitor_func(leaf)
