# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from ..system_tree import SystemTree


def print_tree_r(tree, indent=0):
    if isinstance(tree, SystemTree):
        tree = tree.root

    leading_space = '  ' * indent
    for child in tree.children:
        print(leading_space, child.node_id)
        print_tree_r(child, indent=indent+1)
