from traits.api import Instance, List

from karabo.common.api import walk_traits_object
from .bases import BaseProjectObjectModel
from .device import DeviceInstanceModel


def find_parent_object(model, ancestor_model, search_klass):
    """Given a project child object and a project object which is the child's
    ancestor, find the immediate parent of the child.

    :param model: A project object instance
    :param ancestor_model: A project object model which is the ancestor of
                           ``model``.
    :param search_klass: The type of parent object to look for. This can be a
                         class, type, or tuple as it is passed to `isinstance`.
    :return: A parent project object model or None
    """

    class _Visitor(object):
        found_parent = None
        parent_candidate = None

        def __call__(self, obj, parent):
            if isinstance(parent, search_klass):
                self.parent_candidate = parent
            if obj is model:
                if self.found_parent is not None:
                    msg = "Object {} has more than one parent!"
                    raise RuntimeError(msg.format(obj))
                self.found_parent = self.parent_candidate

    visitor = _Visitor()
    walk_traits_object(ancestor_model, visitor, pass_parent=True)
    return visitor.found_parent


def get_children_of_klass(model, search_klass):
    """Provides all the desired children of ``search_klass``
    """
    ret = []

    class _Visitor(object):
        nonlocal ret

        def __call__(self, obj):
            if isinstance(obj, search_klass):
                ret.append(obj)

    visitor = _Visitor()
    walk_traits_object(model, visitor)

    return ret


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


def device_config_exists(project, instance_id, config_names):
    """Check whether the a ``project`` already has a device[s] configuration
    with the given ``config_names`` and return ``True`` or ``False``
    """
    found = False

    # Allow one or more device configuration names
    if isinstance(config_names, str):
        config_names = (config_names,)

    def visitor(obj):
        nonlocal found
        if (isinstance(obj, DeviceInstanceModel)
                and obj.instance_id == instance_id):
            existing = set(conf.simple_name for conf in obj.configs)
            if not existing.isdisjoint(config_names):
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
            storage.store(domain, leaf.uuid, leaf)
