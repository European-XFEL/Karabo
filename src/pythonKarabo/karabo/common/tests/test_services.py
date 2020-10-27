from karabo.common import services as servicemod


def test_num_services():
    assert len(servicemod.CORE_DEVICES_NAMES) == 4
