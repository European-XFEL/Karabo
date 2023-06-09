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
from traits.api import HasStrictTraits, Instance, Int, List

from karabo.common.api import walk_traits_object

sum = 0


class Simple(HasStrictTraits):
    value = Int


class Walkable(HasStrictTraits):
    children_list = List(Instance(Simple))
    children_non_walkable = List(Instance(int))


def sum_visitor(item):
    global sum
    if isinstance(item, Simple):
        sum += item.value


def fast_exit_visitor(item):
    global sum
    if isinstance(item, Simple):
        sum += item.value
    if sum == 20:  # Stop when sum == 20!!!
        return True


def test_walk_traits_object():
    obj = Walkable(
        children_list=[Simple(value=10), Simple(value=10), Simple(value=10)],
        children_non_walkable=[70, 42, 90],
    )

    global sum
    sum = 0
    walk_traits_object(obj, sum_visitor)
    assert sum == 30

    sum = 0
    walk_traits_object(obj, fast_exit_visitor, fast_exit=True)
    assert sum == 20
