# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.common import services as servicemod


def test_num_services():
    assert len(servicemod.CORE_DEVICES_NAMES) == 4
