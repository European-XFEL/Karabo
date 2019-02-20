from traits.api import push_exception_handler, pop_exception_handler

from karabo.common.api import DeviceStatus
from karabo.native import AccessLevel
from karabogui.enums import NavigationItemTypes
from ..device_tree import DeviceTreeNode


def setUp():
    push_exception_handler(lambda *args: None, reraise_exceptions=True)


def tearDown():
    pop_exception_handler()


def test_tree_node_basics():
    empty = DeviceTreeNode(
        node_id='node_id',
        visibility=AccessLevel.OPERATOR,
        status=DeviceStatus.OFFLINE,
        children=[]
    )

    # default global accesslevel is OBSERVER
    empty2 = DeviceTreeNode(
        visibility=AccessLevel.OBSERVER
    )

    assert empty.child('a_child') is None
    assert empty.info() is None
    assert empty.row() == 0
    assert empty.is_visible is False
    assert empty2.is_visible is True


def test_tree_node_levels():
    root = DeviceTreeNode()
    level_0 = DeviceTreeNode(node_id='FXE_OGT1_BIU', parent=root, level=0)
    root.children.append(level_0)
    level_1 = DeviceTreeNode(node_id='MOTOR', parent=level_0, level=1)
    level_0.children.append(level_1)
    level_2 = DeviceTreeNode(node_id='SCREEN', parent=level_1, level=2)
    level_1.children.append(level_2)

    assert root.child('FXE_OGT1_BIU') is level_0
    assert level_0.child('MOTOR') is level_1 and level_0.row() == 0
    assert level_1.child('SCREEN') is level_2 and level_1.row() == 0

    level_0_info = level_0.info()
    assert level_0_info['type'] == NavigationItemTypes.UNDEFINED
    assert level_0_info['domainId'] == 'FXE_OGT1_BIU'
    level_1_info = level_1.info()
    assert level_1_info['type'] == NavigationItemTypes.UNDEFINED
    assert level_1_info['typeId'] == 'MOTOR'
    level_2_info = level_2.info()
    assert level_2_info['type'] == NavigationItemTypes.DEVICE
    assert level_2_info['memberId'] == 'SCREEN'
    assert level_2_info['deviceId'] == ''
