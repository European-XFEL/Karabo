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
import datetime
from time import gmtime, strftime, strptime, time

from lxml import etree

from ..bases import DatabaseBase
from ..const import DATE_FORMAT
from ..mysql_db.database import MySQLHandle
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
                # The MySQL back-end doesn't accept project items with no
                # name or no type - originally this test's xml only had 'uuid'.
                xml_rep = (
                    f'<xml uuid="{testproject2}" item_type="device_server" '
                    'simple_name="xyz">foo</xml>')

                meta = db.save_item('LOCAL', testproject2, xml_rep)

                try:
                    path = f"{db.root}/LOCAL/{testproject2}_0"
                    self.assertTrue(db.dbhandle.hasDocument(path))
                    decoded = db.dbhandle.getDoc(path).decode('utf-8')
                    doctree = etree.fromstring(decoded)
                    self.assertEqual(doctree.get('uuid'), testproject2)
                    self.assertEqual(doctree.text, 'foo')
                    self.assertTrue('domain' in meta)
                    self.assertTrue('uuid' in meta)
                except NotImplementedError:
                    # For MySQL, methods like getDoc and hasDocument cannot be
                    # implemented properly. Use load_item instead.
                    res = db.load_item("LOCAL", [testproject2])
                    self.assertEqual(res[0]['uuid'], testproject2)
                    doctree = etree.fromstring(res[0]['xml'])
                    self.assertEqual(doctree.get('item_type'), "device_server")
                    self.assertEqual(doctree.get('simple_name'), "xyz")

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
                    # The MySQL back-end does not save the inner "foo" text of
                    # the original XML as it doesn't map to any item field and
                    # is ignored. The previous assert for itemxml.text == 'foo'
                    # has thus been replaced by a UUID check
                    self.assertTrue(itemxml.attrib['uuid'] in items)

            with self.subTest(msg='test_list_items'):
                create_hierarchy(db)
                items = db.list_items('LOCAL', ['project', 'scene'])
                self.assertEqual(len(items), 5)
                scenecnt = 0
                for i in items:
                    if i["item_type"] == "scene":
                        scenecnt += 1
                    # The contract of "list_items" is to return a
                    # list of items (dictionaries) where
                    # each item would have keys "uuid", "item_type"
                    # and "simple_name" (no mention to "is_trashed")
                    if "is_trashed" in i.keys():
                        self.assertTrue(i["is_trashed"] == 'false')
                self.assertGreaterEqual(scenecnt, 4)

            with self.subTest(msg='test_trashed_projects'):
                create_trashed_project(db)
                items = db.list_items('LOCAL', ['project'])
                # Before the assertion was for number of projects being 2, but
                # for the MySQL not all the save operations are invoked.
                self.assertTrue(len(items) >= 1)
                nb_is_trashed = 0
                for it in items:
                    if it["is_trashed"] == 'true':
                        nb_is_trashed += 1
                self.assertEqual(nb_is_trashed, 1)

            with self.subTest(msg='test_list_domains'):
                items = db.list_domains()
                for it in items:
                    self.assertTrue(it in ['LOCAL_TEST', 'REPO', 'LOCAL'])

            # Revision support has been deprecated and is not implemented for
            # the MySQL back-end
            if type(db.dbhandle) is not MySQLHandle:
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
                # In MySQL, an unattached scene doesn't belong to a project and
                # is not related to any project domain (in ExistDB a domain is
                # , roughly speaking, the directory where a document is, so one
                # can say that an unattached scene still belongs to a domain).
                # For the MySQL case, we use "attached" scenes.
                if type(db.dbhandle) is MySQLHandle:
                    create_hierarchy(db, "Scene!")
                else:
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
                # The MySQL back-end doesn't accept project items with no
                # name or no type - originally this test's xml only had 'uuid'
                # and 'date'
                xml_rep = """
                <xml uuid="{uuid}" date="{date}" simple_name="a_scene"
                item_type="scene">foo</xml>
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
                try:
                    current_date_t = strptime(new_date, DATE_FORMAT)
                except ValueError:
                    # The MySQL back-end sends an UTC date in ISO8601 format
                    # in the metadata returned by save_item.
                    # Try to parse the date using that format.
                    current_date_t = (
                        datetime.datetime.fromisoformat(new_date).timetuple())

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
