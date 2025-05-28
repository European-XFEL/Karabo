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
from time import strptime

import pytest
import pytest_asyncio
from lxml import etree

from karabo.native import Hash
from karabo.project_db import (
    DATE_FORMAT, ProjectDBError, SQLDatabase, make_xml_if_needed)
from karabo.project_db.testing import (
    create_hierarchy, create_trashed_project, generate_uuid)


@pytest_asyncio.fixture(loop_scope="module")
async def database():
    database = SQLDatabase(local=True)
    await database.initialize()
    yield database
    await database.delete()


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_project_interface(database, subtests):
    # A bunch of document "names" for the following tests
    testproject = generate_uuid()
    testproject2 = generate_uuid()

    async with database as db:
        with subtests.test(msg='test_add_domain'):
            await db.add_domain("LOCAL_TEST")
            assert await db.domain_exists("LOCAL_TEST")

        with subtests.test(msg='test_save_item'):
            # The MySQL back-end doesn't accept project items with no
            # name or no type - originally this test's xml only had 'uuid'.
            # The '&' in the project name tests if the XML is being properly
            # escaped by the backend while loading items.
            NAME = "xy & z"
            xml_rep = (
                f'<xml uuid="{testproject2}" item_type="device_server" '
                f'simple_name="{NAME.replace("&", "&amp;")}">foo</xml>')

            await db.save_item('LOCAL', testproject2, xml_rep)

            # For MySQL, methods like getDoc and hasDocument cannot be
            # implemented properly. Use load_item instead.
            res = await db.load_item("LOCAL", [testproject2])
            assert res[0]['uuid'] == testproject2
            doctree = etree.fromstring(res[0]['xml'])
            assert doctree.get('item_type') == "device_server"
            assert doctree.get('simple_name') == NAME

        with subtests.test(msg='test_save_bad_item'):
            xml_rep = """
            <xml uuid="{uuid}"><foo|bar/></xml>
            """.format(uuid=testproject)
            with pytest.raises(ProjectDBError):
                await db.save_item('LOCAL',
                                   testproject, xml_rep)

        with subtests.test(msg='load_items'):
            items = [testproject2]
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
                    assert i["is_trashed"] == 'false'
            assert scenecnt >= 4

        with subtests.test(msg='test_find_macro'):
            items = await db.list_items('LOCAL', ['macro'])
            assert len(items) == 4
            items = await db.get_projects_with_macro("NOTTHERE", "macro")
            assert not items

            items = await db.get_projects_with_macro("LOCAL", "macro")
            assert len(items) == 1
            project = items[0]
            assert "projectname" in project
            assert "date" in project
            assert "uuid" in project
            assert "items" in project
            assert project["projectname"] == "Project"
            assert project["items"] == [
                "macroname-0", "macroname-1",
                "macroname-2", "macroname-3"]

            items = await db.get_projects_with_macro("LOCAL", "mAcRo")
            assert len(items) == 1

        with subtests.test(msg='test_find_device'):
            items = await db.get_projects_with_device("LOCAL", "nodevice")
            assert len(items) == 0

            items = await db.get_projects_with_device("LOCAL", "Karabo")
            assert len(items) == 1
            assert len(items[0]["items"]) == 32

            items = await db.get_projects_with_device("LOCAL", "kArabo")
            assert len(items) == 1
            assert len(items[0]["items"]) == 32

        with subtests.test(msg='test_find_server'):
            items = await db.get_projects_with_server("LOCAL", "nosrv")
            assert len(items) == 0

            items = await db.get_projects_with_server("LOCAL", "karabo")
            assert len(items) == 1
            assert len(items[0]["items"]) == 4

            items = await db.get_projects_with_server("LOCAL", "kaRaboCpp")
            assert len(items) == 1
            assert len(items[0]["items"]) == 4

        with subtests.test(msg='test_trashed_projects'):
            await create_trashed_project(db)
            items = await db.list_items('LOCAL', ['project'])
            # Before the assertion was for number of projects being 2, but
            # for the MySQL not all the save operations are invoked.
            assert len(items) >= 1
            nb_is_trashed = 0
            uuid_proj = None
            for it in items:
                if it["is_trashed"] == 'true':
                    nb_is_trashed += 1
                    uuid_proj = it["uuid"]
                    item_type = it["item_type"]
            assert nb_is_trashed == 1
            assert item_type == "project"

            # untrash
            h = Hash("uuid", uuid_proj,
                     "domain", "LOCAL",
                     "item_type", item_type,
                     "value", False)
            await db.update_trashed(**h)
            items = await db.list_items('LOCAL', ['project'])
            nb_is_trashed = len([it for it in items
                                 if it["is_trashed"] == "true"])
            assert nb_is_trashed == 0

        with subtests.test(msg='test_list_domains'):
            items = await db.list_domains()
            for it in items:
                assert it in ['LOCAL_TEST', 'REPO', 'LOCAL']

        with subtests.test(msg='test_named_items'):
            # In MySQL, an unattached scene doesn't belong to a project and
            # is not related to any project domain (in ExistDB a domain is
            # , roughly speaking, the directory where a document is, so one
            # can say that an unattached scene still belongs to a domain).
            # For the MySQL case, we use "attached" scenes.
            await create_hierarchy(db, "Scene!")
            items = await db.list_named_items('LOCAL', 'scene', 'Scene!')
            assert len(items) == 4
            scenecnt = 0
            for i in items:
                if i["item_type"] == "scene":
                    scenecnt += 1
                assert i["simple_name"] == "Scene!"
            assert scenecnt >= 4


@pytest.mark.timeout(60)
@pytest.mark.asyncio
async def test_save_check_modification(database, subtests):
    proj_uuid = generate_uuid()
    async with database as db:
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
