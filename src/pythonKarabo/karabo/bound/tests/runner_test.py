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

from ..runner import Runner


def test_argument_parser():
    runner = Runner('someServer')
    cmdLine = [
        'foo',
        'serverId=goo',
        'init={"a.b": "c","x": {"y": 12, "a.c": 1}}'
    ]
    ok, config = runner.parseCommandLine(cmdLine)
    assert ok is True
    assert config.has("serverId")
    assert config.get("serverId") == "goo"
    assert config.has("init")
    initjson = config.get("init")
    assert initjson == '{"a.b": "c","x": {"y": 12, "a.c": 1}}'


def test_argument_parser_initString():
    init_string = """
init={
    "deviceId1": {
        "classId": "TheClassName",
        "stringProperty": "",
        "floatProperty": 42.1,
        "node": {
            "stringProperty": "Value1"
        }
    },
    "deviceId2": {
        "classId": "TheClassName",
        "stringProperty": "1.2.3:14",
        "floatProperty": 42,
        "node": {
            "stringProperty": "Value2"
        }
    }
}
                  """
    cmdLine = ['foo', 'serverId=bingo', init_string]
    runner = Runner('someServer')
    ok, _ = runner.parseCommandLine(cmdLine)
    assert ok
