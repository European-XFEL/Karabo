from karabo.common.api import Capabilities
from karabo.middlelayer import AccessLevel, Hash
from ..tree import SystemTree


def print_tree_r(tree, indent=0):
    if isinstance(tree, SystemTree):
        tree = tree.root

    leading_space = '  ' * indent
    for child in tree.children:
        print(leading_space, child.node_id)
        print_tree_r(child, indent=indent+1)


def system_hash():
    """Generate a system hash which will be built into a system tree
    """
    h = Hash()

    h['server.swerver'] = None
    h['server.swerver', ...] = {
        'host': 'BIG_IRON',
        'visibility': AccessLevel.ADMIN,
        'deviceClasses': ['FooClass', 'BarClass'],
        'visibilities': [AccessLevel.USER, AccessLevel.GOD]
    }
    h['device.divvy'] = None
    h['device.divvy', ...] = {
        'host': 'BIG_IRON',
        'visibility': AccessLevel.ADMIN,
        'capabilities': Capabilities.PROVIDES_SCENES,
        'serverId': 'swerver',
        'classId': 'FooClass',
        'status': 'testing'
    }
    h['macro.macdonald'] = None
    h['macro.macdonald', ...] = {
        'host': 'BIG_IRON',
        'visibility': AccessLevel.ADMIN,
        'serverId': 'swerver',
        'classId': 'BarClass',
        'status': 'metacontrolling'
    }

    h['device.orphan'] = None
    h['device.orphan', ...] = {
        'visibility': AccessLevel.ADMIN,
        'serverId': '__none__',
        'classId': 'Parentless',
        'status': 'existential'
    }

    return h
