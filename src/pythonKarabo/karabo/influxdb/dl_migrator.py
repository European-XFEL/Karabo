#!/usr/bin/env python

import argparse
from asyncio import get_event_loop
import os
import re
import shutil
from urllib.parse import urlparse

from karabo.influxdb import (
    DlRaw2Influx, DlSchema2Influx, PROCESSED_RAWS_FILE_NAME,
    PROCESSED_SCHEMAS_FILE_NAME
)
from karabo.influxdb.dlutils import device_id_from_path


class DlMigrator():
    def __init__(self, db_name, input_dir, output_dir,
                 write_url, write_user, write_pwd,
                 read_url, read_user, read_pwd,
                 lines_per_write, write_timeout,
                 dry_run=True, prints_on=False):
        self.db_name = db_name
        self.input_dir = input_dir
        self.output_dir = output_dir
        self.write_url = write_url
        self.write_user = write_user
        self.write_pwd = write_pwd
        self.read_url = read_url
        self.read_user = read_user
        self.read_pwd = read_pwd
        self.dry_run = dry_run
        self.prints_on = prints_on
        self.lines_per_write = lines_per_write
        self.write_timeout = write_timeout
        self.prev_run_file = os.path.join(output_dir, '.previous_run.txt')
        self.processed_raws = set()
        self.processed_schemas = set()

    def _get_prev_run_num(self):
        """Retrieves the number of the previous run."""
        prev_run_num = 0
        if os.path.exists(self.prev_run_file):
            with open(self.prev_run_file, 'r') as pr_file:
                prev_run_num = int(pr_file.read())  # Invalid file should throw.
        return prev_run_num

    def _backup_previous_run(self):
        """Backs-up 'processed' and 'part_processed' trees for the previous run
        of the migrator.
        The trees are saved in a directory named 'run_[num_of_previous_run]'.
        The number of the previous run is kept in file '.previous_run.txt' in
        the [output_dir] directory. Run numbers start from one.
        """
        proc_path = os.path.join(self.output_dir, 'processed')
        part_proc_path = os.path.join(self.output_dir, 'part_processed')
        if not os.path.exists(proc_path) and not os.path.exists(part_proc_path):
            return  # there's nothing to back-up

        prev_run_num = self._get_prev_run_num()
        backup_dir = os.path.join(self.output_dir,
                                  f'run_{prev_run_num:03}')
        os.makedirs(backup_dir)

        if os.path.exists(proc_path):
            shutil.move(proc_path, os.path.join(backup_dir, 'processed'))
        if os.path.exists(part_proc_path):
            shutil.move(part_proc_path,
                        os.path.join(backup_dir, 'part_processed'))

    def _inc_previous_run(self):
        """Increments (or initializes) the previous run number file."""
        prev_run_num = self._get_prev_run_num()
        if not os.path.exists(self.output_dir):
            os.makedirs(self.output_dir)
        with open(self.prev_run_file, 'w') as pr_file:
            pr_file.write(str(prev_run_num + 1))

    def _load_processed_files(self):
        proc_raws_path = os.path.join(self.output_dir, PROCESSED_RAWS_FILE_NAME)
        if os.path.exists(proc_raws_path):
            with open(proc_raws_path, 'r') as proc_raws_file:
                for _, file_name in enumerate(proc_raws_file):
                    self.processed_raws.add(file_name.strip())
        proc_schemas_path = os.path.join(self.output_dir,
                                         PROCESSED_SCHEMAS_FILE_NAME)
        if os.path.exists(proc_schemas_path):
            with open(proc_schemas_path, 'r') as proc_schemas_file:
                for _, file_name in enumerate(proc_schemas_file):
                    self.processed_schemas.add(file_name.strip())

    async def run(self):
        self._load_processed_files()
        self._backup_previous_run()
        self._inc_previous_run()
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
                    if file_path in self.processed_schemas:
                        print("'{}' already migrated: skip.".format(file_path))
                        continue
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
                    if file_path in self.processed_raws:
                        print("'{}' already migrated: skip.".format(file_path))
                        continue
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
        try:
            conf2db = DlRaw2Influx(
                raw_path=path, output_dir=output_dir,
                topic=self.db_name, user=self.write_user,
                password=self.write_pwd, url=self.write_url,
                dry_run=self.dry_run, device_id=device_id,
                lines_per_write=self.lines_per_write,
                write_timeout=self.write_timeout,
                prints_on=self.prints_on)
        except Exception as exc:
            print("Error creating property file converter:\n"
                  "File to be converted: {}\n"
                  "Error message: {}".format(path, exc))
            return False
        return await conf2db.run()

    async def insert_schema(self, path, output_dir, device_id):
        try:
            schema2db = DlSchema2Influx(
                schema_path=path, topic=self.db_name,
                read_user=self.read_user, read_pwd=self.read_pwd,
                read_url=self.read_url, write_user=self.write_user,
                write_pwd=self.write_pwd, write_url=self.write_url,
                device_id=device_id, output_dir=output_dir,
                write_timeout=self.write_timeout,
                dry_run=self.dry_run, prints_on=self.prints_on)
        except Exception as exc:
            print("Error creating schema file converter:\n"
                  "File to be converted: {}\n"
                  "Error message: {}".format(path, exc))
            return False
        return await schema2db.run()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Migrates data from datalogger raw files "
                    "to InfluxDb. Completely and partially "
                    "migrated files are moved to directories "
                    "under a specified output base directory.")
    parser.add_argument("db_name", help="InfluxDB database (Karabo topic)")
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
    parser.add_argument("--write-url", dest="write_url",
                        help="Url for write operations")
    parser.set_defaults(write_url="http://localhost:8086")
    parser.add_argument("--write-user", dest="write_user",
                        help="Username for write operations")
    parser.set_defaults(write_user="")
    parser.add_argument("--write-pwd", dest="write_pwd",
                        help="Password for write operations")
    parser.set_defaults(write_pwd="")
    parser.add_argument("--read-url", dest="read_url",
                        help="Url for read operations")
    parser.set_defaults(read_url="http://localhost:8086")
    parser.add_argument("--read-user", dest="read_user",
                        help="Username for read operations")
    parser.set_defaults(read_user="")
    parser.add_argument("--read-pwd", dest="read_pwd",
                        help="Password for read operations")
    parser.set_defaults(read_pwd="")
    parser.add_argument("--lines-per-write", dest="lines_per_write", type=int,
                        help="Max. number of lines per write request")
    # The default value has been obtained from write tests performed against
    # a Telegraf node acting as an access-point for a cluster of three InfluxDb
    # instances on three different hosts.
    parser.set_defaults(lines_per_write=500)
    parser.add_argument("--write-timeout", dest="write_timeout", type=int,
                        help="Timeout (secs.) for write requests")
    # Tornado has a default request_timeout of 20 secs. and can return http 599
    # errors if the server does not reply within that interval.
    # https://www.tornadoweb.org/en/stable/httpclient.html#exceptions
    # The default has been set to 40 because some 599 http errors have occurred
    # during tests in the environment with a Telegraf access point for writing
    # to three Influx instances on the back.
    parser.set_defaults(write_timeout=40)
    parser.add_argument("--dry_run", dest="dry_run", action="store_true")
    parser.add_argument("--quiet", dest="prints_on", action="store_false")
    parser.set_defaults(dry_run=False)
    args = parser.parse_args()
    o = DlMigrator(**vars(args))
    get_event_loop().run_until_complete(o.run())
