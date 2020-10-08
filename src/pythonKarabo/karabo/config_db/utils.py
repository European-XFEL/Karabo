import base64

from karabo.native import encodeBinary, decodeBinary, Hash

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
