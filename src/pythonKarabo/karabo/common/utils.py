#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 13, 2017
# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
import math
import weakref
from inspect import signature
from types import MethodType

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
                yield from _tree_iter(child, parent=obj)

    for leaf, parent in _tree_iter(traits_obj):
        if pass_parent:
            success = visitor_func(leaf, parent)
        else:
            success = visitor_func(leaf)

        if success and fast_exit:
            break


class WeakMethodRef:
    """A weakref.ref() for bound methods"""

    def __init__(self, bound_method, num_args=-1):
        # Preconditions...
        # bound_method MUST be a bound method
        assert type(bound_method) is MethodType
        if num_args > -1:
            # bound_method MUST take N args (- 1 because of self)!
            needed_args = bound_method.__func__.__code__.co_argcount - 1
            assert needed_args == num_args

        obj = bound_method.__self__
        if obj is not None:
            self.obj = weakref.ref(obj, self._owner_deleted)
            self.name = bound_method.__name__

        sig = signature(bound_method.__func__)
        # Replace the signature!
        self.__call__.__func__.__signature__ = sig

    def __call__(self, *args, **kwargs):
        if self.obj is not None:
            obj = self.obj()
            if obj is not None:
                method = getattr(obj, self.name)
                method(*args, **kwargs)

    def _owner_deleted(self, ref):
        self.obj = None


def get_arrowhead_points(x1: int, y1: int, x2: int, y2: int) -> (
        tuple)[int, int, int, int]:
    """Calculate the two points for the arrow head for the given lines"""
    arrow_size = 10
    angle_offset = math.pi / 10
    angle = math.atan2(y2 - y1, x2 - x1)

    # The points for the arrowhead triangle
    hx1 = round(x2 - arrow_size * math.cos(angle - angle_offset))
    hy1 = round(y2 - arrow_size * math.sin(angle - angle_offset))

    hx2 = round(x2 - arrow_size * math.cos(angle + angle_offset))
    hy2 = round(y2 - arrow_size * math.sin(angle + angle_offset))

    return hx1, hy1, hx2, hy2
