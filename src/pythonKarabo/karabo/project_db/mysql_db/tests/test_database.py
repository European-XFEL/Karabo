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
import unittest
from time import strptime

from lxml import etree

from karabo.project_db.const import DATE_FORMAT
from karabo.project_db.testing import (
    _gen_uuid, create_hierarchy, create_trashed_project)
from karabo.project_db.util import ProjectDBError, make_xml_if_needed

from ..database import ProjectDatabase


class TestMySQLDatabase(unittest.TestCase):
    """ Base test implementation

    All ProjectDB plugins should implement a test that extends this base
    class.
    """

    def stop_local_database(self):
        """Should be implemented by the subclass"""

    def _db_init(self):
        # In test_mode an in-memory SQLite database is used. No need for
        # database connection parameters (e.g. user, db_name).
        return ProjectDatabase(test_mode=True)

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

                db.save_item('LOCAL', testproject2, xml_rep)

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
                items = [testproject2]
                res = db.load_item('LOCAL', items)
                for r in res:
                    itemxml = make_xml_if_needed(r["xml"])
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

            with self.subTest(msg='test_named_items'):
                # In MySQL, an unattached scene doesn't belong to a project and
                # is not related to any project domain (in ExistDB a domain is
                # , roughly speaking, the directory where a document is, so one
                # can say that an unattached scene still belongs to a domain).
                # For the MySQL case, we use "attached" scenes.
                create_hierarchy(db, "Scene!")
                items = db.list_named_items('LOCAL', 'scene', 'Scene!')
                self.assertEqual(len(items), 4)
                scenecnt = 0
                for i in items:
                    if i["item_type"] == "scene":
                        scenecnt += 1
                    self.assertEqual(i["simple_name"], "Scene!")
                self.assertGreaterEqual(scenecnt, 4)

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
