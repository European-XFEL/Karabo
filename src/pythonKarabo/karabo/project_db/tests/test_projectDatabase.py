from unittest import TestCase
from uuid import uuid4

from lxml import etree

from karabo.project_db.project_database import ProjectDatabase
from karabo.project_db.util import stop_database


def _gen_uuid():
    return str(uuid4())


def create_hierarchy(db):
    uuid = _gen_uuid()
    xml = ('<xml revision="0" item_type="{atype}" '
           'uuid="{uuid}" alias = "{alias}" '
           'simple_name="{name}">').format(uuid=uuid, atype='project',
                                           alias=uuid, name=uuid)

    xml += "<children>"

    # create some scenes
    for i in range(4):
        sub_uuid = _gen_uuid()
        xml += ('<xml item_type="{atype}" uuid="{uuid}"'
                ' alias = "{alias}" revision="0"'
                ' simple_name="{name}" />').format(uuid=sub_uuid,
                                                   atype='scene',
                                                   alias=sub_uuid,
                                                   name=sub_uuid)

        scene_xml = ('<xml item_type="{atype}" uuid="{uuid}"'
                     ' alias = "{alias}" revision="0"'
                     ' simple_name="{name}" >foo</xml>'
                     .format(uuid=sub_uuid, atype='scene',
                             alias=uuid, name=sub_uuid))

        db.save_item("LOCAL", sub_uuid, scene_xml)

    # create some device_servers
    for i in range(4):
        sub_uuid = _gen_uuid()
        xml += ('<xml item_type="{atype}" uuid="{uuid}"'
                ' alias = "{alias}" revision="0"'
                ' simple_name="{name}" />').format(uuid=sub_uuid,
                                                   atype='device_server',
                                                   alias=uuid,
                                                   name=sub_uuid)

        ds_xml = ('<xml item_type="{atype}" uuid="{uuid}"'
                  ' alias = "{alias}" revision="0"'
                  ' simple_name="{name}" >foo</xml>'
                  .format(uuid=sub_uuid, atype='device_server',
                          alias=uuid, name=sub_uuid))

        db.save_item("LOCAL", sub_uuid, ds_xml)

    xml += "</children>"
    xml += "</xml>"
    db.save_item("LOCAL", uuid, xml)
    return uuid


class TestProjectDatabase(TestCase):
    user = "admin"
    password = "karabo"

    def test__make_xml_if_needed(self):
            xml_rep = "<test>foo</test>"
            ret = ProjectDatabase._make_xml_if_needed(xml_rep)
            self.assertEqual(ret.tag, "test")
            self.assertEqual(ret.text, "foo")

    def test__make_str_if_needed(self):
        element = etree.Element('test')
        element.text = 'foo'
        str_rep = ProjectDatabase._make_str_if_needed(element)
        self.assertEqual(str_rep, "<test>foo</test>\n")

    def test_project_interface(self):
        # A bunch of document "names" for the following tests
        testproject = _gen_uuid()
        testproject2 = _gen_uuid()

        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:

            # remove previously existing test collection
            path = "{}/{}".format(db.root, 'LOCAL_TEST')
            if db.dbhandle.hasCollection(path):
                db.dbhandle.removeCollection(path)

            path = "{}/{}".format(db.root, 'LOCAL')
            if db.dbhandle.hasCollection(path):
                db.dbhandle.removeCollection(path)

            # make sure we have the LOCAL collection and subcollections
            path = "{}/{}".format(db.root, 'LOCAL')
            if not db.dbhandle.hasCollection(path):
                db.dbhandle.createCollection(path)

            with self.subTest(msg='test_domain_exists'):
                self.assertTrue(db.domain_exists("LOCAL"))

            with self.subTest(msg='test_add_domain'):
                db.add_domain("LOCAL_TEST")
                self.assertTrue(db.domain_exists("LOCAL_TEST"))

            with self.subTest(msg='test_get_versioning_info'):
                xml_rep = """
                <xml uuid="{uuid}"
                      revision="2"
                      date="the day after tomorrow"
                      user="bob"
                      alias="test">foo</xml>
                """.format(uuid=testproject)

                # db.save_project('LOCAL', testproject, ret)
                path = "{}/{}/{}_2".format(db.root, 'LOCAL', testproject)
                db.dbhandle.load(xml_rep, path)

                vers = db.get_versioning_info('LOCAL', testproject)
                self.assertEqual(vers['document'],
                                 '/krb_test/LOCAL/{}'.format(testproject))
                # depends on how often tests were run
                self.assertGreaterEqual(len(vers['revisions']), 1)
                first_rev = vers['revisions'][0]
                self.assertTrue('revision' in first_rev)
                self.assertTrue('date' in first_rev)
                self.assertTrue('alias' in first_rev)
                self.assertEqual(first_rev['user'], 'bob')

            with self.subTest(msg='test_save_item'):
                xml_rep = '<test revision="42">foo</test>'
                meta= db.save_item('LOCAL', testproject2, xml_rep)

                path = "{}/LOCAL/{}_42".format(db.root, testproject2)
                self.assertTrue(db.dbhandle.hasDocument(path))
                decoded = db.dbhandle.getDoc(path).decode('utf-8')
                self.assertEqual(decoded, xml_rep)
                self.assertTrue('versioning_info' in meta)
                self.assertTrue('domain' in meta)
                self.assertTrue('uuid' in meta)

            with self.subTest(msg='load_item'):
                item = db.load_item('LOCAL', testproject, 2)
                itemxml = db._make_xml_if_needed(item)
                self.assertEqual(itemxml.tag, 'xml')
                self.assertEqual(itemxml.text, 'foo')

            with self.subTest(msg='test_list_items'):
                create_hierarchy(db)
                items = db.list_items('LOCAL', ['project', 'scene'])
                self.assertEqual(len(items), 5)
                scenecnt = 0
                for i in items:
                    if i["item_type"] == "scene":
                        scenecnt += 1
                self.assertGreaterEqual(scenecnt, 4)

            with self.subTest(msg='test_list_domains'):
                items = db.list_domains()
                for it in items:
                    self.assertTrue(it in ['LOCAL_TEST', 'REPO', 'LOCAL'])

            stop_database()
