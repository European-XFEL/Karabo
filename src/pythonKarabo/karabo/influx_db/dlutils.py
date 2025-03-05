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
import os.path as op
import re

SEC_TO_USEC = 1000000
USEC_TO_ATTOSEC = 1000000000000


def isValidRawFileName(path):
    """
    Return True if full path is a sensible Karabo DataLogger path
    """
    if not op.isfile(path):
        return False

    name_pattern = re.compile(r"archive_\d+\.txt")
    if not name_pattern.match(op.basename(path)):
        return False

    return True


def device_id_from_path(log_data_path):
    """Extracts the device_id from a given path.

    :param log_data_path: the path which is assumed to be of the form
    '*/karaboHistory/[device_id]/raw' - this is the standard path for
    file-based log files in Karabo.
    :return: the id of the device as extracted from the path.
    :raises RuntimeError: if the path is not in the assumed format.
    """
    dev_id = ""
    if isValidRawFileName(log_data_path):
        dirname = op.dirname(log_data_path)
        # erase dirname from begin to 'karaboHistory/' included
        dirname = dirname[dirname.find('karaboHistory/') +
                          len('karaboHistory/'):]
        dev_id = dirname.rstrip('/raw')
    else:
        raise RuntimeError("invalid raw file path: "
                           "{}".format(log_data_path))
    return dev_id


def format_line_protocol_body(data):
    """Formats a list of strings (or a single string) to conform to
    InfluxDb's line protocol body requirements.

    :param data:either a string or a list of strings to be formatted
    :return:data formatted as a single string conformant to the line
    protocol and ready to be placed in the body of a write request to
    influxdb.
    """
    if isinstance(data, str):
        data = [data]
    data = ('\n'.join(data) + '\n').encode('utf-8')
    return data


def escape_tag_field_key(s):
    """Escapes a string to be used as a tag key, tag value or field key.

    Escaping requirements extracted from
    https://docs.influxdata.com/influxdb/v1.7/write_protocols/line_protocol_reference/
    """
    s = s.replace("\\", "\\\\")
    s = s.replace(",", "\\,")
    s = s.replace("=", "\\=")
    s = s.replace(" ", "\\ ")
    return s


def escape_measurement(s):
    """Escapes a string to be used as a measurement name.

    Escaping requirements extracted from
    https://docs.influxdata.com/influxdb/v1.7/write_protocols/line_protocol_reference/
    """
    s = s.replace(",", "\\,")
    s = s.replace(" ", "\\ ")
    return s
