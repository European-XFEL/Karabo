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

import pytest
from lxml import etree

from karabo.project_db import (
    DATE_FORMAT, ExistDatabase, ProjectDBError, make_xml_if_needed)
from karabo.project_db.exist_db import TESTDB_ADMIN_PASSWORD
from karabo.project_db.testing import (
    create_hierarchy, create_trashed_project, create_unattached_scenes,
    generate_uuid)

from .util import stop_local_database


@pytest.fixture(scope="function")
def database():
    user = "admin"
    password = TESTDB_ADMIN_PASSWORD
    yield ExistDatabase(user, password,
                        test_mode=True, init_db=True)
    stop_local_database()


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_project_interface(database, subtests):
    # A bunch of document "names" for the following tests
    testproject = generate_uuid()
    testproject2 = generate_uuid()
    async with database as db:
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

        with subtests.test(msg='test_domain_exists'):
            assert db.domain_exists("LOCAL")

        with subtests.test(msg='test_add_domain'):
            db.add_domain("LOCAL_TEST")
            assert db.domain_exists("LOCAL_TEST")

        with subtests.test(msg='test_save_item'):
            # The MySQL back-end doesn't accept project items with no
            # name or no type - originally this test's xml only had 'uuid'.
            xml_rep = (
                f'<xml uuid="{testproject2}" item_type="device_server" '
                'simple_name="xyz">foo</xml>')

            meta = await db.save_item('LOCAL', testproject2, xml_rep)
            path = f"{db.root}/LOCAL/{testproject2}_0"
            assert db.dbhandle.hasDocument(path)
            decoded = db.dbhandle.getDoc(path).decode('utf-8')
            doctree = etree.fromstring(decoded)
            assert doctree.get('uuid') == testproject2
            assert doctree.text == 'foo'
            assert 'domain' in meta
            assert 'uuid' in meta

        with subtests.test(msg='test_save_bad_item'):
            xml_rep = """
            <xml uuid="{uuid}"><foo|bar/></xml>
            """.format(uuid=testproject)
            with pytest.raises(ProjectDBError):
                await db.save_item('LOCAL', testproject, xml_rep)

        with subtests.test(msg='load_items'):
            items = [testproject, testproject2]
            res = await db.load_item('LOCAL', items)
            for r in res:
                itemxml = make_xml_if_needed(r["xml"])
                assert itemxml.tag == 'xml'
                # The MySQL back-end does not save the inner "foo" text of
                # the original XML as it doesn't map to any item field and
                # is ignored. The previous assert for itemxml.text == 'foo'
                # has thus been replaced by a UUID check
                assert itemxml.attrib['uuid'] in items

        with subtests.test(msg='test_list_items'):
            await create_hierarchy(db)
            items = await db.list_items('LOCAL', ['project', 'scene'])
            assert len(items) == 5
            scenecnt = 0
            for i in items:
                if i["item_type"] == "scene":
                    scenecnt += 1
                # The contract of "list_items" is to return a
                # list of items (dictionaries) where
                # each item would have keys "uuid", "item_type"
                # and "simple_name" (no mention to "is_trashed")
                if "is_trashed" in i.keys():
                    assert i["is_trashed"] is False
            assert scenecnt >= 4

        with subtests.test(msg='test_trashed_projects'):
            await create_trashed_project(db)
            items = await db.list_items('LOCAL', ['project'])
            # Before the assertion was for number of projects being 2, but
            # for the MySQL not all the save operations are invoked.
            assert len(items) >= 1
            nb_is_trashed = 0
            for it in items:
                if it["is_trashed"] is True:
                    nb_is_trashed += 1
            assert nb_is_trashed == 1

        with subtests.test(msg='test_list_domains'):
            items = await db.list_domains()
            for it in items:
                assert it in ['LOCAL_TEST', 'REPO', 'LOCAL']

        with subtests.test(msg='test_old_versioned_data'):
            versioned_uuid = generate_uuid()
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
                path = "{}/{}/{}_{}".format(
                    db.root, 'LOCAL', versioned_uuid, i)
                # Save directly without engaging save_item()
                db.dbhandle.load(xml.encode('utf-8'), path)

            # Read it back, no revision given
            res = await db.load_item('LOCAL', [versioned_uuid])
            assert len(res) == 1
            itemxml = make_xml_if_needed(res[0]['xml'])
            assert int(itemxml.get('revision')) == MAX_REV

            # Write it back with save_item
            date = strftime(DATE_FORMAT, gmtime())
            xml_rep = """
            <xml uuid="{uuid}" date="{date}" user="sue">foo</xml>
            """.format(uuid=versioned_uuid, date=date)
            meta = await db.save_item('LOCAL', versioned_uuid, xml_rep)

            # Make sure there's a revision of 0 (auto-added)
            res = await db.load_item('LOCAL', [versioned_uuid])
            itemxml = make_xml_if_needed(res[0]['xml'])
            assert itemxml.get('revision') == '0'

        with subtests.test(msg='test_named_items'):
            await create_unattached_scenes(db)
            items = await db.list_named_items(
                'LOCAL', 'scene', 'Scene!')
            assert len(items) == 4
            scenecnt = 0
            for i in items:
                if i["item_type"] == "scene":
                    scenecnt += 1
                assert i["simple_name"] == "Scene!"
            assert scenecnt >= 4

        stop_local_database()


@pytest.mark.timeout(30)
@pytest.mark.asyncio
async def test_save_check_modification(database, subtests):
    proj_uuid = generate_uuid()

    async with database as db:
        path = "{}/{}".format(db.root, 'LOCAL')
        if db.dbhandle.hasCollection(path):
            db.dbhandle.removeCollection(path)

        # make sure we have the LOCAL collection and subcollections
        path = "{}/{}".format(db.root, 'LOCAL')
        if not db.dbhandle.hasCollection(path):
            db.dbhandle.createCollection(path)

        with subtests.test(msg='test_save_item'):
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
                meta = await db.save_item('LOCAL', proj_uuid, xml_rep)
            except ProjectDBError:
                success = False

            assert success
            assert 'date' in meta
            new_date = meta['date']
            assert new_date != date
            date_t = strptime(date, DATE_FORMAT)
            try:
                current_date_t = strptime(new_date, DATE_FORMAT)
            except ValueError:
                # The MySQL back-end sends an UTC date in ISO8601 format
                # in the metadata returned by save_item.
                # Try to parse the date using that format.
                # TODO: FIX
                current_date_t = (
                    datetime.datetime.fromisoformat(new_date).timetuple())

            assert date_t < current_date_t

            # Try saving again but using same time stamp
            success = True
            reason = ""
            try:
                meta = await db.save_item('LOCAL', proj_uuid, xml_rep)
            except ProjectDBError as e:
                success = False
                reason = str(e)
            assert not success
            assert reason
