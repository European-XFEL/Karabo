# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.common.api import WeakMethodRef


def test_weak_method_ref():
    """Test that we can safely call bound methods"""
    global called
    called = False

    class Device:
        def move(self):
            global called
            called = True

    device = Device()
    func = WeakMethodRef(device.move)
    func()
    assert called is True
    called = False
    del device
    func()
    assert called is False
