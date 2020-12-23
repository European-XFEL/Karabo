from contextlib import ContextDecorator
import hashlib
import os
import sqlite3

from .commands import (
    CREATE_CONFIG_TABLE, CREATE_SCHEMA_TABLE, CREATE_CONFIG_SET_TABLE,
    CREATE_INDEX_NAME_TABLE, CREATE_INDEX_PRIORITY_TABLE,
    CREATE_INDEX_DEVICE_ID_TABLE, CREATE_INDEX_SCHEMA_DIGEST_TABLE,
    CREATE_INDEX_DEVICE_SET_ID_TABLE, CMD_GET_CONFIGURATION,
    CMD_GET_LAST_CONFIGURATION, CMD_LIST_NO_NAME, CMD_LIST_NAME,
    CMD_LIST_DEVS_NO_NAME, CMD_CHECK_NAME_TAKEN_ANY_DEVICE,
    CMD_CHECK_SCHEMA_ALREADY_SAVED, CMD_INSERT_CONFIGURATION,
    CMD_INSERT_CONFIGURATION_NO_TIMESTAMP, CMD_INSERT_SCHEMA,
    CMD_INSERT_CONFIG_SET, CMD_UPDATE_CONFIG_SET,
    CMD_UPDATE_CONFIGURATION_NO_TIMESTAMP, CMD_UPDATE_CONFIGURATION,
    CMD_CHECK_DEVICE_LIMIT, CMD_LIST_DEVICES_PRIORITY,
    CMD_GET_OVERWRITABLE_CONFIG_FOR_SET)
from .utils import (
    CONFIG_DB_DATA, CONFIG_DB_DEVICE_ID, CONFIG_DB_DESCRIPTION, CONFIG_DB_NAME,
    CONFIG_DB_PRIORITY, CONFIG_DB_SCHEMA, CONFIG_DB_TIMEPOINT, CONFIG_DB_USER,
    CONFIG_DB_MIN_TIMEPOINT, CONFIG_DB_MAX_TIMEPOINT, CONFIG_DB_DIFF_TIMEPOINT,
    CONFIG_DB_OVERWRITABLE, create_config_set_id
)
from .utils import ConfigurationDBError

