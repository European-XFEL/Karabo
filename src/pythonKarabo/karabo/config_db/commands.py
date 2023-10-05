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
# flake8: noqa

CREATE_CONFIG_SET_TABLE = """
    CREATE TABLE IF NOT EXISTS ConfigSet (
      id            INTEGER      PRIMARY KEY AUTOINCREMENT,
      device_set_id STRING(40)   NOT NULL,
      config_name   STRING(80)   NOT NULL,
      user          STRING(64)   DEFAULT "." NOT NULL,
      priority      INTEGER      DEFAULT 1 CHECK(priority > 0 AND priority < 4),
      description   TEXT,
      overwritable  BOOL         DEFAULT FALSE NOT NULL,

      UNIQUE(device_set_id, config_name)
    );"""

CREATE_CONFIG_TABLE = """
    CREATE TABLE IF NOT EXISTS DeviceConfig (
      id            INTEGER      PRIMARY KEY AUTOINCREMENT,
      config_set_id INTEGER      NOT NULL,
      device_id     STRING(1024) NOT NULL,
      config_data   TEXT         NOT NULL,
      schema_id     INTEGER      NOT NULL,
      timestamp     DATETIME     NOT NULL DEFAULT(STRFTIME('%Y-%m-%d %H:%M:%f', 'NOW')),

      UNIQUE(config_set_id, device_id),
      FOREIGN KEY(config_set_id) REFERENCES ConfigSet(id),
      FOREIGN KEY(schema_id) REFERENCES ConfigSchema(id)
    );"""

CREATE_SCHEMA_TABLE = """
    CREATE TABLE IF NOT EXISTS ConfigSchema (
      id           INTEGER    PRIMARY KEY AUTOINCREMENT,
      digest       STRING(40) NOT NULL,
      schema_data  TEXT       NOT NULL,

      UNIQUE(digest)
    );"""

CREATE_INDEX_SCHEMA_DIGEST_TABLE = """
    CREATE INDEX IF NOT EXISTS IdxDigest ON ConfigSchema (
      digest ASC
    );"""

CREATE_INDEX_DEVICE_ID_TABLE = """
    CREATE INDEX IF NOT EXISTS IdxDeviceId ON DeviceConfig (
      device_id ASC
    );"""

CREATE_INDEX_PRIORITY_TABLE = """
    CREATE INDEX IF NOT EXISTS IdxPriority ON ConfigSet (
      priority ASC
    );"""

CREATE_INDEX_NAME_TABLE = """
    CREATE INDEX IF NOT EXISTS IdxName ON ConfigSet (
      config_name ASC
    );"""

CREATE_INDEX_DEVICE_SET_ID_TABLE = """
    CREATE INDEX IF NOT EXISTS IdxDeviceSetId ON ConfigSet (
      device_set_id ASC
    );"""

# --------------------------------------------------------------------------
# Functions

CMD_LIST_NAME = """
        SELECT cfg_set.config_name, cfg.timestamp, cfg_set.description,
               cfg_set.priority, cfg_set.user, cfg_set.overwritable
        FROM DeviceConfig cfg, ConfigSet cfg_set
        WHERE cfg.device_id = ?
          AND cfg.config_set_id = cfg_set.id
          AND cfg_set.config_name LIKE ?
    """

CMD_LIST_NO_NAME = """
        SELECT cfg_set.config_name, cfg.timestamp, cfg_set.description,
               cfg_set.priority, cfg_set.user, cfg_set.overwritable
        FROM DeviceConfig cfg, ConfigSet cfg_set
        WHERE cfg.device_id = ?
          AND cfg.config_set_id = cfg_set.id
    """

CMD_LIST_DEVICES_PRIORITY = """
        SELECT DISTINCT cfg.device_id
        FROM DeviceConfig cfg, ConfigSet cfg_set
        WHERE cfg_set.priority = ?
          AND cfg.config_set_id = cfg_set.id
    """


def CMD_LIST_DEVS_NO_NAME(num_of_devices):
    return f"""
        SELECT *,
        ((JULIANDAY(max_timepoint)-JULIANDAY(min_timepoint))*86400) timepoint_diff_sec
    FROM (
        SELECT COUNT(cfg.device_id) num_cfgs, cfg_set.config_name,
            cfg_set.description, cfg_set.priority, cfg_set.user, cfg_set.overwritable,
            MIN(timestamp) min_timepoint,
            MAX(timestamp) max_timepoint
        FROM DeviceConfig cfg, ConfigSet cfg_set
        WHERE cfg.device_id IN ({','.join('?'*num_of_devices)})
          AND cfg.config_set_id = cfg_set.id
        GROUP BY cfg_set.config_name, cfg_set.device_set_id
    )
    WHERE num_cfgs = {num_of_devices}
    """


