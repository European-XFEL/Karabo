# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from traits.api import pop_exception_handler, push_exception_handler

from karabo.common.api import InstanceStatus
from karabogui.itemtypes import NavigationItemTypes
from karabogui.testing import system_hash, system_hash_server_and_plugins

from ..system_tree import SystemTree, SystemTreeNode


def setUp():
    push_exception_handler(lambda *args: None, reraise_exceptions=True)


def tearDown():
    pop_exception_handler()


def test_tree_node_basics():
    empty = SystemTreeNode(
        node_id='node_id', path='path',
        status=InstanceStatus.NONE,
        children=[]
    )

    assert empty.child('a_child') is None
    assert empty.info() is None
    assert empty.row() == 0


def test_tree_node_levels():
    root = SystemTreeNode()
    level_0 = SystemTreeNode(node_id='level_0', parent=root, level=0)
    root.children.append(level_0)
    level_1 = SystemTreeNode(node_id='level_1', parent=level_0, level=1)
    level_0.children.append(level_1)
    level_2 = SystemTreeNode(node_id='level_2', parent=level_1, level=2)
    level_1.children.append(level_2)
    level_3 = SystemTreeNode(node_id='level_3', parent=level_2, level=3)
    level_2.children.append(level_3)

    assert root.child('level_0') is level_0
    assert level_0.child('level_1') is level_1 and level_0.row() == 0
    assert level_1.child('level_2') is level_2 and level_1.row() == 0
    assert level_2.child('level_3') is level_3 and level_2.row() == 0

    level_0_info = level_0.info()
    assert level_0_info['type'] == NavigationItemTypes.HOST
    level_1_info = level_1.info()
    assert level_1_info['type'] == NavigationItemTypes.SERVER
    assert level_1_info['serverId'] == 'level_1'
    assert level_1_info['log'] == 'INFO'
    level_2_info = level_2.info()
    assert level_2_info['type'] == NavigationItemTypes.CLASS
    assert level_2_info['serverId'] == 'level_1'
    assert level_2_info['classId'] == 'level_2'
    assert level_2_info['deviceId'] == ''
    level_3_info = level_3.info()
    assert level_3_info['type'] == NavigationItemTypes.DEVICE
    assert level_3_info['classId'] == 'level_2'
    assert level_3_info['deviceId'] == 'level_3'


def test_tree_basics():
    tree = SystemTree()
    assert len(tree.find('anything')) == 0

    sys_hash = system_hash()
    names = ('swerver', 'divvy', 'macdonald')

    for node_id in names:
        assert len(tree.find(node_id)) == 0

    tree.initialize(sys_hash)
    for node_id in names:
        assert len(tree.find(node_id)) > 0, node_id

    tree.clear_all()
    for node_id in names:
        assert len(tree.find(node_id)) == 0


def test_tree_find():
    tree = SystemTree()
    names = ('FooClass', 'BarClass')

    for node_id in names:
        assert len(tree.find(node_id)) == 0

    h = system_hash_server_and_plugins()
    tree.update(h)
    assert len(tree.find('BarClass')) == 1
    assert len(tree.find('barclass', case_sensitive=False)) == 1
    assert len(tree.find('Bar')) == 1
    assert len(tree.find('FooClass')) == 1
    assert len(tree.find('BarClass')) == 1
    assert len(tree.find('BlahClass')) == 0
    kwargs = {'case_sensitive': True,
              'use_reg_ex': True}
    assert len(tree.find('(.*)Class', **kwargs)) == 2
    assert len(tree.find('(.*)class', **kwargs)) == 1
    kwargs['case_sensitive'] = False
    kwargs['use_reg_ex'] = True
    assert len(tree.find('(.*)fooclass', **kwargs)) == 1
    assert len(tree.find('HooClass', **kwargs)) == 0
    kwargs['full_match'] = True
    assert len(tree.find('HooClass', **kwargs)) == 0
    assert len(tree.find('HooClass_0', **kwargs)) == 0
    assert len(tree.find('HooClass_1', **kwargs)) == 0


def test_tree_clear_existing():
    tree = SystemTree()
    names = ('swerver', 'divvy', 'macdonald', 'orphan')
    sys_hash = system_hash()

    tree.update(sys_hash)
    tree.clear_existing(sys_hash)
    for node_id in names:
        assert len(tree.find(node_id)) == 0
