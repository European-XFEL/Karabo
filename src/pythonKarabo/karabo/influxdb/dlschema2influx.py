#!/usr/bin/env python
import base64
import hashlib
import json
import os
import os.path as op
import time
from urllib.parse import urlparse

import numpy as np
# we use karathon here to ensure serialization compatiblity between
# the loggers running in C++ and the migration script
from karathon import BinarySerializerSchema, TextSerializerSchema

from karabo.influxdb.client import InfluxDbClient
from karabo.influxdb.dlutils import device_id_from_path, escape_measurement

PROCESSED_SCHEMAS_FILE_NAME = '.processed_schemas.txt'


class DlSchema2Influx():
    def __init__(self, schema_path, topic,
                 read_user, read_pwd, read_url,
                 write_user, write_pwd, write_url,
                 device_id, output_dir, workload_id,
                 write_timeout, dry_run):
        self.schema_path = schema_path
        self.db_name = topic
        self.output_dir = output_dir
        self.device_id = device_id
        self.dry_run = dry_run
        self.workload_id = workload_id

        url_read_parts = urlparse(read_url)  # throws for invalid urls
        read_protocol = url_read_parts.scheme
        read_port = url_read_parts.port
        read_host = url_read_parts.hostname

        url_write_parts = urlparse(write_url)  # throws for invalid urls
        write_protocol = url_write_parts.scheme
        write_port = url_write_parts.port
        write_host = url_write_parts.hostname

        self.read_client = InfluxDbClient(
            host=read_host, port=read_port,
            protocol=read_protocol, user=read_user,
            password=read_pwd, db=self.db_name,
        )
        self.write_client = InfluxDbClient(
            host=write_host, port=write_port,
            protocol=write_protocol, user=write_user,
            password=write_pwd, db=self.db_name,
            request_timeout=write_timeout
        )

        if self.device_id is None:  # infers device_id from directory structure
            self.device_id = device_id_from_path(self.schema_path)

        # Derives the proc_out_path and part_proc_out_path using output_dir
        # as the base path and the device_id as the final directory in the
        # path.
        if self.output_dir:
            self.proc_out_path = op.join(
                self.output_dir,
                'processed',
                self.device_id,
                f'{op.basename(self.schema_path)}.ok')
            self.part_proc_out_path = op.join(
                self.output_dir,
                'part_processed',
                self.device_id,
                f'{op.basename(self.schema_path)}.err')
            self.proc_schemas_path = op.join(
                self.output_dir, PROCESSED_SCHEMAS_FILE_NAME)
        else:
            self.proc_out_path = None
            self.part_proc_out_path = None
            self.proc_schemas_path = None

    async def run(self):
        data = []
        schema_digests = set()
        part_proc = False  # indicates the file has been partially processed.
        stats = {
            'start_time': time.time(),
            'end_time': None,
            'elapsed_secs': 0.0,
            'insert_rate': 0.0,
            'lines_written': 0,
            'write_retries': [],
        }
        schema_update_epoch = op.getmtime(self.schema_path)
        karathon_text_ser = TextSerializerSchema.create("Xml")
        karathon_bin_ser = BinarySerializerSchema.create("Bin")
        with open(self.schema_path, 'r') as fp:

            for i, l in enumerate(fp):
                try:
                    line_fields = self.parse_schema(l)

                    if line_fields:
                        # line has been parsed successfully
                        # save the event entry for the schema update
                        # use karathon to ensure the same code base as for
                        # live logging when serializing.
                        xml = line_fields['schema']
                        karathon_schema = karathon_text_ser.load(xml)
                        b_schema = karathon_bin_ser.save(karathon_schema)
                        digest = hashlib.sha1(b_schema).hexdigest()
                        safe_m = escape_measurement(self.device_id)
                        data.append(
                            '{m}__EVENTS,type="SCHEMA" schema_digest="{d}" {t}'
                            .format(
                                m=safe_m,
                                d=digest,
                                t=line_fields["timestamp"]
                            )
                        )
                        if digest not in schema_digests:
                            # the schema has not been included yet in this
                            # batch of schemas to save; see if it is already
                            # in the database.
                            qry = (
                                f"select count(*) from \"{safe_m}__SCHEMAS\""
                                f" where digest='\"{digest}\"'"
                            )
                            qry_args = {"db": self.db_name, "q": qry}
                            rs = await self.read_client.query(qry_args)
                            rs_body = rs.body.decode("utf-8")
                            rs_obj = json.loads(rs_body)
                            if "series" not in rs_obj['results'][0]:
                                # the schema is not yet in the database;
                                # include it in the batch to be saved.
                                sch_b = (
                                    base64.b64encode(b_schema).decode('utf-8')
                                )
                                data.append(
                                    '{m}__SCHEMAS,digest="{d}" '
                                    'schema="{sch}" {t}'
                                    .format(
                                        m=safe_m,
                                        d=digest,
                                        sch=sch_b,
                                        t=line_fields["timestamp"]
                                    )
                                )

                            schema_digests.add(digest)

                    if not self.dry_run:
                        for line in data:
                            r = await self.write_client.write_with_retry(line)
                            if r['code'] != 204:
                                # Error saving the schema; raise.
                                raise Exception(
                                    "Error saving schema in Influx: "
                                    "{} - {}\nWrite retries: {}\nContent:\n"
                                    "{} ...\n\n".format(
                                        r['code'], r['reason'], r['retried'],
                                        line[:320]))
                            if r['retried']:
                                stats['write_retries'].append(r['retried'])
                    data = []
                    stats['lines_written'] = i+1

                except Exception as exc:
                    # Handles an error by logging it in a .err file in the
                    # partially processed directory if there's an output_dir
                    # defined. Otherwise just re-raises the exception and
                    # keep the previously existing behavior of stopping on
                    # first error.
                    part_proc = True
                    if self.part_proc_out_path and not self.dry_run:
                        if not op.exists(op.dirname(self.part_proc_out_path)):
                            os.makedirs(op.dirname(self.part_proc_out_path))
                        with open(self.part_proc_out_path, 'a') as err:
                            err.write("Error at line {}: {}\n".format(i, exc))
                    elif not self.part_proc_out_path:
                        raise

        # Registers successful processing of schema file
        stats['end_time'] = time.time()
        stats['elapsed_secs'] = stats['end_time']-stats['start_time']
        stats['insert_rate'] = stats['lines_written']/(stats['elapsed_secs'])
        if self.proc_out_path and not self.dry_run and not part_proc:
            if not op.exists(op.dirname(self.proc_out_path)):
                os.makedirs(op.dirname(self.proc_out_path))
            with open(self.proc_out_path, 'w') as ok_file:
                ok_file.write(json.dumps(stats))
            with open(self.proc_schemas_path, 'a') as proc_schemas_file:
                proc_schemas_file.write(
                    f'{schema_update_epoch}|{self.workload_id}|'
                    f'{self.schema_path}\n')

        return not part_proc  # True if fully successfully processed.

    def parse_schema(self, line):
        """
        :param line: a line of schema text file with format
        seconds|fractional_seconds|train_id|text_serialized_schema
        :return: a dictionary with the fields in the line of the
        schema file.
        """
        if len(line.strip()) == 0:
            # Nothing to parse
            return None

        line_fields = {}
        tokens = line.split(' ', 3)
        try:
            secs = tokens[0].strip()
            frac_secs = tokens[1].strip()
            train_id = tokens[2].strip()
            schema = tokens[3].strip()
        except IndexError:
            raise Exception(
                "Unable to parse line: '{}'".format(line.strip()))

        try:
            timestamp_ns = np.int(np.float(secs)*1E9 + np.float(frac_secs)/1E9)
            train_id = np.uint32(train_id)
        except ValueError:
            raise Exception(
                "Unable to parse timestamp ({}.{}) and/or "
                "train_id ({})."
                .format(secs, frac_secs, train_id))

        line_fields["timestamp"] = int(timestamp_ns//1000)
        line_fields["train_id"] = train_id
        line_fields["schema"] = schema

        return line_fields
