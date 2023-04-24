# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import base64
import hashlib
from datetime import datetime

from karabo.native import Hash, decodeBinary, encodeBinary

CONFIG_DB_DEVICE_ID = "deviceId"
CONFIG_DB_NAME = "name"
CONFIG_DB_DATA = "config"
CONFIG_DB_DESCRIPTION = "description"
CONFIG_DB_PRIORITY = "priority"
CONFIG_DB_SCHEMA = "schema"
CONFIG_DB_MIN_TIMEPOINT = "min_timepoint"
CONFIG_DB_MAX_TIMEPOINT = "max_timepoint"
CONFIG_DB_DIFF_TIMEPOINT = "diff_timepoint"
CONFIG_DB_TIMEPOINT = "timepoint"
CONFIG_DB_USER = "user"
CONFIG_DB_OVERWRITABLE = "overwritable"
CONFIG_DB_CONFIG_SET_ID = "config_set_id"

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
    # TODO: Fix the schema serialization in the middlelayer-api and get rid of
    #       the "hack" of piggy-backing on the hash serialization.
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


def create_config_set_id(deviceIds):
    """Generates a setId that is unique for a given set of devices.

    :param deviceIds: list of deviceIds whose setId should be generated.

    :return: a sha1 hash that is unique for a given set of devices.
    """
    assert(type(deviceIds) is list)
    uniques = sorted(set([devId.upper() for devId in deviceIds]))
    ids_str = '::'.join(uniques)
    return hashlib.sha1(ids_str.encode('utf-8')).hexdigest()


def datetime_from_string(str_date):
    """Obtains the datetime object equivalent to a string that is assumed to
    be in either ISO8601 or 'YYYY-MM-DD HH:mm:SS.Micro' formats.

    An exception will be thrown if the string isn't in any of the expected
    formats.
    """
    if str_date.find('T') >= 0:
        return datetime.strptime(str_date, ISO8601_FORMAT)
    else:
        return datetime.strptime(str_date, '%Y-%m-%d %H:%M:%S.%f')
