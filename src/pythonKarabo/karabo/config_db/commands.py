# flake8: noqa

CREATE_CONFIG_TABLE = """
    CREATE TABLE IF NOT EXISTS DeviceConfig (
      id            INTEGER      PRIMARY KEY AUTOINCREMENT,
      device_id     STRING(1024) NOT NULL,
      timestamp     DATETIME     NOT NULL DEFAULT(STRFTIME('%Y-%m-%d %H:%M:%f', 'NOW')),
      config_name   STRING(80)   NOT NULL,
      config_data   TEXT         NOT NULL,
      schema_id     INTEGER      NOT NULL,
      user          STRING(64)   DEFAULT "." NOT NULL,
      priority      INTEGER      DEFAULT 1 CHECK(priority > 0 AND priority < 4),
      description   TEXT,

      UNIQUE(device_id, config_name),
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
    CREATE INDEX IF NOT EXISTS IdxPriority ON DeviceConfig (
      priority ASC
    );"""

CREATE_INDEX_NAME_TABLE = """
    CREATE INDEX IF NOT EXISTS IdxName ON DeviceConfig (
      config_name ASC
    );"""

# --------------------------------------------------------------------------
# Functions

CMD_LIST_NAME = """
        SELECT cfg.config_name, cfg.timestamp, cfg.config_data,
               sch.schema_data, cfg.description, cfg.priority,
               cfg.user
        FROM DeviceConfig cfg, ConfigSchema sch
        WHERE cfg.schema_id = sch.id
          AND cfg.device_id = ?
          AND cfg.config_name LIKE ?
    """

CMD_LIST_NO_NAME = """
        SELECT cfg.config_name, cfg.timestamp, cfg.config_data,
               sch.schema_data, cfg.description, cfg.priority,
               cfg.user
        FROM DeviceConfig cfg, ConfigSchema sch
        WHERE cfg.schema_id = sch.id
          AND cfg.device_id = ?
    """

CMD_GET_CONFIGURATION = """
        SELECT cfg.config_name, cfg.timestamp, cfg.config_data,
               sch.schema_data, cfg.description, cfg.priority,
               cfg.user
        FROM DeviceConfig cfg, ConfigSchema sch
        WHERE cfg.schema_id = sch.id
          AND cfg.device_id = ?
          AND cfg.config_name = ?
    """

CMD_GET_LAST_CONFIGURATION = """
        SELECT cfg.config_name, cfg.timestamp, cfg.config_data,
               sch.schema_data, cfg.description, cfg.priority,
               cfg.user
        FROM DeviceConfig cfg, ConfigSchema sch
        WHERE cfg.schema_id = sch.id
          AND cfg.device_id = ?
          AND cfg.priority = ?
        ORDER BY timestamp DESC
        LIMIT 1
    """

CMD_CHECK_NAME_TAKEN = """
        SELECT COUNT(id) FROM DeviceConfig
        WHERE config_name = ?
          AND device_id = ?
    """

CMD_CHECK_CONFIG_ALREADY_SAVED = """
        SELECT COUNT(id) FROM DeviceConfig
        WHERE device_id = ?
          AND timestamp = strftime('%Y-%m-%d %H:%M:%f', ?)
    """

CMD_CHECK_SCHEMA_ALREADY_SAVED = """
        SELECT id FROM ConfigSChema
        WHERE digest=?
    """

CMD_SAVE_SCHEMA = """
        INSERT INTO ConfigSchema(digest, schema_data)
        VALUES (?, ?)
    """

CMD_SAVE_CONFIGURATION_NO_TIMESTAMP = """
        INSERT INTO DeviceConfig
        (device_id, config_name, config_data, schema_id, user, priority, description)
        VALUES
        (?, ?, ?, ?, ?, ?, ?)
    """

CMD_SAVE_CONFIGURATION = """
        INSERT INTO DeviceConfig
        (device_id, config_name, timestamp, config_data, schema_id, user, priority, description)
        VALUES
        (?, ?, strftime('%Y-%m-%d %H:%M:%f', ?), ?, ?, ?, ?, ?)
    """
