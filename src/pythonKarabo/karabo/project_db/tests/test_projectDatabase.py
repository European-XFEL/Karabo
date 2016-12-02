import copy
from unittest import TestCase

from lxml import etree

from karabo.project_db.project_database import ProjectDatabase
from karabo.project_db.util import stop_database


def create_hierarchy(db, prefix, uuid_suf, level=0):
    uuid = "{}_{}".format(prefix, uuid_suf)
    xml = '<project item_type="{atype}" uuid="{uuid}" alias = "{alias}"'\
          ' simple_name="{name}">'.format(uuid=uuid, atype='project',
                                          alias=uuid, name=uuid)

    xml += "<children>"
    counter = 0
    # create some subprojects
    if level < 2:
        for i in range(4):
            sub_uuid = create_hierarchy(db, uuid, counter, level + 1)

            xml += ('<project item_type="{atype}" uuid="{uuid}"'
                    ' alias = "{alias}"'
                    ' simple_name="{name}" />').format(uuid=sub_uuid,
                                                       atype='project',
                                                       alias=sub_uuid,
                                                       name=sub_uuid)
            counter += 1

    # create some scenes
    for i in range(4):
        sub_uuid = '{}_{}'.format(uuid, counter)
        xml += ('<scene item_type="{atype}" uuid="{uuid}"'
                ' alias = "{alias}"'
                ' simple_name="{name}" />').format(uuid=sub_uuid,
                                                   atype='scene',
                                                   alias=sub_uuid,
                                                   name=sub_uuid)

        scene_xml = ('<scene item_type="{atype}" uuid="{uuid}"'
                     ' alias = "{alias}"'
                     ' simple_name="{name}" >foo</scene>'
                     .format(uuid=sub_uuid, atype='scene',
                             alias=uuid, name=sub_uuid))

        db.save_item("LOCAL", sub_uuid, scene_xml)

        counter += 1

    # create some device_servers
    for i in range(4):
        sub_uuid = '{}_{}'.format(uuid, counter)
        xml += ('<device_server item_type="{atype}" uuid="{uuid}"'
                ' alias = "{alias}"'
                ' simple_name="{name}" />').format(uuid=sub_uuid,
                                                   atype='device_server',
                                                   alias=uuid,
                                                   name=sub_uuid)

        ds_xml = ('<device_server item_type="{atype}" uuid="{uuid}"'
                  ' alias = "{alias}"'
                  ' simple_name="{name}" >foo</device_server>'
                  .format(uuid=sub_uuid, atype='device_server',
                          alias=uuid, name=sub_uuid))

        db.save_item("LOCAL", sub_uuid, ds_xml)

        counter += 1

    xml += "</children>"
    xml += "</project>"
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
                xml_rep = "<test alias='test'>foo</test>"

                # db.save_project('LOCAL', 'testproject', ret)
                path = "{}/{}".format(db.root, 'LOCAL/testproject')
                db.dbhandle.load(xml_rep, path)
                # do twice to assure a version increment
                db.dbhandle.load(xml_rep, path)

                vers = db.get_versioning_info(path)
                self.assertEqual(vers['document'],
                                 '/db/krb_test/LOCAL/testproject')
                # depends on how often tests were run
                self.assertGreaterEqual(len(vers['revisions']), 0)
                first_rev = vers['revisions'][0]
                self.assertTrue('revision' in first_rev)
                self.assertTrue('date' in first_rev)
                self.assertTrue('alias' in first_rev)
                self.assertEqual(first_rev['user'], 'admin')

            with self.subTest(msg='test_save_item'):
                xml_rep = '<test>foo</test>'
                success, meta = db.save_item('LOCAL', 'testproject2', xml_rep)

                path = "{}/LOCAL/testproject2".format(db.root)
                self.assertTrue(db.dbhandle.hasDocument(path))
                decoded = db.dbhandle.getDoc(path).decode('utf-8')
                self.assertEqual(decoded, xml_rep)
                self.assertTrue(success)
                self.assertTrue('versioning_info' in meta)
                self.assertTrue('domain' in meta)
                self.assertTrue('uuid' in meta)

            with self.subTest(msg='test_copy_item'):

                xml_rep = "<test>foo</test>"
                db.save_item('LOCAL', 'testproject', xml_rep)

                db.copy_item('LOCAL', 'REPO', 'testproject',
                             'testproject_copy')
                path = "{}/REPO/testproject_copy".format(db.root)
                origin_path = "{}/LOCAL/testproject".format(db.root)
                self.assertTrue(db.dbhandle.hasDocument(path))
                decoded1 = db.dbhandle.getDoc(path).decode('utf-8')
                decoded2 = db.dbhandle.getDoc(origin_path).decode('utf-8')
                self.assertEqual(decoded1, decoded2)

            with self.subTest(msg='test_rename_item'):

                origin_path = "{}/REPO/testproject_copy".format(db.root)
                origin = db.dbhandle.getDoc(origin_path).decode('utf-8')

                db.rename_item('REPO', 'testproject_copy', 'testproject_copy2')
                path = "{}/REPO/testproject_copy2".format(db.root)

                self.assertTrue(db.dbhandle.hasDocument(path))
                self.assertEqual(db.dbhandle.getDoc(path).decode('utf-8'),
                                 origin)

            with self.subTest(msg='test_move_item'):
                origin_path = "{}/REPO/testproject_copy2".format(db.root)

                origin = db.dbhandle.getDoc(origin_path).decode('utf-8')

                db.move_item('REPO', 'LOCAL', 'testproject_copy2')
                path = "{}/LOCAL/testproject_copy2".format(db.root)

                self.assertTrue(db.dbhandle.hasDocument(path))
                self.assertEqual(db.dbhandle.getDoc(path).decode('utf-8'),
                                 origin)

            with self.subTest(msg='load_item'):
                item, revision = db.load_item('LOCAL', 'testproject_copy2')
                itemxml = db._make_xml_if_needed(item)
                self.assertEqual(itemxml.tag, 'test')
                self.assertEqual(itemxml.text, 'foo')

            with self.subTest(msg='test_save_item_conflict'):
                xml_rep_start = '<test>foo</test>'

                success, meta = db.save_item('LOCAL', 'testproject2',
                                             xml_rep_start)
                self.assertTrue(success)

                path = "{}/LOCAL/testproject2".format(db.root)
                doc = db._make_xml_if_needed(db.load_item('LOCAL',
                                                          'testproject2')[0])

                success, meta = db.save_item('LOCAL', 'testproject2',
                                             xml_rep_start)

                doc.text = 'goo'

                doc2 = copy.copy(doc)
                doc2.text = 'hoo'
                doc2.attrib['{http://exist-db.org/versioning}revision'] = "-1"
                doc2.attrib['{http://exist-db.org/versioning}key'] = "fdsf"

                success, meta = db.save_item('LOCAL', 'testproject2', doc)
                self.assertTrue(success)
                success, meta = db.save_item('LOCAL', 'testproject2', doc)
                self.assertTrue(success)

                success, meta = db.save_item('LOCAL', 'testproject2', doc2)

                self.assertTrue(db.dbhandle.hasDocument(path))
                test = db._make_xml_if_needed(db.load_item('LOCAL',
                                                           'testproject2')[0])
                self.assertEqual(test.text, 'goo')
                self.assertFalse(success)
                self.assertTrue('versioning_info' in meta)

                # now overwrite
                success, meta = db.save_item('LOCAL', 'testproject2', doc2,
                                             True)

                self.assertTrue(db.dbhandle.hasDocument(path))
                test = db._make_xml_if_needed(db.load_item('LOCAL',
                                                           'testproject2')[0])
                self.assertEqual(test.text, 'hoo')

            with self.subTest(msg='test_load_multi'):
                # create a device server and multiple config entries
                xml_reps = ['<test uuid="0">foo</test>',
                            '<test uuid="1">goo</test>',
                            '<test uuid="2">hoo</test>',
                            '<test uuid="3">nope</test>']

                for i, rep in enumerate(xml_reps):
                    success, meta = db.save_item('LOCAL', 'testconfig{}'
                                                 .format(i), rep, True)
                    self.assertTrue(success)

                # twice to initiate versioning
                for i, rep in enumerate(xml_reps):
                    success, meta = db.save_item('LOCAL', 'testconfig{}'
                                                 .format(i), rep, True)
                    self.assertTrue(success)

                self.assertTrue(success)

                # get version info for what we inserted
                revisions = []
                for i in range(3):
                    path = "{}/LOCAL/testconfig{}"\
                            .format(db.root, i)
                    v = db.get_versioning_info(path)
                    revisions.append(v['revisions'][-1]['revision'])

                xml_serv = "<testserver list_tag='configs'><configs>"
                for i in range(3):
                    xml_serv += '<configuration uuid="{}" revision="{}"/>'\
                                .format(i, revisions[i])
                xml_serv += " </configs></testserver>"

                success, meta = db.save_item('LOCAL', 'testserver_m', xml_serv)
                self.assertTrue(success)

                # now load again
                res = db.load_multi('LOCAL', xml_serv, ['configs'])
                for i, item in enumerate(res):
                    rxml = db._make_xml_if_needed(item['xml'])
                    rindex = int(rxml.get('uuid'))
                    gxml = db._make_xml_if_needed(xml_reps[rindex])
                    self.assertEqual(rxml.text, gxml.text)
                    self.assertEqual(rxml.attrib['uuid'], gxml.attrib['uuid'])

            with self.subTest(msg='test_versioning_from_item'):
                vers = db.get_versioning_info_item("LOCAL", "1")
                self.assertEqual(vers['document'],
                                 '/db/krb_test/LOCAL/testconfig1')

            create_hierarchy(db, "hierarchy_test", 0, 0)

            with self.subTest(msg='test_list_items'):
                items = db.list_items('LOCAL', ['project', 'scene'])
                self.assertEqual(len(items), 10)
                scenecnt = 0
                for i in items:
                    if i["item_type"] == "scene":
                        scenecnt += 1
                self.assertGreaterEqual(scenecnt, 8)

            with self.subTest(msg='test_list_domains'):
                items = db.list_domains()
                for it in items:
                    self.assertTrue(it in ['LOCAL_TEST', 'REPO', 'LOCAL'])


            stop_database()
