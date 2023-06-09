# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from time import gmtime, strftime, strptime, time

from lxml import etree

from ..bases import DatabaseBase
from ..const import DATE_FORMAT
from ..util import ProjectDBError
from .util import _gen_uuid, create_hierarchy


def create_trashed_project(db, is_trashed=True):
    uuid = _gen_uuid()
    xml = (f'<xml item_type="project" uuid="{uuid}"'
           f' simple_name="{uuid}" is_trashed="{str(is_trashed).lower()}">'
           '</xml>')
    db.save_item("LOCAL", uuid, xml)
    return uuid


def create_unattached_scenes(db):
    # create scenes with the same simple_name
    # unattached to the project
    for i in range(4):
        sub_uuid = _gen_uuid()
        scene_xml = ('<xml item_type="scene" uuid="{uuid}"'
                     ' simple_name="Scene!" >中文</xml>'
                     .format(uuid=sub_uuid))

        db.save_item("LOCAL", sub_uuid, scene_xml)


class ProjectDatabaseVerification():
    """ Base test implementation

    All ProjectDB plugins should implement a test that extends this base
    class.
    """

    def _db_init(self):
        """Should be implemented by the subclass"""

    def stop_local_database(self):
        """Should be implemented by the subclass"""

    def test__make_xml_if_needed(self):
        xml_rep = "<test>foo</test>"
        ret = DatabaseBase._make_xml_if_needed(xml_rep)
        self.assertEqual(ret.tag, "test")
        self.assertEqual(ret.text, "foo")

    def test_malformed_xml_inputs(self):
        xml_rep = "<test>foo</test><bad|symbols></bad|symbols>"
        self.assertRaises(ValueError, DatabaseBase._make_xml_if_needed,
                          xml_rep)

    def test__make_str_if_needed(self):
        element = etree.Element('test')
        element.text = 'foo'
        str_rep = DatabaseBase._make_str_if_needed(element)
        self.assertEqual(str_rep, "<test>foo</test>\n")

    def test_project_interface(self):
        # A bunch of document "names" for the following tests
        testproject = _gen_uuid()
        testproject2 = _gen_uuid()
        with self._db_init() as db:
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

                path = f"{db.root}/LOCAL/{testproject2}_0"
                self.assertTrue(db.dbhandle.hasDocument(path))
                decoded = db.dbhandle.getDoc(path).decode('utf-8')
                doctree = etree.fromstring(decoded)
                self.assertEqual(doctree.get('uuid'), testproject2)
                self.assertEqual(doctree.text, 'foo')
                self.assertTrue('domain' in meta)
                self.assertTrue('uuid' in meta)

            with self.subTest(msg='test_save_bad_item'):
                xml_rep = """
                <xml uuid="{uuid}"><foo|bar/></xml>
                """.format(uuid=testproject)
                self.assertRaises(ProjectDBError, db.save_item, 'LOCAL',
                                  testproject, xml_rep)

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
                    self.assertTrue(i["is_trashed"] == 'false')
                self.assertGreaterEqual(scenecnt, 4)

            with self.subTest(msg='test_trashed_projects'):
                create_trashed_project(db)
                items = db.list_items('LOCAL', ['project'])
                self.assertEqual(len(items), 2)
                nb_is_trashed = 0
                for it in items:
                    if it["is_trashed"] == 'true':
                        nb_is_trashed += 1
                self.assertEqual(nb_is_trashed, 1)

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
                    date = strftime(DATE_FORMAT, gmtime(earlier + i))
                    xml = xml_rep.format(uuid=versioned_uuid, revision=i,
                                         date=date)
                    path = "{}/{}/{}_{}".format(db.root, 'LOCAL',
                                                versioned_uuid, i)
                    # Save directly without engaging save_item()
                    db.dbhandle.load(xml.encode('utf-8'), path)

                # Read it back, no revision given
                res = db.load_item('LOCAL', [versioned_uuid])
                self.assertEqual(len(res), 1)
                itemxml = db._make_xml_if_needed(res[0]['xml'])
                self.assertEqual(int(itemxml.get('revision')), MAX_REV)

                # Write it back with save_item
                date = strftime(DATE_FORMAT, gmtime())
                xml_rep = """
                <xml uuid="{uuid}" date="{date}" user="sue">foo</xml>
                """.format(uuid=versioned_uuid, date=date)
                meta = db.save_item('LOCAL', versioned_uuid, xml_rep)

                # Make sure there's a revision of 0 (auto-added)
                res = db.load_item('LOCAL', [versioned_uuid])
                itemxml = db._make_xml_if_needed(res[0]['xml'])
                self.assertEqual(itemxml.get('revision'), '0')

            with self.subTest(msg='test_named_items'):
                create_unattached_scenes(db)
                items = db.list_named_items('LOCAL', 'scene', 'Scene!')
                self.assertEqual(len(items), 4)
                scenecnt = 0
                for i in items:
                    if i["item_type"] == "scene":
                        scenecnt += 1
                    self.assertEqual(i["simple_name"], "Scene!")
                self.assertGreaterEqual(scenecnt, 4)

            with self.subTest(msg='test_get_projects_and_configs'):
                _, device_id_conf_map = create_hierarchy(db)
                for device_id, conf_id in device_id_conf_map.items():
                    items = db.get_projects_with_conf("LOCAL", device_id)
                    proj, conf = next(iter(items.items()))
                    self.assertEqual(proj, "Project")
                    self.assertEqual(conf_id, conf)

            self.stop_local_database()

    def test_save_check_modification(self):
        proj_uuid = _gen_uuid()

        with self._db_init() as db:

            path = "{}/{}".format(db.root, 'LOCAL')
            if db.dbhandle.hasCollection(path):
                db.dbhandle.removeCollection(path)

            # make sure we have the LOCAL collection and subcollections
            path = "{}/{}".format(db.root, 'LOCAL')
            if not db.dbhandle.hasCollection(path):
                db.dbhandle.createCollection(path)

            with self.subTest(msg='test_save_item'):
                date = "2011-11-01 09:00:52"
                xml_rep = """
                <xml uuid="{uuid}" date="{date}">foo</xml>
                """.format(uuid=proj_uuid, date=date)

                success = True
                try:
                    meta = db.save_item('LOCAL', proj_uuid, xml_rep)
                except ProjectDBError:
                    success = False

                self.assertTrue(success)
                self.assertTrue('date' in meta)
                new_date = meta['date']
                self.assertNotEqual(new_date, date)
                date_t = strptime(date, DATE_FORMAT)
                current_date_t = strptime(new_date, DATE_FORMAT)
                self.assertLess(date_t, current_date_t)

                # Try saving again but using same time stamp
                success = True
                reason = ""
                try:
                    meta = db.save_item('LOCAL', proj_uuid, xml_rep)
                except ProjectDBError as e:
                    success = False
                    reason = str(e)
                self.assertFalse(success)
                self.assertTrue(reason)

            self.stop_local_database()
