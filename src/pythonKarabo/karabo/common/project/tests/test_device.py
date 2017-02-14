from uuid import uuid4

from traits.api import pop_exception_handler, push_exception_handler

from karabo.common.api import walk_traits_object
from karabo.common.project.api import (
    BaseProjectObjectModel, DeviceConfigurationModel, DeviceInstanceModel,
    read_device, write_device
)
from karabo.testing.utils import temp_xml_file, xml_is_equal


UUID = str(uuid4())
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


def test_uuid_revision_update():
    def _visitor(model):
        if isinstance(model, BaseProjectObjectModel):
            model.uuid = str(uuid4())
            model.revision = 0

    conf0_uuid = str(uuid4())
    conf1_uuid = str(uuid4())
    dev_uuid = str(uuid4())
    conf0 = DeviceConfigurationModel(class_id='BarClass', uuid=conf0_uuid,
                                     revision=42)
    conf1 = DeviceConfigurationModel(class_id='BarClass', uuid=conf1_uuid,
                                     revision=123)
    dev = DeviceInstanceModel(class_id='BarClass', instance_id='fooDevice',
                              configs=[conf0, conf1],
                              active_config_ref=(conf0_uuid, 42),
                              uuid=dev_uuid, revision=1)

    selected_conf = dev.select_config(conf0_uuid, 42)
    assert selected_conf is conf0

    walk_traits_object(dev, _visitor)
    assert conf0.uuid != conf0_uuid
    assert conf1.uuid != conf1_uuid
    assert dev.uuid != dev_uuid
    assert dev.active_config_ref == (conf0.uuid, 0)

    selected_conf = dev.select_config(conf0.uuid, 0)
    assert selected_conf is conf0
