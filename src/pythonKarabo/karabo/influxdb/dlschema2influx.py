#!/usr/bin/env python
import argparse
import base64
import hashlib
import json
import numpy as np
import os
import os.path as op
import re
import shutil

from asyncio import get_event_loop

from karabo.influxdb.client import InfluxDbClient
from karabo.influxdb.dlutils import (
    device_id_from_path, escape_measurement, format_line_protocol_body
)
from karabo.native import encodeBinary, Schema, decodeXML


class DlSchema2Influx():
    def __init__(self, schema_path, topic,
                 user, password, protocol,
                 host, port=8086, device_id=None,
                 output_dir='/tmp', dry_run=True,
                 prints_on=True, use_gateway=False):
        self.schema_path = schema_path
        self.output_dir = output_dir
        self.dry_run = dry_run
        self.device_id = device_id
        self.chunk_queries = 8000  # optimal values are between 5k and 10k
        self.prints_on = prints_on
        self.db_client = InfluxDbClient(
                             host=host, port=port, db=topic,
                             protocol=protocol, user=user, password=password,
                             use_gateway=use_gateway
                         )
        self.db_name = topic
        self.authenticated_access = len(user) > 0

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
                op.basename(self.schema_path))
            self.part_proc_out_path = op.join(
                self.output_dir,
                'part_processed',
                self.device_id,
                op.basename(self.schema_path))
        else:
            self.proc_out_path = None
            self.part_proc_out_path = None

    async def run(self):
        if self.prints_on:
            print("Connecting to host: {}..."
                  .format(self.db_client.host))

        if self.prints_on:
            print("Feeding to influxDB file {} ...".format(self.schema_path))

        data = []
        schema_digests = set()
        part_proc = False  # indicates the file has been partially processed.
        with open(self.schema_path, 'r') as fp:

            for i, l in enumerate(fp):
                try:
                    line_fields = self.parse_schema(l)

                    if line_fields:
                        # line has been parsed successfully
                        # save the event entry for the schema update
                        class_id, xml = line_fields['schema'].split(':', 1)
                        content = Schema(class_id)
                        content.hash = decodeXML(xml)
                        b_schema = encodeBinary(content)
                        digest = hashlib.sha1(b_schema).hexdigest()
                        safe_m = escape_measurement(self.device_id)
                        data.append(
                            '{m}__EVENTS,type=SCHEMA digest="{d}" {t}'
                            .format(
                                m=safe_m,
                                d=digest,
                                t=line_fields["timestamp"]
                            )
                        )
                        if digest not in schema_digests:
                            # the schema has not been included yet in this batch
                            # of schemas to save; see if it is already in the
                            # database.
                            qry = (
                                "select count(*) from \"{m}__SCHEMAS\" "
                                "where digest='{d}'".format(m=safe_m, d=digest)
                            )
                            qry_args = {"db": self.db_name, "q": qry}
                            rs = await self.db_client.query(qry_args)
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
                            r = await self.db_client.write(line)
                            if r.code != 204:
                                # Error saving the schema; raise.
                                raise Exception(
                                   "Error saving schema in Influx: {} - {}"
                                   .format(r.code, r.reason))
                    data = []

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
                        err_path = self.part_proc_out_path + ".err"
                        file_mode = 'a' if op.exists(err_path) else 'w'
                        with open(err_path, file_mode) as err:
                            err.write("Error at line {}: {}\n".format(i, exc))
                    elif not self.part_proc_out_path:
                        raise

        # Copies the file either to the partially processed or processed dir, if
        # the corresponding output paths are defined.
        if self.proc_out_path and not self.dry_run and not part_proc:
            if not op.exists(op.dirname(self.proc_out_path)):
                os.makedirs(op.dirname(self.proc_out_path))
            shutil.copy(self.schema_path, self.proc_out_path)
        elif self.part_proc_out_path and not self.dry_run:
            # No need to check for output directory existence in here; it has
            # already been created to log the error.
            shutil.copy(self.schema_path, self.part_proc_out_path)

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
