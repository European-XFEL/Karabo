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
import argparse
import os
import sqlite3
import sys
from pathlib import Path

from dateutil import parser

from karabo.config_db import (
    ConfigurationDatabase, datetime_to_str, hashFromBase64Bin)
from karabo.native import encodeXML

SHOW_TABLES = False
NEW_DB_NAME = "karaboDB3"


async def run(filename: Path | None = None):
    if filename is not None:
        config_db_file = Path(filename)
        config_db_path = config_db_file.parent
    else:
        env = os.getenv("KARABO")
        assert env, "Karabo must be activated ..."
        path = Path(env)
        config_db_path = path.joinpath("var/data/config_db")
        if not config_db_path.exists():
            print("Did not find config db folder (.../var/data/config_db) "
                  "exiting")
            sys.exit(1)

        config_db_file = config_db_path.joinpath("karaboDB")
        if not config_db_file.exists():
            print("No `karaboDB` file existing. Migration not required.")
            sys.exit(1)
        print("Found configuration db file, migrating ...")

    new_config_db_path = config_db_path.joinpath(NEW_DB_NAME)

    # Connect to the old database
    conn = sqlite3.connect(config_db_file)
    cursor = conn.cursor()

    if SHOW_TABLES:
        cursor.execute(
            "SELECT name FROM sqlite_master WHERE type='table';")
        tables = cursor.fetchall()

        # For each table, get column descriptions
        for table_name in tables:
            table = table_name[0]
            print(f"\nTable: {table}")
            cursor.execute(f"PRAGMA table_info({table});")
            columns = cursor.fetchall()
            # Columns: cid, name, type, notnull, default_value, pk
            for col in columns:
                col_name = col[1]
                col_type = col[2]
                is_pk = " PRIMARY KEY" if col[5] else ""
                is_not_null = " NOT NULL" if col[3] else ""
                default_val = (f" DEFAULT {col[4]}"
                               if col[4] is not None else "")

                print(
                    f"  - {col_name} ({col_type}){is_pk}"
                    f"{is_not_null}{default_val}")

    new_db = ConfigurationDatabase(new_config_db_path)
    await new_db.assure_existing()

    # First, load all ConfigSets into a dictionary for fast lookup
    cursor.execute("SELECT id, config_name FROM ConfigSet")
    configset_map = {row[0]: row[1] for row in cursor.fetchall()}

    # Example: Read all DeviceConfigs
    cursor.execute("SELECT config_set_id, device_id, "
                   "config_data, timestamp FROM DeviceConfig")
    rows = cursor.fetchall()

    # DeviceConfigs saving
    for row in rows:
        # index = row[0]
        config_set_id = row[0]
        deviceId = row[1]
        config = hashFromBase64Bin(row[2])
        serverId = config["serverId"]
        classId = config["classId"]
        timepoint = row[3]
        # Use dateutil.parser to auto-detect and parse
        dt = parser.parse(timepoint)
        timepoint = datetime_to_str(dt)
        config_name = str(configset_map.get(config_set_id))
        assert config_name is not None
        config_data = [
            {"deviceId": deviceId, "config": encodeXML(config),
             "serverId": serverId,
             "classId": classId}
        ]
        text = (f"Saving ... Device: {deviceId}, "
                f"Config Name: {config_name}, "
                f"Timestamp: {timepoint}")
        print(text)
        await new_db.save_configuration(
            config_name, configs=config_data,
            timestamp=timepoint)


def main():
    import asyncio
    parser = argparse.ArgumentParser()
    text = ("Absolute path to the file for testing. If `None` as default, "
            "the configuration db path with default file name is looked up.")
    parser.add_argument("--filename", help=text, default=None)
    args = parser.parse_args()
    asyncio.run(run(args.filename))


if __name__ == "__main__":
    main()
