#!/usr/bin/env python
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
import datetime
import json
import os
import re
import shutil
import time
from asyncio import gather, get_event_loop

from karabo.influx_db import (
    PROCESSED_RAWS_FILE_NAME, PROCESSED_SCHEMAS_FILE_NAME, DlRaw2Influx,
    DlSchema2Influx)


class DlMigrator():
    def __init__(self, db_name, input_dir, output_dir, write_url, write_user,
                 write_pwd, read_url, read_user, read_pwd, lines_per_write,
                 write_timeout, concurrent_tasks, start=None, end=None,
                 dry_run=True):
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
        self.lines_per_write = lines_per_write
        self.write_timeout = write_timeout
        self.concurrent_tasks = concurrent_tasks
        self.prev_run_file = os.path.join(output_dir, '.previous_run.txt')
        self.run_info_file = os.path.join(output_dir, '.run_info.json')
        self.processed_raws = {}
        self.processed_schemas = {}
        self.n_processed = 0
        self.n_part_processed = 0
        self.n_skipped = 0
        self.start_date = start
        self.end_date = end

    def _get_prev_run_num(self):
        """Retrieves the number of the previous run."""
        prev_run_num = 0
        if os.path.exists(self.prev_run_file):
            with open(self.prev_run_file) as pr_file:
                prev_run_num = int(pr_file.read())  # Invalid file should throw
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
        if (not os.path.exists(proc_path)
                and not os.path.exists(part_proc_path)):
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
        if os.path.isfile(self.run_info_file):
            shutil.move(self.run_info_file,
                        os.path.join(backup_dir, 'run_info.json'))

    def _inc_previous_run(self):
        """Increments (or initializes) the previous run number file."""
        prev_run_num = self._get_prev_run_num()
        if not os.path.exists(self.output_dir):
            os.makedirs(self.output_dir)
        with open(self.prev_run_file, 'w') as pr_file:
            pr_file.write(str(prev_run_num + 1))

    def _load_processed_files(self):
        """Loads the list of processed property and schema files.

        Each line in the files that store those lists has three fields
        separated by a '|' (pipe character). The first field is the last
        modification time (in Unix epoch format) for the processed file when
        it was processed. The second field is the id of the workload to which
        the processed file belonged.
        The third fiekd is the absolute path of the processed file.

        The loaded data is stored in dictionaries that have the processed file
        path as a key and the last modification time as the value.
        """
        proc_raws_path = os.path.join(self.output_dir,
                                      PROCESSED_RAWS_FILE_NAME)
        if os.path.exists(proc_raws_path):
            with open(proc_raws_path) as proc_raws_file:
                for _, file_record in enumerate(proc_raws_file):
                    fields = file_record.split('|')
                    last_update = float(fields[0])
                    file_name = fields[2].strip()
                    self.processed_raws[file_name] = last_update

        proc_schemas_path = os.path.join(self.output_dir,
                                         PROCESSED_SCHEMAS_FILE_NAME)
        if os.path.exists(proc_schemas_path):
            with open(proc_schemas_path) as proc_schemas_file:
                for _, file_record in enumerate(proc_schemas_file):
                    fields = file_record.split('|')
                    last_update = float(fields[0])
                    file_name = fields[2].strip()
                    self.processed_schemas[file_name] = last_update

    def _split_workload(self):
        """Split the workload in a number of approximately equally sized parts.
        The number of parts is the number of concurrent migration tasks.

        return: list of workloads. Each workload is a list of tuples with the
        path of a file to be migrated and the deviceId associated to that file.
        """
        print('Splitting workload among {} concurrent tasks ...'
              .format(self.concurrent_tasks))

        full_workload = []
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
            for file_name in files:
                if (not re.match(r"^archive_[0-9]+\.txt$", file_name) and
                        file_name != "archive_schema.txt"):
                    continue
                file_path = os.path.join(dir_, file_name)
                last_update = os.path.getmtime(file_path)
                if (self.processed_schemas.get(file_path) == last_update or
                        self.processed_raws.get(file_path) == last_update):
                    print(f"'{file_path}' already migrated: skip.")
                    self.n_skipped += 1
                    continue
                if ((self.start_date and last_update < self.start_date) or
                        (self.end_date and last_update > self.end_date)):
                    print(f"'{file_path}' outside date range.")
                    self.n_skipped += 1
                    continue

                full_workload.append((device_id, file_path, last_update))

                # sort list by descending access time
                # (newer files are to be processed first)
                full_workload.sort(key=lambda e: e[2], reverse=True)

        workloads = [[] for i in range(self.concurrent_tasks)]
        for i in range(0, len(full_workload)):
            workloads[i % self.concurrent_tasks].append(full_workload[i])

        print(f'... workload of {len(full_workload)} files splitted.')

        return workloads

    async def _migrate_workload(self, workload_id, workload):
        """Migrates a given workload of text log files to InfluxDb.

        workload_id: identifier of workload to be migrated
                     used as part of the migration progress info.
        workload: list of paths of files to be migrated.
        """
        for device_id, file_path, _ in workload:
            file_name = os.path.basename(file_path)
            success = False
            print(f"[wk-{workload_id}] - Migrating '{file_path}' ...")
            if file_name == 'archive_schema.txt':
                success = await self.insert_schema(file_path, self.output_dir,
                                                   device_id, workload_id)
            elif re.match(r"^archive_[0-9]+\.txt$", file_name):
                success = await self.insert_archive(file_path, self.output_dir,
                                                    device_id, workload_id)
            else:  # this should not be reached
                self.n_skipped += 1
                print(" SKIPPED - WARNING: '{}' should not be in WORKLOAD!!"
                      .format(file_path))
                continue
            if success:
                self.n_processed += 1
                print("[wk-{}] - ... SUCCESS for '{}'".format(workload_id,
                                                              file_path))
            else:
                self.n_part_processed += 1
                print("[wk-{}] - FAILED for ... '{}'".format(workload_id,
                                                             file_path))

    def _save_run_info(self, start_time, workloads_info, end_time=''):
        """Saves information about current run in a file.

        The information is saved in json format and has one property for each
        of the parameters described below.

        start_time: unix epoch of when the migration started.
        workloads_info: list of tuples with the 'workload_id' in its first
                        position and the number of files in the workload in
                        its second position.
        end_time: unix epoch of when the migration started - should not be
        provided if the migration is still on-going.
        """
        info = {
            'start_time': start_time,
            'end_time': end_time,
            'workloads_info': workloads_info
        }
        with open(self.run_info_file, 'w') as inf_file:
            inf_file.write(json.dumps(info))

    async def run(self):
        start_time = time.time()
        self.n_processed = 0
        self.n_part_processed = 0
        self.n_skipped = 0

        self._load_processed_files()
        self._backup_previous_run()
        self._inc_previous_run()

        workloads = self._split_workload()
        workloads_info = []
        wk_id = 0
        wk_futures = []
        for workload in workloads:
            if workload:
                workloads_info.append((wk_id, len(workload)))
                wk_futures.append(self._migrate_workload(wk_id, workload))
                wk_id += 1
        self._save_run_info(start_time, workloads_info)
        await gather(*wk_futures)
        end_time = time.time()
        self._save_run_info(start_time, workloads_info, end_time)

        print("Migration job finished:")
        print(f"Files skipped: {self.n_skipped}")
        print(f"Files processed successfully: {self.n_processed}")
        print(f"Files with migration errors:  {self.n_part_processed}")

    async def insert_archive(self, path, output_dir, device_id, workload_id):
        try:
            conf2db = DlRaw2Influx(
                raw_path=path, output_dir=output_dir,
                topic=self.db_name, user=self.write_user,
                password=self.write_pwd, url=self.write_url,
                dry_run=self.dry_run, device_id=device_id,
                lines_per_write=self.lines_per_write,
                write_timeout=self.write_timeout,
                workload_id=workload_id)
        except Exception as exc:
            print("Error creating property file converter:\n"
                  "File to be converted: {}\n"
                  "Error message: {}".format(path, exc))
            return False
        return await conf2db.run()

    async def insert_schema(self, path, output_dir, device_id, workload_id):
        try:
            schema2db = DlSchema2Influx(
                schema_path=path, topic=self.db_name,
                read_user=self.read_user, read_pwd=self.read_pwd,
                read_url=self.read_url, write_user=self.write_user,
                write_pwd=self.write_pwd, write_url=self.write_url,
                device_id=device_id, output_dir=output_dir,
                write_timeout=self.write_timeout, workload_id=workload_id,
                dry_run=self.dry_run)
        except Exception as exc:
            print("Error creating schema file converter:\n"
                  "File to be converted: {}\n"
                  "Error message: {}".format(path, exc))
            return False
        return await schema2db.run()


def main():
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
                             "in those directories will be processed. A common"
                             " value for this argument would be "
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
    parser.add_argument("--concurrent-tasks", dest="concurrent_tasks",
                        type=int, help="Number of concurrent migration tasks")
    parser.set_defaults(concurrent_tasks=4)

    def parse_date(s):
        if s:
            return datetime.datetime.strptime(s, '%Y-%m-%d').timestamp()
        return s

    parser.add_argument('--start', type=parse_date,
                        help="Start date of data to be migrated (%Y-%m-%d)")
    parser.set_defaults(start=None)

    parser.add_argument('--end', type=parse_date,
                        help="End date of data to be migrated (%Y-%m-%d)")
    parser.set_defaults(end=None)

    parser.add_argument("--dry_run", dest="dry_run", action="store_true")
    parser.set_defaults(dry_run=False)

    args = parser.parse_args()
    # check dates are sane
    if args.start:
        args.start = args.start
    if args.end:
        args.end = args.end
    if args.start and args.end and args.start > args.end:
        raise AttributeError("Start date must be in the past of end date!")

    o = DlMigrator(**vars(args))
    get_event_loop().run_until_complete(o.run())


if __name__ == "__main__":
    main()
