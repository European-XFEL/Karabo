#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 13, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import HasTraits, Instance, List


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
