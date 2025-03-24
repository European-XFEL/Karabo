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
import base64
from datetime import datetime, timezone

from dateutil.parser import isoparse

from karabo.native import Hash, decodeBinary, encodeBinary

ISO8601_FORMAT = '%Y-%m-%dT%H:%M:%S.%f'


class ConfigurationDBError(Exception):
    pass


def hashToBase64Bin(hash):
    """Converts a hash to its binary serialized form encoded in Base64.

    :param hash: the hash to be converted.
    :returns: a string with the base64 encoding of the binary serialized form
        of the hash.
    """
    binHash = encodeBinary(hash)
    b64Bin = base64.b64encode(binHash).decode('utf-8')
    return b64Bin


def hashFromBase64Bin(encodedHash):
    """Retrieves a hash from its binary serialized form encoded in Base64.

    :param encodedHash: the encoded hash.
    :returns: the decoded hash.
    """
    buf = base64.b64decode(encodedHash)
    hash = decodeBinary(buf)
    return hash


def schemaToBase64Bin(sch):
    """Converts a schema to its binary serialized form encoded in Base64.

    :param sch: the schema to be converted.
    :returns: a string with the base64 encoding of the binary serialized form
        of the schema.
    """
    hashWithSch = Hash('sch', sch)
    binSch = encodeBinary(hashWithSch)
    b64Bin = base64.b64encode(binSch).decode('utf-8')
    return b64Bin


def schemaFromBase64Bin(encodedSch):
    """Retrieves a schema from its binary serialized form encoded in Base64.

    :param encodedSch: the encoded schema.
    :returns: the decoded schema.
    """
    buf = base64.b64decode(encodedSch)
    hashWithSch = decodeBinary(buf)
    sch = hashWithSch['sch']
    return sch


def datetime_from_string(date: str) -> datetime:
    """Parses a datetime string in ISO8601 or
    'YYYY-MM-DD HH:mm:SS.Micro' formats."""
    return isoparse(date)


def utc_to_local(timestamp: datetime | None) -> str:
    """Format a datetime object to a local str representation"""
    if timestamp is None:
        return ""
    return timestamp.replace(tzinfo=timezone.utc).astimezone().strftime(
        ISO8601_FORMAT)
