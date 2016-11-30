from traits.api import HasTraits, HasStrictTraits, Dict, Instance, List

from karabo.common.savable import BaseSavableModel


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


def _clear_modification_flag(root):
    if hasattr(root, 'modified'):
        root.modified = False
    for name in root.copyable_trait_names():
        attr = getattr(root, name)
        if isinstance(attr, list):
            for child in attr:
                if hasattr(child, 'copyable_trait_names'):
                    _clear_modification_flag(child)
        elif isinstance(attr, dict):
            for child in attr.values():
                if hasattr(child, 'copyable_trait_names'):
                    _clear_modification_flag(child)
        if hasattr(attr, 'modified'):
            attr.modified = False


def test_simple_child_modification():
    s = Simple(simple_child=[1])

    assert not s.modified
    s.simple_child = {}
    assert s.modified


def test_savable_children():
    d_child = Simple(simple_child=0)
    l_child = Simple(simple_child=1)
    ns_child = Plain(plain_child=2)
    child = Simple(simple_child=3)
    complicated = Complex(children_dict={'foo': d_child},
                          children_list=[l_child],
                          children_non_savable=[ns_child],
                          children_pod=['a string'],
                          sav_child=child)
    parent = MoreComplex(nested=[complicated])

    assert not complicated.modified
    assert not parent.modified

    complicated.children_non_savable[0].plain_child = 42
    assert not complicated.modified

    complicated.children_dict['foo'].simple_child = 42
    assert complicated.modified
    _clear_modification_flag(parent)

    complicated.children_list[0].simple_child = 42
    assert complicated.modified
    _clear_modification_flag(complicated)

    complicated.children_pod[0] = 42
    assert complicated.modified
    _clear_modification_flag(complicated)

    complicated.sav_child.simple_child = 42
    assert complicated.modified
    _clear_modification_flag(complicated)

    complicated.sav_child = Simple(simple_child=4)
    assert complicated.modified
    _clear_modification_flag(complicated)

    complicated.sav_child.simple_child = 42
    assert complicated.modified
    _clear_modification_flag(parent)

    parent.nested[0].children_list[0].simple_child = 1000
    assert parent.modified and complicated.modified
    _clear_modification_flag(parent)

    parent.nested[0].children_list[0] = Simple(simple_child=0)
    assert parent.modified and complicated.modified
    _clear_modification_flag(parent)

    parent.nested[0].children_list[0].simple_child = 42
    assert parent.modified and complicated.modified
    _clear_modification_flag(parent)


def test_notification_management():
    child = Simple(simple_child=0)
    parent = Complex()

    assert not parent.modified

    parent.children_list.append(child)
    _clear_modification_flag(parent)
    child.simple_child = 42
    assert parent.modified
    _clear_modification_flag(parent)

    # After removing a child, it should not affect the old parent
    parent.children_list = []
    _clear_modification_flag(parent)
    child.simple_child = 0
    assert not parent.modified

    parent.children_dict['key'] = child
    _clear_modification_flag(parent)
    child.simple_child = 42
    assert parent.modified

    # After removing a child, it should not affect the old parent
    _clear_modification_flag(parent)
    del parent.children_dict['key']
    _clear_modification_flag(parent)
    child.simple_child = 0
    assert child.modified and not parent.modified
