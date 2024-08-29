# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import pytest

from karabogui.programs.protocol_handler import (
    get_cinema_args, get_theatre_args, parse_url)


def test_parse_url():
    # Check URL with a domain and two scene UUIDs
    url = "karabo://cinema?domain=SQS&scene_uuid=xxx-xxx&scene_uuid=yyy-yyy"
    host, queries = parse_url(url)
    assert host == "cinema"
    assert queries == {"domain": ["SQS"],
                       "scene_uuid": ["xxx-xxx", "yyy-yyy"]}

    # Check URL with a domain, a scene UUID and a nosplash flag
    url = "karabo://cinema?domain=SQS&scene_uuid=xxx-xxx&nosplash"
    host, queries = parse_url(url)
    assert host == "cinema"
    assert queries == {"domain": ["SQS"],
                       "scene_uuid": ["xxx-xxx"],
                       "nosplash": ['']}

    # Check URL with a domain, a scene UUID, annd username/host/port
    url = "karabo://cinema?domain=SQS&scene_uuid=xxx-xxx&nosplash"
    host, queries = parse_url(url)
    assert host == "cinema"
    assert queries == {"domain": ["SQS"],
                       "scene_uuid": ["xxx-xxx"],
                       "nosplash": ['']}

    url = ("karabo://cinema?domain=SQS&scene_uuid=xxx-xxx&username=operator"
           "&host=localhost&port=44444")
    host, queries = parse_url(url)
    assert host == "cinema"
    assert queries == {"domain": ["SQS"],
                       "scene_uuid": ["xxx-xxx"],
                       "username": ["operator"],
                       "host": ["localhost"],
                       "port": ["44444"]}

    url = ("karabo://theatre?scene_id=COMPONENT%2FTYPE%2FMEMBER%7CSCENEname"
           "&username=operator&host=localhost&port=44444")
    host, queries = parse_url(url)
    assert host == "theatre"
    assert queries == {"scene_id": ["COMPONENT/TYPE/MEMBER|SCENEname"],
                       "username": ["operator"],
                       "host": ["localhost"],
                       "port": ["44444"]}

    url = ("karabo://theatre?scene_id=COMPONENT%2FTYPE%2FMEMBER%7CSCENEname")
    host, queries = parse_url(url)
    assert host == "theatre"
    assert queries == {"scene_id": ["COMPONENT/TYPE/MEMBER|SCENEname"]}

    url = ("karabo://theatre?scene_id=COMPONENT%2FTYPE%2FMEMBER%7CSCENEname"
           "&scene_id=COMPONENT%2FTYPE%2FMEMBER%7CSCENEname"
           "&username=operator&host=localhost&port=44444")
    host, queries = parse_url(url)
    assert host == "theatre"
    assert queries == {"scene_id": ["COMPONENT/TYPE/MEMBER|SCENEname",
                                    "COMPONENT/TYPE/MEMBER|SCENEname"],
                       "username": ["operator"],
                       "host": ["localhost"],
                       "port": ["44444"]}

    url = ("karabo://theatre?scene_id=COMPONENT%2FTYPE%2FMEMBER%7CSCENEname")
    host, queries = parse_url(url)
    assert host == "theatre"
    assert queries == {"scene_id": ["COMPONENT/TYPE/MEMBER|SCENEname"]}


def test_get_cinema_args():
    # Check query with a domain and two scene UUIDs
    queries = {"domain": ["SQS"],
               "scene_uuid": ["xxx-xxx", "yyy-yyy"]}
    assert get_cinema_args(queries) == ["SQS", "xxx-xxx", "yyy-yyy"]

    # Check query with a domain, a scene UUID and a nosplash flag
    queries = {"domain": ["SQS"],
               "scene_uuid": ["xxx-xxx"],
               "nosplash": ['']}
    assert get_cinema_args(queries) == ["SQS", "xxx-xxx", "--nosplash"]

    # Check query with two domains and a scene UUID
    queries = {"domain": ["SQS", "SCS"],
               "scene_uuid": ["xxx-xxx"]}
    with pytest.raises(ValueError):
        get_cinema_args(queries)

    # Check query with one domain and no scene UUID
    queries = {"domain": ["SQS"]}
    with pytest.raises(ValueError):
        get_cinema_args(queries)

    # Check query with blank domains and a scene UUID
    queries = {"domain": [''],
               "scene_uuid": ["xxx-xxx"]}
    with pytest.raises(ValueError):
        get_cinema_args(queries)

    # Check query for optional but both supplied host and port
    queries = {"domain": ["SQS"],
               "scene_uuid": ["xxx-xxx"],
               "host": ["localhost"],
               "port": ["44444"]}
    assert get_cinema_args(queries) == ["SQS", "xxx-xxx",
                                        "--host", "localhost",
                                        "--port", "44444"]

    # Check query for optional but only host is supplied
    queries = {"domain": ["SQS"],
               "scene_uuid": ["xxx-xxx"],
               "host": ["localhost"]}
    assert get_cinema_args(queries) == ["SQS", "xxx-xxx"]

    # Check query for optional but only port is supplied
    queries = {"domain": ["SQS"],
               "scene_uuid": ["xxx-xxx"],
               "port": ["44444"]}
    assert get_cinema_args(queries) == ["SQS", "xxx-xxx"]


def test_get_theatre_args():
    queries = {"scene_id": ["COMPONENT/TYPE/MEMBER|SCENEname"],
               "host": ["localhost"],
               "port": ["44444"]}

    assert get_theatre_args(queries) == ["COMPONENT/TYPE/MEMBER|SCENEname",
                                         "--host", "localhost",
                                         "--port", "44444"]

    queries = {"scene_id": ["COMPONENT/TYPE/MEMBER|SCENEname"]}

    assert get_theatre_args(queries) == ["COMPONENT/TYPE/MEMBER|SCENEname"]

    queries = {"scene_id": ["bla", "bli"],
               "host": ["localhost"],
               "port": ["44444"]}

    assert get_theatre_args(queries) == ["bla", "bli",
                                         "--host", "localhost",
                                         "--port", "44444"]
