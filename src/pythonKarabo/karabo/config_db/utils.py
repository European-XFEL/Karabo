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
from datetime import UTC, datetime

from karabo.native import Hash, decodeBinary, encodeBinary

DATE_FORMAT = "%Y-%m-%d %H:%M:%S"


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


def datetime_now() -> datetime:
    """Return a datetime object in utc without microseconds"""
    return datetime.now(UTC).replace(microsecond=0)


def datetime_str_now() -> str:
    """Return a datetime string for now for config db format"""
    return datetime_to_str(datetime.now(UTC))


def datetime_from_str(timestamp: str) -> datetime:
    """Parses a datetime string in date formats."""
    return datetime.strptime(timestamp, DATE_FORMAT).replace(tzinfo=UTC)


def datetime_to_str(timestamp: datetime | None) -> str:
    """Format a datetime object to a local str representation"""
    if timestamp is None:
        return ""
    return timestamp.replace(tzinfo=UTC).strftime(DATE_FORMAT)
