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
from traits.api import Dict, HasStrictTraits, HasTraits, Instance, List

from karabo.common.api import BaseSavableModel, set_modified_flag


class Plain(HasStrictTraits):
    plain_child = Instance(object)


class Simple(BaseSavableModel):
    simple_child = Instance(object)


class Complex(BaseSavableModel):
    children_dict = Dict(str, Instance(BaseSavableModel))
    children_list = List(Instance(BaseSavableModel))
    children_non_savable = List(Instance(HasTraits))
    children_pod = List
    sav_child = Instance(BaseSavableModel)


class MoreComplex(BaseSavableModel):
    nested = List(Instance(Complex))


def test_simple_child_modification():
    s = Simple(simple_child=[1], initialized=True)

    assert not s.modified
    s.simple_child = {}
    assert s.modified


def test_savable_children():
    d_child = Simple(simple_child=0, initialized=True)
    l_child = Simple(simple_child=1, initialized=True)
    ns_child = Plain(plain_child=2)
    child = Simple(simple_child=3, initialized=True)
    complicated = Complex(
        children_dict={"foo": d_child},
        children_list=[l_child],
        children_non_savable=[ns_child],
        children_pod=["a string"],
        sav_child=child,
        initialized=True,
    )
    parent = MoreComplex(nested=[complicated], initialized=True)

    assert not complicated.modified
    assert not parent.modified

    complicated.children_non_savable[0].plain_child = 42
    assert not complicated.modified

    complicated.children_dict["foo"].simple_child = 42
    assert complicated.modified
    set_modified_flag(parent)

    complicated.children_list[0].simple_child = 42
    assert complicated.modified
    set_modified_flag(complicated)

    complicated.children_pod[0] = 42
    assert complicated.modified
    set_modified_flag(complicated)

    complicated.sav_child.simple_child = 42
    assert complicated.modified
    set_modified_flag(complicated)

    complicated.sav_child = Simple(simple_child=4, initialized=True)
    assert complicated.modified
    set_modified_flag(complicated)

    complicated.sav_child.simple_child = 42
    assert complicated.modified
    set_modified_flag(parent)

    parent.nested[0].children_list[0].simple_child = 1000
    assert parent.modified and complicated.modified
    set_modified_flag(parent)

    parent.nested[0].children_list[0] = Simple(
        simple_child=0, initialized=True
    )
    assert parent.modified and complicated.modified
    set_modified_flag(parent)

    parent.nested[0].children_list[0].simple_child = 42
    assert parent.modified and complicated.modified
    set_modified_flag(parent)


def test_notification_management():
    child = Simple(simple_child=0, initialized=True)
    parent = Complex(initialized=True)

    assert not parent.modified

    parent.children_list.append(child)
    set_modified_flag(parent)
    child.simple_child = 42
    assert parent.modified
    set_modified_flag(parent)

    # After removing a child, it should not affect the old parent
    parent.children_list = []
    set_modified_flag(parent)
    child.simple_child = 0
    assert not parent.modified

    parent.children_dict["key"] = child
    set_modified_flag(parent)
    child.simple_child = 42
    assert parent.modified

    # After removing a child, it should not affect the old parent
    set_modified_flag(parent)
    del parent.children_dict["key"]
    set_modified_flag(parent)
    child.simple_child = 0
    assert child.modified and not parent.modified
