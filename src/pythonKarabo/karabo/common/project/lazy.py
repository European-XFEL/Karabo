#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 20, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from io import BytesIO

from traits.api import HasTraits, Instance, List

from .bases import BaseProjectObjectModel


def read_lazy_object(uuid, revision, db_adapter, reader_func, existing=None):
    """Read a lazily-loaded object.

    :param uuid: The UUID of the object
    :param revision: The desired revision number of the object
    :param db_adapter: An object which gives access to data from the database
    :param reader_func: A callable which can generically read bytestreams and
                        generate project model objects
    :return: a non-lazy model object
    """
    collected = []

    def visitor(child):
        nonlocal collected
        if isinstance(child, BaseProjectObjectModel) and not child.initialized:
            collected.append(child)

    data = db_adapter.retrieve(uuid, revision)
    obj = reader_func(BytesIO(data), existing=existing)
    obj.initialized = True

    _walk_traits_object(obj, visitor)
    for child in collected:
        read_lazy_object(child.uuid, child.revision, db_adapter, reader_func,
                         existing=child)

    return obj

# ---------------------------------------------------------------------------


def _walk_traits_object(traits_obj, visitor_func):
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
