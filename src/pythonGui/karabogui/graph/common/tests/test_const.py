# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.common.states import State
from karabogui.graph.common.const import ALL_STATES


def test_const_state_def():
    """Make sure that our own state mapping has all the values of Karabo"""
    state_defs = State.__members__.values()
    assert len(set(ALL_STATES)) == len(state_defs)

    for state in State:
        assert state in ALL_STATES