CMD_GET_CONFIGURATION = """
        SELECT cfg_set.config_name, cfg.timestamp, cfg.config_data,
               sch.schema_data, cfg_set.description, cfg_set.priority,
               cfg_set.user, cfg_set.overwritable
        FROM DeviceConfig cfg, ConfigSchema sch, ConfigSet cfg_set
        WHERE cfg.schema_id = sch.id
          AND cfg.config_set_id = cfg_set.id
          AND cfg.device_id = ?
          AND cfg_set.config_name = ?
    """

CMD_GET_OVERWRITABLE_CONFIG_FOR_SET = """
        SELECT id
        FROM ConfigSet
        WHERE device_set_id = ?
          AND config_name = ?
          AND overwritable = 1
    """

CMD_GET_CONFIG_FOR_SET = """
        SELECT id
        FROM ConfigSet
        WHERE device_set_id = ?
          AND config_name = ?
    """

CMD_GET_OVERWRITABLE_CONFIGS_FOR_NAME = """
        SELECT device_set_id
          FROM ConfigSet
          WHERE config_name = ?
            AND overwritable = 1
    """

CMD_GET_CONFIGS_PER_DEVICE_SET_MIN_AGE = """
        SELECT cfg_set.id set_id, cfg_set.device_set_id set_digest,
               cfg_set.config_name, cfg.id cfg_id, cfg.device_id, cfg.timestamp
        FROM DeviceConfig cfg, ConfigSet cfg_set
        WHERE cfg.config_set_id = cfg_set.id
          AND (JULIANDAY('NOW') - JULIANDAY(cfg.timestamp))*1440 > ?
        ORDER BY set_id
"""

CMD_GET_LAST_CONFIGURATION = """
        SELECT cfg_set.config_name, cfg.timestamp, cfg.config_data,
               sch.schema_data, cfg_set.description, cfg_set.priority,
               cfg_set.user, cfg_set.overwritable
        FROM DeviceConfig cfg, ConfigSchema sch, ConfigSet cfg_set
        WHERE cfg.schema_id = sch.id
          AND cfg.config_set_id = cfg_set.id
          AND cfg.device_id = ?
          AND cfg_set.priority = ?
        ORDER BY timestamp DESC
        LIMIT 1
    """


def CMD_CHECK_NAME_USED_READONLY_CONFIG(num_of_devices):
    return f"""
        SELECT COUNT(cfg.id)
        FROM DeviceConfig cfg, ConfigSet cfg_set
        WHERE cfg.config_set_id = cfg_set.id
          AND cfg_set.config_name = ?
          AND cfg.device_id IN ({','.join('?'*num_of_devices)})
          AND cfg_set.overwritable = 0
    """


def CMD_CHECK_DEVICE_LIMIT(num_of_devices):
    return f"""
    SELECT COUNT(device_id) FROM (
        SELECT device_id, COUNT(id) num_configs
        FROM DeviceConfig
        WHERE device_id IN ({','.join('?'*num_of_devices)})
        GROUP BY device_id )
    WHERE num_configs >= ?
"""


CMD_CHECK_CONFIG_ALREADY_SAVED = """
        SELECT COUNT(id) FROM DeviceConfig
        WHERE device_id = ?
          AND timestamp = strftime('%Y-%m-%d %H:%M:%f', ?)
    """

CMD_CHECK_SCHEMA_ALREADY_SAVED = """
        SELECT id FROM ConfigSchema
        WHERE digest=?
    """

CMD_INSERT_CONFIG_SET = """
        INSERT INTO ConfigSet
        (device_set_id, config_name, user, priority, description, overwritable)
        VALUES
        (?, ?, ?, ?, ?, ?)
    """

CMD_INSERT_SCHEMA = """
        INSERT INTO ConfigSchema(digest, schema_data)
        VALUES (?, ?)
    """

CMD_INSERT_CONFIGURATION_NO_TIMESTAMP = """
        INSERT INTO DeviceConfig
        (config_set_id, device_id, config_data, schema_id)
        VALUES
        (?, ?, ?, ?)
    """

CMD_INSERT_CONFIGURATION = """
        INSERT INTO DeviceConfig
        (config_set_id, device_id, timestamp, config_data, schema_id)
        VALUES
        (?, ?, strftime('%Y-%m-%d %H:%M:%f', ?), ?, ?)
    """

CMD_UPDATE_CONFIG_SET = """
        UPDATE ConfigSet
        SET
          user = ?,
          priority = ?,
          description = ?
        WHERE id = ?
    """

CMD_UPDATE_CONFIGURATION_NO_TIMESTAMP = """
        UPDATE DeviceConfig
        SET
          config_data = ?,
          schema_id = ?,
          timestamp = STRFTIME('%Y-%m-%d %H:%M:%f', 'NOW')
        WHERE config_set_id = ?
          AND device_id = ?
    """

CMD_UPDATE_CONFIGURATION = """
        UPDATE DeviceConfig
        SET
          config_data = ?,
          schema_id = ?,
          timestamp = strftime('%Y-%m-%d %H:%M:%f', ?)
        WHERE config_set_id = ?
          AND device_id = ?
    """
