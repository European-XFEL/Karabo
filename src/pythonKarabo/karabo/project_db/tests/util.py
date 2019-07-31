import os
from subprocess import check_call
from uuid import uuid4


def _gen_uuid():
    return str(uuid4())


def create_hierarchy(db):
    uuid = _gen_uuid()
    xml = ('<xml item_type="{atype}" uuid="{uuid}" '
           'simple_name="{name}">').format(uuid=uuid, atype='project',
                                           name='Project')

    xml += "<children>"

    # create some scenes
    for i in range(4):
        sub_uuid = _gen_uuid()
        xml += ('<xml item_type="{atype}" uuid="{uuid}"'
                ' simple_name="{name}" />').format(uuid=sub_uuid,
                                                   atype='scene',
                                                   name=sub_uuid)

        scene_xml = ('<xml item_type="{atype}" uuid="{uuid}"'
                     ' simple_name="{name}" >中文</xml>'
                     .format(uuid=sub_uuid, atype='scene', name=sub_uuid))

        db.save_item("LOCAL", sub_uuid, scene_xml)

    # create some device_servers
    for i in range(4):
        sub_uuid = _gen_uuid()
        xml += ('<xml item_type="{atype}" uuid="{uuid}"'
                ' simple_name="{name}" />').format(uuid=sub_uuid,
                                                   atype='device_server',
                                                   name=sub_uuid)

        ds_xml = ('<xml item_type="{atype}" uuid="{uuid}"'
                  ' simple_name="{name}" >foo</xml>'
                  .format(uuid=sub_uuid, atype='device_server', name=sub_uuid))

        db.save_item("LOCAL", sub_uuid, ds_xml)

    xml += "</children>"
    xml += "</xml>"
    db.save_item("LOCAL", uuid, xml)
    return uuid


def stop_local_database():
    """
    Stops a **locally** running instance of eXistDB if this is the one used
    :return:
    """
    test_db_host = os.getenv('KARABO_TEST_PROJECT_DB', None)
    if test_db_host and test_db_host != 'localhost':
        return
    karabo_install = os.getenv('KARABO')
    script_path = os.path.join(karabo_install, 'bin',
                               'karabo-stopconfigdb')
    check_call([script_path])
