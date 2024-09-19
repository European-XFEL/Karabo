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

from ..device_tree import DeviceTreeNode


def setUp():
    push_exception_handler(lambda *args: None, reraise_exceptions=True)


def tearDown():
    pop_exception_handler()


def test_tree_node_basics():
    empty = DeviceTreeNode(
        node_id='node_id',
        status=InstanceStatus.OK,
        children=[]
    )

    assert empty.child('a_child') is None
    assert empty.info() is None
    assert empty.row() == 0


def test_tree_node_levels():
    root = DeviceTreeNode()
    level_0 = DeviceTreeNode(node_id='FXE_OGT1_BIU', parent=root, level=0)
    root.children.append(level_0)
    level_1 = DeviceTreeNode(node_id='MOTOR', parent=level_0, level=1)
    level_0.children.append(level_1)
    level_2 = DeviceTreeNode(node_id='FXE_OGT1_BIU/MOTOR/SCREEN',
                             parent=level_1, level=2)
    level_1.children.append(level_2)

    assert root.child('FXE_OGT1_BIU') is level_0
    assert level_0.child('MOTOR') is level_1 and level_0.row() == 0
    assert (level_1.child('FXE_OGT1_BIU/MOTOR/SCREEN') is level_2
            and level_1.row() == 0)

    level_0_info = level_0.info()
    assert level_0_info['type'] == NavigationItemTypes.UNDEFINED
    assert level_0_info['domainId'] == 'FXE_OGT1_BIU'
    level_1_info = level_1.info()
    assert level_1_info['type'] == NavigationItemTypes.UNDEFINED
    assert level_1_info['typeId'] == 'MOTOR'
    level_2_info = level_2.info()
    assert level_2_info['type'] == NavigationItemTypes.DEVICE
    assert level_2_info['deviceId'] == 'FXE_OGT1_BIU/MOTOR/SCREEN'
