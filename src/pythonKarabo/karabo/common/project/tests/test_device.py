from traits.api import pop_exception_handler, push_exception_handler

from karabo.common.project.api import (
    DeviceConfigurationModel, DeviceInstanceModel, read_device, write_device
)
from karabo.testing.utils import temp_xml_file, xml_is_equal

UUID = 'c43e5c53-bea4-4e9e-921f-042b52e58f4c'
DEVICE_XML = """
<device_instance class_id='BazClass'
                 instance_id='fooDevice'
                 if_exists='ignore'
                 active_uuid='{uuid}'
                 active_rev='1'>
    <device_config uuid='{uuid}' revision='0' class_id='BazClass' />
    <device_config uuid='{uuid}' revision='1' class_id='BazClass' />
    <device_config uuid='{uuid}' revision='2' class_id='BazClass' />
</device_instance>
""".format(uuid=UUID)


def setUp():
    push_exception_handler(lambda *args: None, reraise_exceptions=True)


def tearDown():
    pop_exception_handler()


def test_reading():
    with temp_xml_file(DEVICE_XML) as fn:
        device = read_device(fn)

    assert device.class_id == 'BazClass'
    assert device.instance_id == 'fooDevice'
    assert device.if_exists == 'ignore'
    assert device.active_config_ref == (UUID, 1)


def test_writing():
    conf0 = DeviceConfigurationModel(class_id='BazClass', uuid=UUID,
                                     revision=0)
    conf1 = DeviceConfigurationModel(class_id='BazClass', uuid=UUID,
                                     revision=1)
    conf2 = DeviceConfigurationModel(class_id='BazClass', uuid=UUID,
                                     revision=2)
    foo = DeviceInstanceModel(class_id='BazClass', instance_id='fooDevice',
                              if_exists='ignore',
                              configs=[conf0, conf1, conf2],
                              active_config_ref=(UUID, 1), uuid=UUID,
                              revision=0)

    xml = write_device(foo)
    assert xml_is_equal(DEVICE_XML, xml)


def test_simple_round_trip():
    with temp_xml_file(DEVICE_XML) as fn:
        device = read_device(fn)

    xml = write_device(device)
    assert xml_is_equal(DEVICE_XML, xml)


def test_child_finding():
    conf0 = DeviceConfigurationModel(class_id='BazClass', uuid=UUID,
                                     revision=0)
    conf1 = DeviceConfigurationModel(class_id='BazClass', uuid=UUID,
                                     revision=1)
    conf2 = DeviceConfigurationModel(class_id='BazClass', uuid=UUID,
                                     revision=2)
    foo = DeviceInstanceModel(class_id='BazClass', instance_id='fooDevice',
                              configs=[conf0, conf1, conf2],
                              active_config_ref=(UUID, 0), uuid=UUID,
                              revision=0)
    conf = foo.select_config(UUID, 1)
    assert conf == conf1

    conf = foo.select_config(UUID, 3)
    assert conf is None
