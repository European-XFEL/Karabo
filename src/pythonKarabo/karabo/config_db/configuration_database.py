# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import hashlib
import os
from contextlib import ContextDecorator
from datetime import datetime

from peewee import DoesNotExist, SqliteDatabase, fn

from .models import ConfigSchema, ConfigSet, DeviceConfig
from .utils import (
    CONFIG_DB_CONFIG_SET_ID, CONFIG_DB_DATA, CONFIG_DB_DESCRIPTION,
    CONFIG_DB_DEVICE_ID, CONFIG_DB_DIFF_TIMEPOINT, CONFIG_DB_MAX_TIMEPOINT,
    CONFIG_DB_MIN_TIMEPOINT, CONFIG_DB_NAME, CONFIG_DB_OVERWRITABLE,
    CONFIG_DB_PRIORITY, CONFIG_DB_SCHEMA, CONFIG_DB_TIMEPOINT, CONFIG_DB_USER,
    ISO8601_FORMAT, ConfigurationDBError, create_config_set_id,
    datetime_from_string)

CONFIGURATION_LIMIT = 300


class DbHandle(ContextDecorator):
    """The DbHandle class create a configuration database connection

    XXX: Eventual authorization is handled here...
    """

    def __init__(self, dbName):
        self.dbName = dbName
        self.dbConn = SqliteDatabase(
            self.dbName,
            pragmas={
                'journal_mode': 'wal',
                'foreign_key': 1,
                'ignore_check_constraints': 0
            }
        )

    def __enter__(self):
        self.dbConn.bind(
            [ConfigSet, ConfigSchema, DeviceConfig]
        )
        self.dbConn.connect(reuse_if_open=True)
        return self.dbConn

    def __exit__(self, type, value, traceback):
        self.dbConn.close()

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
            db.create_tables(
                [ConfigSchema, ConfigSet, DeviceConfig],
                safe=True
            )

    # Public Interface
    # --------------------------------------------------------------------

    # TODO: ponder with others and implement, depending on the results of the
    # pondering process, methods to retrieve all devices, all config sets,
    # and all the number of saved configs for all devices in the Db. Those
    # methods would support a ConfigDb Manager application. Also ponder about
    # adding the most recent timestamp when a configuration has been applied.
    # That information might be useful for someone with a ConfigDb admin role
    # that wants to get rid of stale configurations. Also add the corresponding
    # method to delete a configuration - only accessible to users with admin
    # role (users might be real people or scheduled maintenance jobs.

    def list_configurations(self, deviceId, name_part=''):
        """Retrieves the set of device configurations related to a name part

        :param deviceId: the id of the device
        :param name_part: the name part of the device configurations. empty
                          means all the named configurations.

        :returns: list of configuration records (can be empty). Each configu-
                  ration record is a dictionary. The value for the key
                  'config_set_id' os either -1, if the configuration is the
                  only that has been saved as part of the save operation, or it
                  is the id of the config_set (and its value can be used to
                  retrieve the remaining configs that were saved by the same
                  save operation).
        """
        with self.dbHandle:
            cfgs = (
                DeviceConfig
                .select(
                    ConfigSet.config_name, DeviceConfig.timestamp,
                    ConfigSet.description, ConfigSet.priority,
                    ConfigSet.user, ConfigSet.overwritable,
                    ConfigSet.device_set_id, ConfigSet.id
                )
                .join(ConfigSet)
            )
            if name_part:
                # When a name_part is given, filter configs by it.
                cfgs = cfgs.where(
                    DeviceConfig.device_id == deviceId,
                    ConfigSet.config_name.contains(name_part)
                )
            else:
                # Otherwise, only filters by device_id
                cfgs = cfgs.where(
                    DeviceConfig.device_id == deviceId
                )
            cfgs = cfgs.tuples()

            ret_recordings = []
            device_set_digest = create_config_set_id([deviceId])
            for cfg in cfgs:
                conf_dict = {}
                conf_dict[CONFIG_DB_NAME] = cfg[0]
                conf_dict[CONFIG_DB_TIMEPOINT] = (
                    cfg[1].strftime(ISO8601_FORMAT))
                conf_dict[CONFIG_DB_DESCRIPTION] = cfg[2]
                conf_dict[CONFIG_DB_PRIORITY] = cfg[3]
                conf_dict[CONFIG_DB_USER] = cfg[4]
                conf_dict[CONFIG_DB_OVERWRITABLE] = cfg[5]

                # If the config isn't the only config in its device set,
                # returns the config_set_id (the prim. key of the device set)
                # for the key CONFIG_DB_CONFIG_SET_ID; Otherwise returns -1 to
                # flag that the config is the only one saved in its device set.
                if device_set_digest == cfg[6]:
                    # the id generated for the device_set based on the
                    # device_id matches the digest in the database;
                    # that means the device configuration is the only one
                    # that has been saved as part of the config_set save.
                    conf_dict[CONFIG_DB_CONFIG_SET_ID] = -1
                else:
                    conf_dict[CONFIG_DB_CONFIG_SET_ID] = cfg[7]

                ret_recordings.append(conf_dict)

            return ret_recordings

    def list_configurations_in_set(self, config_set_id):
        """Retrieves the set of configurations in a given config set.

        :param config_set_id: the id of the config set

        :returns: list of configuration records (one or more). Each configu-
                  ration record is a dictionary with the keys:
                      "deviceId",
                      "config",
                      "schema",
                      "timepoint".
                  the list of configurations is sorted by "deviceId".
        """
        with self.dbHandle:
            cfgs = (
                DeviceConfig
                .select(
                    DeviceConfig.device_id, DeviceConfig.config_data,
                    ConfigSchema.schema_data, DeviceConfig.timestamp,
                )
                .join(ConfigSchema)
                .where(
                    DeviceConfig.config_set_id == config_set_id
                )
                .order_by(DeviceConfig.device_id)
            )
            cfgs = cfgs.tuples()

            ret_recordings = []
            for cfg in cfgs:
                conf_dict = {}
                conf_dict[CONFIG_DB_DEVICE_ID] = cfg[0]
                conf_dict[CONFIG_DB_DATA] = cfg[1]
                conf_dict[CONFIG_DB_SCHEMA] = cfg[2]
                conf_dict[CONFIG_DB_TIMEPOINT] = (
                    cfg[3].strftime(ISO8601_FORMAT))
                ret_recordings.append(conf_dict)

            return ret_recordings

    def list_devices(self, priority=3):
        """Retrieves the set of deviceIds related to configuration priority

        :param priority: the desired priority

        :returns: list of deviceIds records (can be empty).
        """
        with self.dbHandle:
            devs = (
                DeviceConfig
                .select(DeviceConfig.device_id.distinct())
                .join(ConfigSet)
                .where(ConfigSet.priority == priority)
                .tuples()
            )
            ret = [dev[0] for dev in devs]
            return ret

    # TODO: min_timepoint and max_timepoint have lost their precision since
    #       the introduction of minSetSize: the gathering of configs in the
    #       config set is not exhaustive for performance reasons and that
    #       means that it is not guaranteed that all timestamps for all
    #       configs in the set have been retrieved. The proposed solution is
    #       to remove MIN_TIMEPOINT, MAX_TIMEPOINT and DIFF_TIMEPOINT from
    #       the response records and add REF_TIMEPOINT which should be the
    #       average timepoint of the timepoints actually retrieved.
    def list_configuration_sets(self, deviceIds, minSetSize=None):
        """Retrieves the sets of configurations related to a list of devices.
        Sets containing configurations for either a subset or a superset of the
        list of devices can be returned depending on the value of the
        parameter minSetSize.

        The goal of returning config sets that do not have all the devices in
        the list (or that have more than the devices in the list) is to support
        configuration sets for components (sets of devices) whose composition
        changes through their lifetime.

        If no minSetSize is specified, all configuration sets containing at
        least the deviceIds specified will be retrieved (supersets). On the
        other hand, a minSetSize lower than the number of devices specified
        will allow retrieving subsets of the devices set.

        :param deviceIds: list of deviceIds whose configurations should be
                          listed.

        :param minSetSize: minimum size of configuration sets to retrieve. If
                           no value or a value lower than 1 is specified,
                           len(deviceIds) is used as minSetSize. If a value
                           greater than the number of devices is specified,
                           an exception of type ConfigurationDBError is
                           thrown.

        :returns: list of configuration records (can be empty). Each configu-
                  ration record is a dictionary. The configurations timestamps
                  are returned in ISO8601 format (yyyy-mm-ddThh:mm:ss.micro).

        Example: Retrieve configuration sets for devices DevA, DevB and DevC.
                 Let's assume that in the database, DevA has configurations
                 named {Nam1, Nam2, Nam3}, DevB has configurations named
                 {Nam2, Nam3} and DevC has configurations named
                 {Nam1, Nam3}. If minSetSize is not specified (or is lower
                 than one), it will assume the value 3 (the number of
                 deviceIds given). Only one set will be retrieved, as there is
                 only one set with configurations for all the given devices,
                 the set with the configuration named Nam3. If a minSetSize of
                 2 is specified, three sets will be returned: one for the two
                 configurations Nam1 saved for DevA and DevC, another for the
                 two configurations Nam2 saved for DevA and DevB and the same
                 set of size 3 returned when minSetSize was not specified.

        """
        if len(deviceIds) == 0:
            raise ConfigurationDBError('Please provide at least one device id')

        if not minSetSize or minSetSize < 0:
            minSetSize = len(deviceIds)
        else:
            if minSetSize > len(deviceIds):
                raise ConfigurationDBError(
                    'minSetSize has to be between 1 and the number of '
                    f'deviceIds, {len(deviceIds)}.')

        with self.dbHandle:
            DevCfgAlias = DeviceConfig.alias()
            query = (
                DevCfgAlias
                .select(
                    fn.COUNT(DevCfgAlias.device_id).alias('num_cfgs'),
                    ConfigSet.id.alias('config_set_id'), ConfigSet.config_name,
                    ConfigSet.description, ConfigSet.priority, ConfigSet.user,
                    ConfigSet.overwritable,
                    fn.MIN(DevCfgAlias.timestamp).alias('min_timepoint'),
                    fn.MAX(DevCfgAlias.timestamp).alias('max_timepoint'),
                )
                .join(ConfigSet)
                .where(DevCfgAlias.device_id << deviceIds)
                .group_by(ConfigSet.config_name, ConfigSet.device_set_id)
            )
            cfgs = (
                DeviceConfig
                .select(
                    query.c.num_cfgs, query.c.config_name,
                    query.c.description, query.c.priority,
                    query.c.user, query.c.overwritable,
                    query.c.min_timepoint, query.c.max_timepoint,
                    query.c.config_set_id
                )
                .from_(query)
                .where(query.c.num_cfgs >= minSetSize)
                .tuples()
            )

            ret_recordings = []
            for cfg in cfgs:
                conf_dict = {}
                conf_dict[CONFIG_DB_NAME] = cfg[1]
                conf_dict[CONFIG_DB_DESCRIPTION] = cfg[2]
                conf_dict[CONFIG_DB_PRIORITY] = cfg[3]
                conf_dict[CONFIG_DB_USER] = cfg[4]
                conf_dict[CONFIG_DB_OVERWRITABLE] = cfg[5]
                # NOTE: both timepoints will be retrieved as strings. SQLite
                # doesn't have a real datetime type; as there was no explicit
                # Model.DateTimeField involved, the peewee fabricated query
                # will return a string. This may change when a database that
                # has a real datetime type (e.g. Postgres) is used.
                min_time = datetime_from_string(cfg[6])
                max_time = datetime_from_string(cfg[7])
                conf_dict[CONFIG_DB_MIN_TIMEPOINT] = (
                    min_time.strftime(ISO8601_FORMAT))
                conf_dict[CONFIG_DB_MAX_TIMEPOINT] = (
                    max_time.strftime(ISO8601_FORMAT))
                diff_time = max_time - min_time
                conf_dict[CONFIG_DB_DIFF_TIMEPOINT] = diff_time.total_seconds()
                # Differently from list_configurations, where -1 is returned
                # for ConfigSets with only one configuration,
                # list_configurations_sets always returns the id of the
                # ConfigSet.
                conf_dict[CONFIG_DB_CONFIG_SET_ID] = cfg[8]

                ret_recordings.append(conf_dict)

            return ret_recordings

    def check_configurations_limits(self, deviceIds):
        """Checks if any of the devices exceeds the configuration limit.

        :param deviceIds: list of deviceIds who should be validated

        :returns: boolean (True or False)
        """
        with self.dbHandle:
            return self._check_configurations_limits(deviceIds)

    def get_configuration(self, deviceId, name):
        """Retrieves a device configuration given its name.

        :param deviceId: the id of the device.
        :param name: the name of the device configuration

        :returns: dictionary result (can be empty). The configuration
                  timestamp is returned in ISO8601 format
                  (yyyy-mm-ddThh:mm:ss.micro).
        """
        with self.dbHandle:
            cfg = (
                DeviceConfig
                .select(
                    ConfigSet.config_name, DeviceConfig.timestamp,
                    DeviceConfig.config_data, ConfigSchema.schema_data,
                    ConfigSet.description, ConfigSet.priority,
                    ConfigSet.user, ConfigSet.overwritable
                )
                .join(
                    ConfigSet,
                    on=(DeviceConfig.config_set == ConfigSet.id))
                .join(
                    ConfigSchema,
                    on=(DeviceConfig.schema == ConfigSchema.id))
                .where(
                    DeviceConfig.device_id == deviceId,
                    ConfigSet.config_name == name
                )
                .tuples()
            )
            return self._config_to_dict(cfg[0]) if len(cfg) > 0 else {}

    def get_last_configuration(self, deviceId, priority=1):
        """Retrieves the most recent device configuration with a priority.

        :param deviceId: the id of the device.
        :param priority: priority associated to the configuration to return.

        :returns: dictionary result (can be empty). The configuration timestamp
                  is returned in ISO8601 format (yyyy-mm-ddThh:mm:ss.micro).
        """
        with self.dbHandle:
            cfg = (
                DeviceConfig
                .select(
                    ConfigSet.config_name, DeviceConfig.timestamp,
                    DeviceConfig.config_data, ConfigSchema.schema_data,
                    ConfigSet.description, ConfigSet.priority,
                    ConfigSet.user, ConfigSet.overwritable
                )
                .join(
                    ConfigSet,
                    on=(DeviceConfig.config_set == ConfigSet.id))
                .join(
                    ConfigSchema,
                    on=(DeviceConfig.schema == ConfigSchema.id))
                .where(
                    DeviceConfig.device_id == deviceId,
                    ConfigSet.priority == priority
                )
                .order_by(DeviceConfig.timestamp.desc())
                .tuples()
            )

            return self._config_to_dict(cfg[0]) if len(cfg) > 0 else {}

    def save_configuration(self, name, configs, description="", user=".",
                           priority=1, timestamp="", overwritable=False):
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

        deviceSetId = create_config_set_id(deviceIds)

        with self.dbHandle as db:

            if self._is_config_name_taken(name, deviceIds, deviceSetId):
                raise ConfigurationDBError(
                    f"The config name {name} is already "
                    f"taken for at least one of the device(s) {deviceIds}")

            overwrite_id = self._get_overwrite_id(name, deviceSetId)
            with db.atomic() as txn:
                try:
                    if overwrite_id:
                        # An overwritable configuration with the name exists
                        # for the set of devices; update it.
                        self._update_configs(
                            overwrite_id, configs, description,
                            user, int(priority), timestamp=timestamp)
                    else:
                        # No configuration with the name exists for the set of
                        # devices; Add it after checking that the limit of
                        # configurations per device stored in the database will
                        # be respected.
                        is_limit = self._check_configurations_limits(deviceIds)
                        if is_limit:
                            raise ConfigurationDBError(
                                f'One of the devices "{deviceIds}" would '
                                f'exceed the number  of configurations per '
                                f'device, {CONFIGURATION_LIMIT}.')
                        self._insert_configs(
                            deviceSetId, name, configs, description,
                            user, int(priority), timestamp=timestamp,
                            overwritable=overwritable)
                except Exception as e:
                    txn.rollback()
                    raise ConfigurationDBError(f"{e}")
                else:
                    txn.commit()  # An explicit commit is not really needed.

    def is_config_name_taken(self, name, deviceIds, deviceSetId=None):
        """Checks if a config name is already in use by any of the devices in a
        list.

        :param  name: the configuration name to be checked.
        :param deviceIds: list of deviceIds whose taken configurations names
                          should be searched.
        :param deviceSetId: identifier for the set of devices in deviceIds.


        :returns: True if the config name is already taken by any of the
                  devices in the list.
        """
        with self.dbHandle:
            return self._is_config_name_taken(name, deviceIds, deviceSetId)

    # Private Interface
    # --------------------------------------------------------------------

    def _is_config_name_taken(self, name, deviceIds, deviceSetId=None):
        """The actual core of the public is_config_name_taken. The public
        wrapper takes care of creating a connection context. The private
        core uses the connection context from its caller.
        """
        ro_cfg_with_name = (
            DeviceConfig
            .select(fn.COUNT(DeviceConfig.id))
            .join(ConfigSet)
            .where(
                ConfigSet.config_name == name,
                ConfigSet.overwritable == False,  # noqa: E712
                DeviceConfig.device_id << deviceIds
            )
            .scalar()
        )
        if ro_cfg_with_name > 0:
            # If there's already any read-only config with the same
            # name for any of the devices, the name is taken.
            return True

        if not deviceSetId:
            deviceSetId = create_config_set_id(deviceIds)

        ovwr_cfgs = (
            ConfigSet
            .select(ConfigSet.device_set_id)
            .where(
                ConfigSet.config_name == name,
                ConfigSet.overwritable == True  # noqa: E712
            )
            .tuples()
        )
        if (
            len(ovwr_cfgs) == 0 or
            (len(ovwr_cfgs) == 1 and ovwr_cfgs[0][0] == deviceSetId)
        ):
            # The name is not taken if there's no other overwritable
            # config with the same name or if there's only one and it is
            # for the same device set.
            return False
        else:
            return True

    def _check_configurations_limits(self, deviceIds):
        """The actual core of the public _check_configuration_limits.
        The public wrapper takes care of creating a connection context. The
        private core uses the connection context from its caller.
        """
        DevConfigAlias = DeviceConfig.alias()
        cnt_qry = (
            DevConfigAlias
            .select(
                DevConfigAlias.device_id,
                fn.COUNT(DevConfigAlias.config_data).alias('num_cfgs')
            )
            .where(DevConfigAlias.device_id << deviceIds)
            .group_by(DevConfigAlias.device_id)
        )
        devs_limit = (
            DeviceConfig
            .select(fn.COUNT(cnt_qry.c.device_id))
            .from_(cnt_qry)
            .where(cnt_qry.c.num_cfgs >= CONFIGURATION_LIMIT)
            .scalar()
        )
        return devs_limit > 0

    def _schema_id_from_digest(self, schemaDigest):
        """Gets the schema id for a given digest.

        :return: -1 if there's no schema with the given digest in the db;
                 otherwise returns the id of the schema in the database.
        """
        try:
            sch_id = (
                ConfigSchema.get(ConfigSchema.digest == schemaDigest).id
            )
            return sch_id
        except DoesNotExist:
            return -1

    def _get_overwrite_id(self, name, set_id):
        """Gets an overwritable configuration for a given set of devices
           from the database.

        :param name: the name of the overwritable configuration.
        :param set_id: the set identifier (unique hash for each set of devices)
        :return: the internal identifier of the overwritable config set, or
                 None if no config set has been found.
        """
        try:
            overwrite_id = (
                ConfigSet
                .select(ConfigSet.id)
                .where(
                    ConfigSet.device_set_id == set_id,
                    ConfigSet.config_name == name
                )
                .get()
            )
            return overwrite_id
        except DoesNotExist:
            return None

    def _get_config_set(self, name, set_id):
        """Gets the config set id for a given name and set digest.

        :param name: the name of the configuration.
        :param set_id: the set identifier (unique hash for each set of devices)
        :return: the internal identifier of the config set, or None if no
                 config set has been found.
        """
        try:
            cfg_set_id = (
                ConfigSet.get(
                    ConfigSet.device_set_id == set_id,
                    ConfigSet.config_name == name
                ).id
            )
            return cfg_set_id
        except DoesNotExist:
            return None

    def _config_to_dict(self, cfg):
        """Private method to transform a config fecthed from the database into
           a dictionary.
        """
        conf_dict = {}
        if cfg:
            conf_dict[CONFIG_DB_NAME] = cfg[0]
            conf_dict[CONFIG_DB_TIMEPOINT] = (
                cfg[1].strftime(ISO8601_FORMAT))
            conf_dict[CONFIG_DB_DATA] = cfg[2]
            conf_dict[CONFIG_DB_SCHEMA] = cfg[3]
            conf_dict[CONFIG_DB_DESCRIPTION] = cfg[4]
            conf_dict[CONFIG_DB_PRIORITY] = cfg[5]
            conf_dict[CONFIG_DB_USER] = cfg[6]
            conf_dict[CONFIG_DB_OVERWRITABLE] = cfg[7]

        return conf_dict

    def _update_configs(self, set_id, configs, description, user,
                        priority, timestamp):
        """Updates overwritable device configurations in the database after
           updating the corresponding device set record."""
        # Starts the transaction and writes each of the configs,
        # committing at the end or rolling back if any error happens.
        ConfigSet.update(
            user=user, priority=priority, description=description
        ).where(ConfigSet.id == set_id).execute()
        for config in configs:
            self._update_config(set_id, config, timestamp)

    def _update_config(self, set_id, config, timestamp):
        """Updates a device configuration in the database.

        NOTE: It's up to the caller to guarantee that the set_id refers to an
              overwritable configuration set.
        """
        schema_id = self._insert_schema_if_needed(config[CONFIG_DB_SCHEMA])

        if len(timestamp) > 0:
            timestamp_val = datetime_from_string(timestamp)
        else:
            timestamp_val = datetime.now()

        update_count = DeviceConfig.update(
            config_data=config[CONFIG_DB_DATA], schema_id=schema_id,
            timestamp=timestamp_val
        ).where(
            DeviceConfig.config_set_id == set_id,
            DeviceConfig.device_id == config[CONFIG_DB_DEVICE_ID]
        ).execute()

        if update_count == 0:
            # There was no device config record to update. This may happen
            # for overwritable configurations that are saved with injected
            # device_set_id values. During the first multi chunk save of
            # such configurations, any save operation that is not for the
            # first chunk of device configs to be saved may not trigger any
            # update during the save.
            self._insert_config(set_id, config, timestamp)

    def _insert_configs(self, set_digest, name, configs, description,
                        user, priority, timestamp, overwritable):
        """Saves new device configurations in the database after saving the
           corresponding device set record."""
        # Starts the transaction and writes each of the configs,
        # committing at the end or rolling back if any error happens.
        set_id = self._get_config_set(name, set_digest)
        if not set_id:
            # There's no config set yet for the configurations to be saved.
            # Insert it.
            set_id = ConfigSet.insert(
                device_set_id=set_digest, config_name=name,
                user=user, priority=priority, description=description,
                overwritable=overwritable
            ).execute()
        for config in configs:
            self._insert_config(set_id, config, timestamp)

    def _insert_config(self, set_id, config, timestamp):
        """Saves a new device configuration in the database."""
        schema_id = self._insert_schema_if_needed(config[CONFIG_DB_SCHEMA])
        if len(timestamp) > 0:
            cfg_id = DeviceConfig.insert(
                config_set_id=set_id, device_id=config[CONFIG_DB_DEVICE_ID],
                timestamp=datetime_from_string(timestamp),
                config_data=config[CONFIG_DB_DATA], schema_id=schema_id
            ).execute()
        else:
            cfg_id = DeviceConfig.insert(
                config_set_id=set_id, device_id=config[CONFIG_DB_DEVICE_ID],
                config_data=config[CONFIG_DB_DATA], schema_id=schema_id
            ).execute()
        return cfg_id

    def _insert_schema_if_needed(self, sch_data):
        """Inserts an schema if it's digest reveals it's not stored yet.

           :param db: handle to an open database connection.
           :param sch_data: serialized form of the schema to store.
           :return: id of the existing or just created schema record.
        """
        sch_digest = hashlib.sha1(sch_data.encode('utf-8')).hexdigest()
        sch_id = self._schema_id_from_digest(sch_digest)
        if sch_id == -1:
            sch_id = ConfigSchema.insert(
                digest=sch_digest, schema_data=sch_data
            ).execute()
        return sch_id
