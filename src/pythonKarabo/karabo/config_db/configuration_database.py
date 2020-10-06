from contextlib import ContextDecorator
import hashlib
import os
import sqlite3

from .commands import (
    CREATE_CONFIG_TABLE, CREATE_SCHEMA_TABLE,
    CREATE_INDEX_NAME_TABLE, CREATE_INDEX_PRIORITY_TABLE,
    CREATE_INDEX_DEVICE_ID_TABLE, CREATE_INDEX_SCHEMA_DIGEST_TABLE,
    CMD_GET_CONFIGURATION, CMD_GET_LAST_CONFIGURATION,
    CMD_LIST_NO_NAME, CMD_LIST_NAME,
    CMD_LIST_DEVS_NO_NAME, CMD_LIST_DEVS_NAME,
    CMD_CHECK_NAME_TAKEN, CMD_CHECK_SCHEMA_ALREADY_SAVED,
    CMD_SAVE_CONFIGURATION, CMD_SAVE_CONFIGURATION_NO_TIMESTAMP,
    CMD_SAVE_SCHEMA
)
from .utils import (
    CONFIG_DB_DATA, CONFIG_DB_DEVICE_ID, CONFIG_DB_DESCRIPTION, CONFIG_DB_NAME,
    CONFIG_DB_PRIORITY, CONFIG_DB_SCHEMA, CONFIG_DB_TIMEPOINT, CONFIG_DB_USER
)
from .utils import ConfigurationDBError


class DbHandle(ContextDecorator):
    """The DbHandle class create a configuration database connection

    XXX: Eventual authorization is handled here...
    """

    def __init__(self, dbName):
        self.dbName = dbName
        self.dbConn = None

    def __enter__(self):
        self.dbConn = sqlite3.connect(self.dbName)
        self.dbConn.execute('pragma journal_mode=wal')
        self.dbConn.execute('pragma foreign_key=ON')
        return self.dbConn

    def __exit__(self, type, value, traceback):
        self.dbConn.close()
        self.dbConn = None

    def authenticate(self):
        return True


