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

from karabind import Hash, generateAutoStartHash, jsonToHash


def test_init_string_to_auto_start_hash_conversion():

    init_string_json = """
{
    "schema_printer1": {
        "classId": "SchemaPrinter"
    },
    "data_logger_manager_1": {
        "classId": "DataLoggerManager",
        "serverList": "karabo/dataLogger"
    }
}
    """

    expected_auto_start_hash = Hash('autoStart', [
        Hash(
            'DataLoggerManager',
            Hash('deviceId', 'data_logger_manager_1', 'serverList',
                 'karabo/dataLogger')),
        Hash('SchemaPrinter', Hash('deviceId', 'schema_printer1'))
    ])

    assert expected_auto_start_hash == generateAutoStartHash(
        jsonToHash(init_string_json))
