#!/usr/bin/env python

import argparse
from asyncio import get_event_loop
import os
import re

from karabo.influxdb import DlRaw2Influx, DlSchema2Influx
from karabo.influxdb.dlutils import device_id_from_path


class DlMigrator():
    def __init__(self, topic, input_dir, output_dir, host,
                 protocol="http", user="", password="", port=8086,
                 dry_run=True, prints_on=False, use_gateway=False):
        self.input_dir = input_dir
        self.output_dir = output_dir
        self.dry_run = dry_run
        self.user = user
        self.password = password
        self.port = port
        self.host = host
        self.protocol = protocol
        self.chunk_queries = 8000  # optimal values are between 5k and 10k
        self.prints_on = prints_on
        self.db_name = topic
        self.use_gateway = use_gateway

    async def run(self):
        n_processed = 0
        n_part_processed = 0
        for dir_, _, files in os.walk(self.input_dir):
            if not dir_.endswith("raw"):
                continue
            if len(files) == 0:
                # Directory contains no files, just move on.
                continue
            common_prefix = os.path.commonprefix([self.input_dir, dir_])
            if not common_prefix.endswith(os.path.sep):
                common_prefix = common_prefix + os.path.sep
            # The deviceId is assumed to be everything between the end
            # of the input_dir path and the '/raw' termination.
            device_id = dir_[len(common_prefix):-4]
            for filename in files:
                file_path = os.path.join(dir_, filename)
                if filename == "archive_schema.txt":
                    print("Migrating schemas in '{}' ...".format(file_path),
                          end='')
                    success = await self.insert_schema(file_path,
                                                       self.output_dir,
                                                       device_id)
                    if success:
                        n_processed += 1
                        print(" succeeded.")
                    else:
                        n_part_processed += 1
                        print(" failed.")
                if re.match(r"^archive_[0-9]+\.txt$", filename):
                    file_path = os.path.join(dir_, filename)
                    print(
                        "Migrating configurations in '{}' ..."
                        .format(file_path), end=''
                    )
                    success = await self.insert_archive(file_path,
                                                        self.output_dir,
                                                        device_id)
                    if success:
                        n_processed += 1
                        print(" succeeded")
                    else:
                        n_part_processed += 1
                        print(" failed")
        print("Migration job finished:")
        print("Files processed successfully: {}".format(n_processed))
        print("Files with migration errors:  {}".format(n_part_processed))

    async def insert_archive(self, path, output_dir, device_id):
        conf2db = DlRaw2Influx(
            raw_path=path, output_dir=output_dir,
            topic=self.db_name, user=self.user, password=self.password,
            host=self.host, port=self.port, protocol=self.protocol,
            dry_run=self.dry_run, device_id=device_id,
            prints_on=self.prints_on, use_gateway=self.use_gateway)
        return await conf2db.run()

    async def insert_schema(self, path, output_dir, device_id):
        schema2db = DlSchema2Influx(
            schema_path=path, output_dir=output_dir,
            topic=self.db_name, user=self.user, password=self.password,
            host=self.host, port=self.port, protocol=self.protocol,
            dry_run=self.dry_run, device_id=device_id,
            prints_on=self.prints_on, use_gateway=self.use_gateway)
        return await schema2db.run()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Migrates data from datalogger raw files "
                    "to InfluxDb. Completely and partially "
                    "migrated files are moved to directories "
                    "under a specified output base directory.")
    parser.add_argument("topic", help="Karabo topic")
    parser.add_argument("input_dir",
                        help="Root firectory for the datalogger raw files: "
                             "every 'raw' directory under this directory "
                             "will be assumed to be a container for "
                             "datalogger files. All the 'archive_*.txt' files "
                             "in those directories will be processed. A common "
                             "value for this argument would be "
                             "'var/data/karaboHistory'.")
    parser.add_argument("output_dir",
                        help="Root directory for the processed and partially "
                             "processed raw files (and the errors that "
                             "prevented them from being fully processed). "
                             "Processed and partially processed files will be "
                             "stored on two subdirectories of output_dir, "
                             "'processed' and 'part_processed'.")
    parser.add_argument("host", help="influxDb hostname")
    parser.add_argument("--user", dest="user", help="influxDb user name")
    parser.set_defaults(user="")
    parser.add_argument("--password", dest="password",
                        help="influxDb user password")
    parser.set_defaults(password="")
    parser.add_argument("--protocol", dest="protocol",
                        help="protocol: http or https")
    parser.set_defaults(protocol="http")
    parser.add_argument("--port", dest="port", help="influxDb port")
    parser.set_defaults(port=8086)
    parser.add_argument("--use_gateway", dest="use_gateway",
                        help="When true, the host is an Influx gateway and "
                             "credentials should be placed in the url between "
                             "protocol and hostname.")
    parser.set_defaults(use_gateway=False)
    parser.add_argument("--dry_run", dest="dry_run", action="store_true")
    parser.add_argument("--quiet", dest="prints_on", action="store_false")
    parser.set_defaults(dry_run=False)
    args = parser.parse_args()
    o = DlMigrator(**vars(args))
    get_event_loop().run_until_complete(o.run())