class ConfigurationDatabase(object):

    def __init__(self, db):
        self.dbHandle = db

    @property
    def path(self):
        return self.dbHandle.dbName

    @property
    def dirname(self):
        return os.path.dirname(self.dbHandle.dbName)

    def assureExisting(self):
        with self.dbHandle as db:
            cursor = db.cursor()
            cursor.execute(CREATE_SCHEMA_TABLE)
            cursor.execute(CREATE_CONFIG_TABLE)
            cursor.execute(CREATE_INDEX_SCHEMA_DIGEST_TABLE)
            cursor.execute(CREATE_INDEX_DEVICE_ID_TABLE)
            cursor.execute(CREATE_INDEX_NAME_TABLE)
            cursor.execute(CREATE_INDEX_PRIORITY_TABLE)

    # Public Interface
    # --------------------------------------------------------------------

    def list_configurations(self, deviceId, name_part=''):
        """Retrieves the set of device configurations related to a name part

        :param deviceId: the id of the device
        :param name_part: the name part of the device configurations. empty
                          means all the named configurations.

        :returns: list of configuration records (can be empty). Each configu-
                  ration record is a dictionary.
        """
        with self.dbHandle as db:
            if name_part:
                name_part = f'%{name_part}%'
                cursor = db.execute(CMD_LIST_NAME, (deviceId, name_part))
            else:
                # When no namePart is given, get all configurations.
                cursor = db.execute(CMD_LIST_NO_NAME, (deviceId,))
            recordings = cursor.fetchall()
            ret_recordings = []

            for rec in recordings:
                conf_dict = {}
                conf_dict[CONFIG_DB_NAME] = rec[0]
                conf_dict[CONFIG_DB_TIMEPOINT] = rec[1]
                conf_dict[CONFIG_DB_DESCRIPTION] = rec[2]
                conf_dict[CONFIG_DB_PRIORITY] = rec[3]
                conf_dict[CONFIG_DB_USER] = rec[4]

                ret_recordings.append(conf_dict)

            return ret_recordings

    def list_devices_configurations(self, deviceIds, name_part=''):
        """Retrieves the set configurations related to a name part for a set of
        devices.

        :param deviceIds: list of deviceIds whose configurations should be
                          listed.
        :param name_part: the name part of the device configurations. empty
                          means all the named configurations.

        :returns: list of configuration records (can be empty). Each configu-
                  ration record is a dictionary.
        """
        if len(deviceIds) == 0:
            raise ConfigurationDBError('Please provide at least one device id')
        with self.dbHandle as db:
            if name_part:
                name_part = f'%{name_part}%'
                cmd = CMD_LIST_DEVS_NAME(len(deviceIds))
                cmd_params = deviceIds.copy()
                cmd_params.append(name_part)
                cursor = db.execute(cmd, cmd_params)
            else:
                # When no namePart is given, get all configurations.
                cmd = CMD_LIST_DEVS_NO_NAME(len(deviceIds))
                cursor = db.execute(cmd, deviceIds)
            recordings = cursor.fetchall()
            ret_recordings = []

            for rec in recordings:
                conf_dict = {}
                conf_dict[CONFIG_DB_DEVICE_ID] = rec[0]
                conf_dict[CONFIG_DB_NAME] = rec[1]
                conf_dict[CONFIG_DB_TIMEPOINT] = rec[2]
                conf_dict[CONFIG_DB_DESCRIPTION] = rec[3]
                conf_dict[CONFIG_DB_PRIORITY] = rec[4]
                conf_dict[CONFIG_DB_USER] = rec[5]

                ret_recordings.append(conf_dict)

            return ret_recordings

    def get_configuration(self, deviceId, name):
        """Retrieves a device configuration given its name.

        :param deviceId: the id of the device.
        :param name: the name of the device configuration

        :returns: dictionary result (can be empty)
        """
        with self.dbHandle as db:
            cursor = db.execute(CMD_GET_CONFIGURATION, (deviceId, name))
            return self._fetch_config(cursor)

    def get_last_configuration(self, deviceId, priority=1):
        """Retrieves the most recent device configuration with a priority.

        :param deviceId: the id of the device.
        :param priority: priority associated to the configuration to return.

        :returns: dictionary result (can be empty)
        """
        with self.dbHandle as db:
            cursor = db.execute(CMD_GET_LAST_CONFIGURATION,
                                (deviceId, int(priority)))

            return self._fetch_config(cursor)

    def save_configuration(self, name, configs, description="", user=".",
                           priority=1, timestamp=""):
        """Saves one or more device configurations (and schemas).

        The configs and schemas are saved for multiple devices and timepoints
        in one roundtrip to the database. They share a common name,
        description, priority and user.

        :param name: the configuration(s) name
        :param configs: a list of dictionaries with the following keys:
                        - 'deviceId',
                        - 'config'
                        - 'schema'

                Here, 'config' and 'schema' are expected to be
                strings with the Base64 encoded form of the binary
                serialization of a device config and its corresponding schema.

        :param description: optional description for the named configuration.

        :param user: The default value, '.', means anonymous.

        :param timestamp: The timestamp to be associated with the device
            configuration. If no timestamp is given, the current UTC timestamp
            with milliseconds precision is used. The timestamp should be UTC
            and specified as a string in ISO8601 format.

        :param priority: Must be a valid between 1 and 3.
        """
        if not 1 <= priority <= 3:
            raise ConfigurationDBError(
                f"Please provide a priority value between 1 and 3.")

        if len(name) >= 80:  # 80 is the size of the underlying db field
            raise ConfigurationDBError(
                f"Please provide a name with less than 80 characters.")

        reqKeys = [CONFIG_DB_DEVICE_ID, CONFIG_DB_DATA, CONFIG_DB_SCHEMA]
        for index, config in enumerate(configs):
            for key in reqKeys:
                if key not in config.keys():
                    raise ConfigurationDBError(
                        f'Configuration #{index} is missing key "{key}".')

        with self.dbHandle as db:
            for config in configs:
                # Checks that the config name isn't already taken for any of
                # the devices.
                deviceId = config[CONFIG_DB_DEVICE_ID]
                if self._name_already_saved(db, name, deviceId):
                    raise ConfigurationDBError(
                        f'Device "{deviceId}" already has a configuration '
                        f'named "{name}".')

            # Starts the transaction and writes each of the configs, committing
            # at the end or rolling back if any error happens.
            db.execute('BEGIN')
            try:
                for config in configs:
                    self._save_config(db, name, config, description, user,
                                      int(priority), timestamp=timestamp)
            except Exception as e:
                db.rollback()
                raise ConfigurationDBError(f"{e}")
            else:
                db.commit()

    # Private Interface
    # --------------------------------------------------------------------

    def _name_already_saved(self, db, name, deviceId):
        """Check if a configuration `name` is already taken from a `deviceId`
        """
        cursor = db.execute(CMD_CHECK_NAME_TAKEN, (name, deviceId))
        rec_with_name = int(cursor.fetchone()[0])

        return rec_with_name > 0

    def _schema_id_from_digest(self, db, schemaDigest):
        """Gets the schema id for a given digest.

        :return: -1 if there's no schema with the given digest in the db;
                 otherwise returns the id of the schema in the database.
        """
        cursor = db.execute(CMD_CHECK_SCHEMA_ALREADY_SAVED, (schemaDigest,))
        schema_id = cursor.fetchone()

        if not schema_id:
            return -1
        else:
            return int(schema_id[0])

    def _save_config(self, db, name, config, description, user, priority,
                     timestamp=""):
        """Save a configuration implementation of the ConfigurationDatabase"""
        schema = config[CONFIG_DB_SCHEMA]
        digest = hashlib.sha1(schema.encode('utf-8')).hexdigest()

        schema_id = self._schema_id_from_digest(db, digest)
        if schema_id == -1:
            db.execute(CMD_SAVE_SCHEMA,
                       (digest, config[CONFIG_DB_SCHEMA]))
            cursor = db.execute('SELECT last_insert_rowid()')
            schema_id = int(cursor.fetchone()[0])

        if len(timestamp) > 0:
            db.execute(CMD_SAVE_CONFIGURATION,
                       (config[CONFIG_DB_DEVICE_ID], name,
                        timestamp, config[CONFIG_DB_DATA], schema_id,
                        user, int(priority), description))
        else:
            db.execute(CMD_SAVE_CONFIGURATION_NO_TIMESTAMP,
                       (config[CONFIG_DB_DEVICE_ID], name,
                        config[CONFIG_DB_DATA], schema_id,
                        user, int(priority), description))

    def _fetch_config(self, cursor):
        """Private method to fetch a single configuration from the database"""
        rec = cursor.fetchone()
        conf_dict = {}
        # XXX: Is rec `None`?
        if rec:
            conf_dict[CONFIG_DB_NAME] = rec[0]
            conf_dict[CONFIG_DB_TIMEPOINT] = rec[1]
            conf_dict[CONFIG_DB_DATA] = rec[2]
            conf_dict[CONFIG_DB_SCHEMA] = rec[3]
            conf_dict[CONFIG_DB_DESCRIPTION] = rec[4]
            conf_dict[CONFIG_DB_PRIORITY] = rec[5]
            conf_dict[CONFIG_DB_USER] = rec[6]

        return conf_dict
