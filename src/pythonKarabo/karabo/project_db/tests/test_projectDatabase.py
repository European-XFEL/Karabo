from unittest import TestCase
from karabo.project_db.project_database import ProjectDatabase
from lxml import etree
import copy


class TestProjectDatabase(TestCase):
    user = "admin"
    password = "karabo"

    def setUp(self):
        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:

            # remove previously existing test collection
            path = "{}/{}".format(db.root, 'LOCAL_TEST')
            if db.dbhandle.hasCollection(path):
                db.dbhandle.removeCollection(path)

            # make sure we have the LOCAL collection and subcollections
            path = "{}/{}".format(db.root, 'LOCAL')
            if not db.dbhandle.hasCollection(path):
                db.dbhandle.createCollection(path)
                for p in db.known_xml_types:
                    ppath = "{}/{}".format(path, p)
                    db.dbhandle.createCollection(ppath)

    def test_domain_exists(self):
        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:
            self.assertTrue(db.domain_exists("LOCAL"))

    def test_add_domain(self):
        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:
            db.add_domain("LOCAL_TEST")
            self.assertTrue(db.domain_exists("LOCAL_TEST"))
            # we also check that all sub collections have been created
            path = "{}/{}".format(db.root, 'LOCAL_TEST')
            for p in db.known_xml_types:
                    ppath = "{}/{}".format(path, p)
                    self.assertTrue(db.dbhandle.hasCollection(ppath))

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

    def test_get_versioning_info(self):

        xml_rep = "<test>foo</test>"
        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:
            # db.save_project('LOCAL', 'testproject', ret)
            path = "{}/{}".format(db.root, '/LOCAL/projects/testproject')
            db.dbhandle.load(xml_rep, path)
            vers = db.get_versioning_info(path)
            self.assertEqual(vers['document'],
                             '/db/krb_test/LOCAL/projects/testproject')
            # depends on how often tests were run
            self.assertGreaterEqual(len(vers['revisions']), 0)
            first_rev = vers['revisions'][0]
            self.assertTrue('id' in first_rev)
            self.assertTrue('date' in first_rev)
            self.assertEqual(first_rev['user'], 'admin')

    def test__check_for_known_xml_type(self):
        testTypes = ['projects', 'scenes', 'macros', 'configs',
                     'device_servers']
        for t in testTypes:
            self.assertTrue(t in ProjectDatabase.known_xml_types)

    def test_save_project(self):
        xml_rep = '<test>foo</test>'
        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:
            success, meta = db.save_project('LOCAL', 'testproject2', xml_rep)
            path = "{}/LOCAL/projects/testproject2".format(db.root)
            self.assertTrue(db.dbhandle.hasDocument(path))
            self.assertEqual(db.dbhandle.getDoc(path).decode('utf-8'), xml_rep)
            self.assertTrue(success)
            self.assertTrue('versioning_info' in meta)
            self.assertEqual(meta['current_xml'], xml_rep)

    def test_save_scene(self):
        xml_rep = '<test>foo</test>'
        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:
            success, meta = db.save_scene('LOCAL', 'testscene', xml_rep)
            path = "{}/LOCAL/scenes/testscene".format(db.root)
            self.assertTrue(db.dbhandle.hasDocument(path))
            self.assertEqual(db.dbhandle.getDoc(path).decode('utf-8'), xml_rep)
            self.assertTrue(success)
            self.assertTrue('versioning_info' in meta)
            self.assertEqual(meta['current_xml'], xml_rep)

    def test_save_config(self):
        xml_rep = '<test>foo</test>'
        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:
            success, meta = db.save_config('LOCAL', 'testconfig', xml_rep)
            path = "{}/LOCAL/configs/testconfig".format(db.root)
            self.assertTrue(db.dbhandle.hasDocument(path))
            self.assertEqual(db.dbhandle.getDoc(path).decode('utf-8'), xml_rep)
            self.assertTrue(success)
            self.assertTrue('versioning_info' in meta)
            self.assertEqual(meta['current_xml'], xml_rep)

    def test_save_device_server(self):
        xml_rep = '<test>foo</test>'
        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:
            success, meta = db.save_device_server('LOCAL', 'testdeviceserver',
                                                  xml_rep)
            path = "{}/LOCAL/device_servers/testdeviceserver".format(db.root)
            self.assertTrue(db.dbhandle.hasDocument(path))
            self.assertEqual(db.dbhandle.getDoc(path).decode('utf-8'), xml_rep)
            self.assertTrue(success)
            self.assertTrue('versioning_info' in meta)
            self.assertEqual(meta['current_xml'], xml_rep)

    def test__copy_item(self):

        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:

            xml_rep = "<test>foo</test>"
            db.save_project('LOCAL', 'testproject', xml_rep)

            db._copy_item('projects', 'LOCAL', 'REPO', 'testproject',
                          'testproject_copy')
            path = "{}/REPO/projects/testproject_copy".format(db.root)
            origin_path = "{}/LOCAL/projects/testproject".format(db.root)
            self.assertTrue(db.dbhandle.hasDocument(path))
            self.assertEqual(db.dbhandle.getDoc(path).decode('utf-8'),
                             db.dbhandle.getDoc(origin_path).decode('utf-8'))

    def test__rename_item(self):
        # for this test to work certain pre-requirements exist in terms of
        # structures in the data base. We run these first to allow out of
        # order and parallel testing.
        self.test_save_project()
        self.test__copy_item()
        # pre-requirements end here!

        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:
            origin_path = "{}/REPO/projects/testproject_copy".format(db.root)
            origin = db.dbhandle.getDoc(origin_path).decode('utf-8')

            db._rename_item('projects', 'REPO', 'testproject_copy',
                            'testproject_copy2')
            path = "{}/REPO/projects/testproject_copy2".format(db.root)

            self.assertTrue(db.dbhandle.hasDocument(path))
            self.assertEqual(db.dbhandle.getDoc(path).decode('utf-8'),
                             origin)

    def test__move_item(self):
        # for this test to work certain pre-requirements exist in terms of
        # structures in the data base. We run these first to allow out of
        # order and parallel testing.
        self.test_save_project()
        self.test__copy_item()
        self.test__rename_item()
        # pre-requirements end here!

        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:
            origin_path = "{}/REPO/projects/testproject_copy2".format(db.root)
            origin = db.dbhandle.getDoc(origin_path).decode('utf-8')

            db._move_item('projects', 'REPO', 'LOCAL',
                          'testproject_copy2')
            path = "{}/LOCAL/projects/testproject_copy2".format(db.root)

            self.assertTrue(db.dbhandle.hasDocument(path))
            self.assertEqual(db.dbhandle.getDoc(path).decode('utf-8'),
                             origin)

    def test_load_item(self):
        # for this test to work certain pre-requirements exist in terms of
        # structures in the data base. We run these first to allow out of
        # order and parallel testing.
        self.test_save_project()
        self.test__copy_item()
        self.test__rename_item()
        self.test__move_item()
        # pre-requirements end here!

        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:
            item = db.load_item('projects', 'LOCAL', 'testproject_copy2')
            itemxml = db._make_xml_if_needed(item)
            self.assertEqual(itemxml.tag, 'test')
            self.assertEqual(itemxml.text, 'foo')

    def test_save_project_conflict(self):
        xml_rep_start = '<test>foo</test>'

        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:

            success, meta = db.save_project('LOCAL', 'testproject2',
                                            xml_rep_start)
            self.assertTrue(success)

            path = "{}/LOCAL/projects/testproject2".format(db.root)
            doc = db._make_xml_if_needed(db.load_item('projects', 'LOCAL',
                                                      'testproject2'))

            doc.text = 'goo'

            doc2 = copy.copy(doc)
            doc2.text = 'hoo'
            doc2.attrib['{http://exist-db.org/versioning}revision'] = str(0)

            success, meta = db.save_project('LOCAL', 'testproject2', doc)
            self.assertTrue(success)

            success, meta = db.save_project('LOCAL', 'testproject2', doc2)

            self.assertTrue(db.dbhandle.hasDocument(path))
            test = db._make_xml_if_needed(db.load_item('projects', 'LOCAL',
                                                       'testproject2'))
            self.assertEqual(test.text, 'goo')
            self.assertFalse(success)
            self.assertTrue('versioning_info' in meta)

            # now overwrite
            success, meta = db.save_project('LOCAL', 'testproject2', doc2,
                                            True)
            self.assertTrue(db.dbhandle.hasDocument(path))
            test = db._make_xml_if_needed(db.load_item('projects', 'LOCAL',
                                                       'testproject2'))
            self.assertEqual(test.text, 'hoo')

    def test_load_multi(self):
        # create a device server and multiple config entries
        xml_reps = ['<test uid="0">foo</test>',
                    '<test uid="1">goo</test>',
                    '<test uid="2">hoo</test>',
                    '<test uid="3">nope</test>']

        with ProjectDatabase(self.user, self.password, server='localhost',
                             test_mode=True) as db:
            for i, rep in enumerate(xml_reps):
                success, meta = db.save_config('LOCAL', 'testconfig{}'
                                               .format(i), rep)
                self.assertTrue(success)

            #we do this twice to have revision info ready
            for i, rep in enumerate(xml_reps):
                success, meta = db.save_config('LOCAL', 'testconfig{}'
                                               .format(i), rep)
                self.assertTrue(success)

            # one is at a different revision
            xml_reps[1] = '<test uid="1">boohoo</test>'
            success, meta = db.save_config('LOCAL', 'testconfig1'
                                           .format(i), xml_reps[1])
            self.assertTrue(success)

            # get version info for what we inserted
            revisions = []
            for i in range(3):
                path = "{}/LOCAL/configs/testconfig{}".format(db.root, i)
                v = db.get_versioning_info(path)
                revisions.append(v['revisions'][-1]['id'])

            xml_serv = "<testserver><configs>"
            for i in range(3):
                xml_serv += '<configuration uid="{}" revision="{}"/>'\
                            .format(i, revisions[i])
            xml_serv += " </configs></testserver>"

            success, meta = db.save_device_server('LOCAL', 'testserver_m',
                                                  xml_serv)
            self.assertTrue(success)

            # now load again
            res = db._load_multi('LOCAL', xml_serv, 'configs')
            for i, r in enumerate(res):
                test_xml = db._make_xml_if_needed(xml_reps[i])
                res_xml = db._make_xml_if_needed(r)
                self.assertEqual(test_xml.text, res_xml.text)
                self.assertEqual(test_xml.attrib['uid'], res_xml.attrib['uid'])
