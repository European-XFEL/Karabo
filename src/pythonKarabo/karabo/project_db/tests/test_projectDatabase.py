from time import gmtime, strftime, time
from unittest import TestCase
from uuid import uuid4

from lxml import etree

from karabo.project_db.project_database import ProjectDatabase
from karabo.project_db.util import stop_database


def _gen_uuid():
    return str(uuid4())


def create_hierarchy(db):
    uuid = _gen_uuid()
    xml = ('<xml item_type="{atype}" uuid="{uuid}" '
           'simple_name="{name}">').format(uuid=uuid, atype='project',
                                           name=uuid)

    xml += "<children>"

    # create some scenes
    for i in range(4):
        sub_uuid = _gen_uuid()
        xml += ('<xml item_type="{atype}" uuid="{uuid}"'
                ' simple_name="{name}" />').format(uuid=sub_uuid,
                                                   atype='scene',
                                                   name=sub_uuid)

        scene_xml = ('<xml item_type="{atype}" uuid="{uuid}"'
                     ' simple_name="{name}" >foo</xml>'
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

            with self.subTest(msg='test_save_item'):
                xml_rep = """
                <xml uuid="{uuid}">foo</xml>
                """.format(uuid=testproject2)

                meta = db.save_item('LOCAL', testproject2, xml_rep)

                path = "{}/LOCAL/{}".format(db.root, testproject2)
                self.assertTrue(db.dbhandle.hasDocument(path))
                decoded = db.dbhandle.getDoc(path).decode('utf-8')
                doctree = etree.fromstring(decoded)
                self.assertEqual(doctree.get('uuid'), testproject2)
                self.assertEqual(doctree.text, 'foo')
                self.assertTrue('domain' in meta)
                self.assertTrue('uuid' in meta)

            with self.subTest(msg='load_items'):
                items = [testproject, testproject2]
                res = db.load_item('LOCAL', items)
                for r in res:
                    itemxml = db._make_xml_if_needed(r["xml"])
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

            with self.subTest(msg='test_old_versioned_data'):
                versioned_uuid = _gen_uuid()
                xml_rep = """
                <xml uuid="{uuid}" revision="{revision}" date="{date}"
                     user="sue">foo</xml>
                """
                MAX_REV = 3
                earlier = time() - 5.0
                # Save three consecutive versions
                for i in range(MAX_REV, 0, -1):
                    # consecutive dates beginning a few seconds ago
                    date = strftime("%Y-%m-%d %H:%M:%S", gmtime(earlier+i))
                    xml = xml_rep.format(uuid=versioned_uuid, revision=i,
                                         date=date)
                    path = "{}/{}/{}_{}".format(db.root, 'LOCAL',
                                                versioned_uuid, i)
                    # Save directly without engaging save_item()
                    db.dbhandle.load(xml, path)

                # Read it back, no revision given
                res = db.load_item('LOCAL', [versioned_uuid])
                self.assertEqual(len(res), 1)
                itemxml = db._make_xml_if_needed(res[0]['xml'])
                self.assertEqual(int(itemxml.get('revision')), MAX_REV)

                # Write it back with save_item
                date = strftime("%Y-%m-%d %H:%M:%S", gmtime())
                xml_rep = """
                <xml uuid="{uuid}" date="{date}" user="sue">foo</xml>
                """.format(uuid=versioned_uuid, date=date)
                meta = db.save_item('LOCAL', versioned_uuid, xml_rep)

                # Make sure there's a revision of 0 (auto-added)
                res = db.load_item('LOCAL', [versioned_uuid])
                itemxml = db._make_xml_if_needed(res[0]['xml'])
                self.assertEqual(itemxml.get('revision'), '0')

            stop_database()
