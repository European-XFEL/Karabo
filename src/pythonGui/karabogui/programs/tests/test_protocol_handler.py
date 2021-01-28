import pytest

from karabogui.programs.protocol_handler import get_cinema_args, parse_url


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

    # Check query for optional username
    queries = {"domain": ["SQS"],
               "scene_uuid": ["xxx-xxx"],
               "username": ["operator"]}
    assert get_cinema_args(queries) == ["SQS", "xxx-xxx",
                                        "--username", "operator"]

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
