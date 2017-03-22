from traits.api import push_exception_handler, pop_exception_handler

from karabo.middlelayer import AccessLevel
from karabo_gui.enums import NavigationItemTypes
from ..tree import SystemTree, SystemTreeNode
from .utils import system_hash


def setUp():
    push_exception_handler(lambda *args: None, reraise_exceptions=True)


def tearDown():
    pop_exception_handler()


def test_tree_node_basics():
    empty = SystemTreeNode(
        node_id='node_id', path='path',
        visibility=AccessLevel.OPERATOR,
        status='testing',
        children=[]
    )

    assert empty.child('a_child') is None
    assert empty.level() == -1
    assert empty.info() is None
    assert empty.row() is None


def test_tree_node_levels():
    root = SystemTreeNode()
    level_0 = SystemTreeNode(node_id='level_0', parent=root)
    root.children.append(level_0)
    level_1 = SystemTreeNode(node_id='level_1', parent=level_0)
    level_0.children.append(level_1)
    level_2 = SystemTreeNode(node_id='level_2', parent=level_1)
    level_1.children.append(level_2)
    level_3 = SystemTreeNode(node_id='level_3', parent=level_2)
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
    assert tree.find('anything') is None

    sys_hash = system_hash()
    names = ('swerver', 'swerver.FooClass', 'swerver.BarClass', 'divvy',
             'macdonald', 'orphan', '__none__')

    for node_id in names:
        assert tree.find(node_id) is None

    tree.update(sys_hash)
    for node_id in names:
        assert tree.find(node_id) is not None

    tree.clear_all()
    for node_id in names:
        assert tree.find(node_id) is None


def test_tree_clear_existing():
    tree = SystemTree()
    names = ('swerver', 'swerver.FooClass', 'swerver.BarClass', 'divvy',
             'macdonald', 'orphan')
    sys_hash = system_hash()

    tree.update(sys_hash)
    tree.clear_existing(sys_hash)
    for node_id in names:
        assert tree.find(node_id) is None
