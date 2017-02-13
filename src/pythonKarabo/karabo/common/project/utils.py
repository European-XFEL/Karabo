from traits.api import Instance, List

from karabo.common.api import walk_traits_object
from .bases import BaseProjectObjectModel
from .device import DeviceInstanceModel


def find_parent_object(model, ancestor_model, search_klass):
    """ Given a project child object and a project object which is the child's
    ancestor, find the immediate parent of the child.

    :param model: A project object instance
    :param ancestor_model: A project object model which is the ancestor of
                           ``model``.
    :param search_klass: The type of parent object to look for
    :return: A parent project object model or None
    """
    class _Visitor(object):
        last_object = None
        parent = None

        def __call__(self, obj):
            if obj is model:
                if self.parent is not None:
                    msg = "Object {} has more than one parent!"
                    raise RuntimeError(msg.format(obj))
                self.parent = self.last_object
            if isinstance(obj, search_klass):
                self.last_object = obj

    visitor = _Visitor()
    walk_traits_object(ancestor_model, visitor)
    return visitor.parent


def device_instance_exists(project, instance_ids):
    """Check whether the a ``project`` already has a device[s] with the given
    ``instance_ids`` and return ``True`` or ``False``
    """
    found = False

    # Allow one or more instance ids
    if isinstance(instance_ids, str):
        instance_ids = (instance_ids,)

    def visitor(obj):
        nonlocal found
        if isinstance(obj, DeviceInstanceModel):
            if obj.instance_id in instance_ids:
                found = True

    walk_traits_object(project, visitor)
    return found


def recursive_save_object(root, storage, domain):
    """Recursively save a project object by using a depth first traversal and
    saving all the modified ``BaseProjectObjectModel`` objects which are
    found in the object tree.
    """
    # XXX: Yes, this is duplicated code. It's basically yield parent first vs.
    # yield parent last. This should be generalized later in walk_traits_object
    def _is_list_of_has_traits(trait):
        if not isinstance(trait.trait_type, List):
            return False
        inner_type = trait.inner_traits[0].trait_type
        if not isinstance(inner_type, Instance):
            return False
        if not issubclass(inner_type.klass, BaseProjectObjectModel):
            return False
        return True

    def _find_iterables(obj):
        return [name for name in obj.copyable_trait_names()
                if _is_list_of_has_traits(obj.trait(name))]

    def _tree_iter(obj):
        # Iteratively yield the children
        iterables = _find_iterables(obj)
        for name in iterables:
            children = getattr(obj, name)
            for child in children:
                for subchild in _tree_iter(child):
                    yield subchild
        # Yield the root last
        yield obj

    for leaf in _tree_iter(root):
        if leaf.modified:
            storage.store(domain, leaf.uuid, leaf.revision, leaf)
