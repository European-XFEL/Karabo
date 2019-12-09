#!/usr/bin/env python
import argparse
import base64
import hashlib
import json
import numpy as np
import os.path as op
import re

from asyncio import get_event_loop

from karabo.influxdb.client import InfluxDbClient
from karabo.influxdb.dlutils import (
    decodeSchemaXML, device_id_from_path, escape_measurement,
    format_line_protocol_body
)
from karabo.native import encodeBinary, Schema


class DlSchema2Influx():
    def __init__(self, path, topic,
                 host, port=8086, device_id=None,
                 chunk_queries=8000,  # optimal values are between 5k and 10k
                 dry_run=True, prints_on=True):
        self.schema_path = path
        self.dry_run = dry_run
        self.device_id = device_id
        self.chunk_queries = chunk_queries
        self.prints_on = prints_on
        self.db_client = InfluxDbClient(host=host, port=port, db=topic)
        self.db_name = topic

    async def run(self):
        if self.prints_on:
            print("Connecting to {}:{} --- schema file: {}"
                  "...".format(self.db_client.host,
                               self.db_client.port,
                               self.schema_path))

        dbs = await self.db_client.get_dbs()
        if self.db_name not in dbs:
            await self.db_client.create_db(self.db_name)
            
        if self.device_id is None:  # infer device_id from directory structure
            self.device_id = device_id_from_path(self.schema_path)

        if self.prints_on:
            print("Feeding to influxDB file {} ...".format(self.schema_path))

        data = []
        schema_digests = set()
        with open(self.schema_path, 'r') as fp:

            for i, l in enumerate(fp):

                line_fields = self.parse_schema(l)

                if line_fields:
                    # line has been parsed successfully
                    # save the event entry for the schema update
                    classId, xml = line_fields['schema'].split(':', 1)
                    content = Schema(classId)
                    content.hash = decodeSchemaXML(xml)
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
                        # the schema has not been included yet in this batch of
                        # schemas to save; see if it is already in the database
                        qry = "select count(*) from \"{m}__SCHEMAS\" where digest='{d}'".format(
                            m=safe_m,
                            d=digest
                        )
                        qry_args = {"db": self.db_name, "q": qry}
                        rs = await self.db_client.query(qry_args)
                        rs_body = rs.body.decode("utf-8")
                        rs_obj = json.loads(rs_body)
                        if "series" not in rs_obj['results'][0]:
                            # the schema is not yet in the database;
                            # include it in the batch to be saved.
                            sch_b = base64.b64encode(b_schema).decode('utf-8')
                            data.append(
                                '{m}__SCHEMAS,digest="{d}" schema="{sch}" {t}'
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
                        if self.prints_on:
                            self.print_write_result(r.code == 204)
                data = []

    def print_write_result(self, success):
        if success:
            print("One line written to InfluxDb!")
        else:
            print("ERROR: failed to write to InfluxDb.")

    def parse_schema(self, line):
        """
        :param line: a line of schema text file with format
        seconds|fractional_seconds|train_id|text_serialized_schema
        :return: a dictionary with the fields in the line of the
        schema file.
        """
        tokens = line.split(' ', 3)
        line_fields = {}
        try:
            seconds = tokens[0].strip()
            frac_seconds = tokens[1].strip()
            train_id = tokens[2].strip()
            schema = tokens[3].strip()
        except IndexError:
            if self.prints_on:
                print("Unable to parse line: '{}' "
                      "in file {}".format(line.strip(), self.schema_path))
            return line_fields

        try:
            timestamp_ns = np.int(np.float(seconds)*1E9 + np.float(frac_seconds)/1E9)
            train_id = np.uint32(train_id)
        except ValueError:
            if self.prints_on:
                print("Unable to parse timestamp/train_id: '{}' "
                      "in file {}".format(line.strip(), self.schema_path))
            return line_fields

        line_fields["timestamp"] = int(timestamp_ns//1000)
        line_fields["train_id"] = train_id
        line_fields["schema"] = schema

        return line_fields
