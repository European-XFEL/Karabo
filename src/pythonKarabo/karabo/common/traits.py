#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 13, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from traits.api import HasTraits, Instance, List


def walk_traits_object(
    traits_obj, visitor_func, pass_parent=False, fast_exit=False
):
    """Walk a Traits object by recursing into List(Instance(HasTraits))
    child traits.

    :param traits_obj: A HasTraits instance to walk
    :param visitor_func: A callable which will be called on each HasTraits
                         object in the tree rooted at `traits_obj`
    :param pass_parent: If True, pass the parent of each node to the visitor
                        along with the node itself.
    :param fast_exit: If True, return immediately if the visitor returns `True`
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
        return [
            name
            for name in obj.copyable_trait_names()
            if _is_list_of_has_traits(obj.trait(name))
        ]

    def _tree_iter(obj, parent=None):
        # Yield the root
        yield obj, parent
        # Then iteratively yield the children
        iterables = _find_iterables(obj)
        for name in iterables:
            children = getattr(obj, name)
            for child in children:
                for subchild, subparent in _tree_iter(child, parent=obj):
                    yield subchild, subparent

    for leaf, parent in _tree_iter(traits_obj):
        if pass_parent:
            success = visitor_func(leaf, parent)
        else:
            success = visitor_func(leaf)

        if success and fast_exit:
            break