CONFIGURATION_LIMIT = 300


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
        if self.dbConn is not None:
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
            cursor.execute(CREATE_CONFIG_SET_TABLE)
            cursor.execute(CREATE_CONFIG_TABLE)
            cursor.execute(CREATE_INDEX_SCHEMA_DIGEST_TABLE)
            cursor.execute(CREATE_INDEX_DEVICE_ID_TABLE)
            cursor.execute(CREATE_INDEX_NAME_TABLE)
            cursor.execute(CREATE_INDEX_PRIORITY_TABLE)
            cursor.execute(CREATE_INDEX_DEVICE_SET_ID_TABLE)

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
                conf_dict[CONFIG_DB_OVERWRITABLE] = rec[5]

                ret_recordings.append(conf_dict)

            return ret_recordings

    def list_devices(self, priority=3):
        """Retrieves the set of deviceIds related to configuration priority

        :param priority: the desired priority

        :returns: list of deviceIds records (can be empty).
        """
        with self.dbHandle as db:
            cursor = db.execute(CMD_LIST_DEVICES_PRIORITY, (int(priority),))
            recordings = cursor.fetchall()
            ret = [rec[0] for rec in recordings]
            return ret

    def list_configuration_sets(self, deviceIds):
        """Retrieves the set of configurations related to a list of devices.

        :param deviceIds: list of deviceIds whose configurations should be
                          listed.

        Example: If deviceA has configurations Alice and Bob ...
                 DeviceB has configurations Bob ...
                 Only Bob will be returned in this query and only if Bob
                 been saved in a single save operation for both devices A and B
                 (in this case, config Bob will be associated to the same
                 Configuration Set for both devices A and B).

        :returns: list of configuration records (can be empty). Each configu-
                  ration record is a dictionary.
        """
        if len(deviceIds) == 0:
            raise ConfigurationDBError('Please provide at least one device id')
        with self.dbHandle as db:
            cmd = CMD_LIST_DEVS_NO_NAME(len(deviceIds))
            cursor = db.execute(cmd, deviceIds)
            recordings = cursor.fetchall()
            ret_recordings = []

            for rec in recordings:
                conf_dict = {}
                conf_dict[CONFIG_DB_NAME] = rec[1]
                conf_dict[CONFIG_DB_DESCRIPTION] = rec[2]
                conf_dict[CONFIG_DB_PRIORITY] = rec[3]
                conf_dict[CONFIG_DB_USER] = rec[4]
                conf_dict[CONFIG_DB_OVERWRITABLE] = rec[5]
                conf_dict[CONFIG_DB_MIN_TIMEPOINT] = rec[6]
                conf_dict[CONFIG_DB_MAX_TIMEPOINT] = rec[7]
                conf_dict[CONFIG_DB_DIFF_TIMEPOINT] = rec[8]
                ret_recordings.append(conf_dict)

            return ret_recordings

    def check_configurations_limits(self, deviceIds):
        """Checks if any of the devices exceeds the configuration limit.

        :param deviceIds: list of deviceIds who should be validated

        :returns: boolean (True or False)
        """
        with self.dbHandle as db:
            cmd = CMD_CHECK_DEVICE_LIMIT(len(deviceIds))
            params = list(deviceIds)
            params.append(CONFIGURATION_LIMIT)
            cursor = db.execute(cmd, params)
            rec_with_name = int(cursor.fetchone()[0])
            return rec_with_name > 0

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
                           priority=1, timestamp="", overwritable=False,
                           setIdDigest=None):
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

        :param priority: Must be an integer between 1 and 3.

        :param timestamp: The timestamp to be associated with the device
            configuration. If no timestamp is given, the current UTC timestamp
            with milliseconds precision is used. The timestamp should be UTC
            and specified as a string in ISO8601 format.

        :param overwritable: Will the configuration be overwritable?
            Ignored if the named configuration already exists in the database
            for the set of devices in configs.

        :param setIdDigest: the digest to be used for the set of devices whose
            configurations are being saved. If no value is provided, a digest
            is generated internally for the set of devices in the method
            argeuments.
        """
        if not 1 <= priority <= 3:
            raise ConfigurationDBError(
                "Please provide a priority value between 1 and 3.")

        if len(name) >= 80:  # 80 is the size of the underlying db field
            raise ConfigurationDBError(
                "Please provide a name with less than 80 characters.")

        reqKeys = [CONFIG_DB_DEVICE_ID, CONFIG_DB_DATA, CONFIG_DB_SCHEMA]
        for index, config in enumerate(configs):
            for key in reqKeys:
                if key not in config.keys():
                    raise ConfigurationDBError(
                        f'Configuration #{index} is missing key "{key}".')

        deviceIds = [conf[CONFIG_DB_DEVICE_ID] for conf in configs]

        if self.is_config_name_taken(name, deviceIds):
            raise ConfigurationDBError(
                f"The config name {name} is already "
                f"taken for at least one of the device(s) {deviceIds}")

        set_digest = (
            setIdDigest if setIdDigest else create_config_set_id(deviceIds)
        )

        with self.dbHandle as db:
            overwrite_id = self._get_overwrite_id(
                db, name, set_digest)
            db.execute('BEGIN')
            try:
                if overwrite_id:
                    # An overwritable configuration with the name exists for
                    # the set of devices; update it.
                    self._update_configs(
                        db, overwrite_id[0], configs, description,
                        user, int(priority), timestamp=timestamp)
                else:
                    # No configuration with the name exists for the set of
                    # devices; Add it after checking that the limit of
                    # configurations per device stored in the database will
                    # be respected.
                    is_limit = self.check_configurations_limits(deviceIds)
                    if is_limit:
                        raise ConfigurationDBError(
                            f'One of the devices "{deviceIds}" would exceed '
                            f'the number  of configurations per device, '
                            f'{CONFIGURATION_LIMIT}.')
                    self._insert_configs(
                        db, set_digest, name, configs, description,
                        user, int(priority), timestamp=timestamp,
                        overwritable=overwritable)
            except Exception as e:
                db.rollback()
                raise ConfigurationDBError(f"{e}")
            else:
                db.commit()

    def is_config_name_taken(self, name, deviceIds):
        """Checks if a config name is already in use by any of the devices in a
        list.

        :param  name: the configuration name to be checked.
        :param deviceIds: list of deviceIds whose taken configurations names
                          should be searched.

        :returns: True if the config name is already taken by any of the
                  devices in the list.
        """
        with self.dbHandle as db:
            cmd = CMD_CHECK_NAME_TAKEN_ANY_DEVICE(len(deviceIds))
            params = [name]
            params.extend(deviceIds)
            cursor = db.execute(cmd, params)
            rec_with_name = int(cursor.fetchone()[0])
            return rec_with_name > 0

    # Private Interface
    # --------------------------------------------------------------------

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

    def _get_overwrite_id(self, db,  name, set_id):
        """Gets an overwritable configuration for a given set of devices
           from the database.

        :param name: the name of the overwritable configuration.
        :param set_id: the set identifier (unique hash for each set of devices)
        :return: the internal identifier of the overwritable config set, or
                 None if no config set is found.
        """
        cursor = db.execute(
                    CMD_GET_OVERWRITABLE_CONFIG_FOR_SET,
                    (set_id, name))
        return cursor.fetchone()

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
            conf_dict[CONFIG_DB_OVERWRITABLE] = rec[7]

        return conf_dict

    def _update_configs(self, db, set_id, configs, description, user,
                        priority, timestamp):
        """Updates overwritable device configurations in the database after
           updating the corresponding device set record."""
        # Starts the transaction and writes each of the configs,
        # committing at the end or rolling back if any error happens.
        db.execute(CMD_UPDATE_CONFIG_SET,
                   (user, priority, description, set_id))
        for config in configs:
            self._update_config(db, set_id, config, timestamp)

    def _update_config(self, db, set_id, config, timestamp):
        """Updates a device configuration in the database.

        NOTE: It's up to the caller to guarantee that the set_id refers to an
              overwritable configuration set.
        """
        schema_id = self._insert_schema_if_needed(db, config[CONFIG_DB_SCHEMA])

        if len(timestamp) > 0:
            db.execute(CMD_UPDATE_CONFIGURATION,
                       (config[CONFIG_DB_DATA], schema_id,
                        timestamp, set_id, config[CONFIG_DB_DEVICE_ID]))
        else:
            db.execute(CMD_UPDATE_CONFIGURATION_NO_TIMESTAMP,
                       (config[CONFIG_DB_DATA], schema_id,
                        set_id, config[CONFIG_DB_DEVICE_ID]))

    def _insert_configs(self, db, set_digest, name, configs, description,
                        user, priority, timestamp, overwritable):
        """Saves new device configurations in the database after saving the
           corresponding device set record."""
        # Starts the transaction and writes each of the configs,
        # committing at the end or rolling back if any error happens.
        db.execute(CMD_INSERT_CONFIG_SET,
                   (set_digest, name, user, priority,
                    description, overwritable))
        cursor = db.execute('SELECT last_insert_rowid()')
        set_id = int(cursor.fetchone()[0])
        for config in configs:
            self._insert_config(db, set_id, config, timestamp)

    def _insert_config(self, db, set_id, config, timestamp):
        """Saves a new device configuration in the database."""
        schema_id = self._insert_schema_if_needed(db, config[CONFIG_DB_SCHEMA])
        if len(timestamp) > 0:
            db.execute(CMD_INSERT_CONFIGURATION,
                       (set_id, config[CONFIG_DB_DEVICE_ID],
                        timestamp, config[CONFIG_DB_DATA], schema_id))
        else:
            db.execute(CMD_INSERT_CONFIGURATION_NO_TIMESTAMP,
                       (set_id, config[CONFIG_DB_DEVICE_ID],
                        config[CONFIG_DB_DATA], schema_id))

    def _insert_schema_if_needed(self, db, schema_data):
        """Inserts an schema if it's digest reveals it's not stored yet.

           :param db: handle to an open database connection.
           :param schema_data: serialized form of the schema to store.
           :return: id of the existing or just created schema record.
        """
        digest = hashlib.sha1(schema_data.encode('utf-8')).hexdigest()
        schema_id = self._schema_id_from_digest(db, digest)
        if schema_id == -1:
            db.execute(CMD_INSERT_SCHEMA,
                       (digest, schema_data))
            cursor = db.execute('SELECT last_insert_rowid()')
            schema_id = int(cursor.fetchone()[0])
        return schema_id
