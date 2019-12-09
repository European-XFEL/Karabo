#!/usr/bin/env python
import argparse
import base64
import os.path as op
import numpy as np
import re

from asyncio import get_event_loop

from karabo.influxdb.client import InfluxDbClient
from karabo.influxdb.dlutils import (
    device_id_from_path, escape_measurement,
    escape_tag_field_key, format_line_protocol_body
)
from karabo.native import decodeXML, encodeBinary


class DlRaw2Influx():
    def __init__(self, path, topic, host, port=8086, device_id=None,
                 dry_run=True, prints_on=True):
        self.raw_path = path
        self.dry_run = dry_run
        self.device_id = device_id
        self.chunk_queries = 8000 # optimal values are between 5k and 10k
        self.prints_on = prints_on
        self.db_client = InfluxDbClient(host=host, port=port, db=topic)
        self.db_name = topic

        if self.device_id is None:  # infer device_id from directory structure
            self.device_id = device_id_from_path(self.raw_path)

    async def run(self):
        if self.prints_on:
            print("Connecting to {}:{} --- raw file: {}"
                  "...".format(self.db_client.host,
                               self.db_client.port,
                               self.raw_path))
        dbs = await self.db_client.get_dbs()
        if self.db_name not in dbs:
            await self.db_client.create_db(self.db_name)

        if self.prints_on:
            print("Feeding to influxDB file {} ...".format(self.raw_path))

        data = []
        with open(self.raw_path, 'r') as fp:

            for i, l in enumerate(fp):
                line_fields = self.parseRaw(l, i)
                if line_fields:
                    # line has been parsed successfully
                    safe_m = escape_measurement(self.device_id)
                    safe_user = escape_tag_field_key(
                                    line_fields["user_name"]
                                )
                    safe_f = escape_tag_field_key(line_fields["name"])
                    safe_v = escape_tag_field_key(line_fields["value"])

                    if len(safe_v) > 0:
                        data.append(
                            '"{m}",user="{user}" "{f}"={v},_tid="{tid}" {t}'
                                .format(m=safe_m,
                                        user=safe_user,
                                        f=safe_f,
                                        v=safe_v,
                                        tid=line_fields["train_id"],
                                        t=line_fields["timestamp"]))

                    upper_flag = line_fields["flag"].upper()
                    if upper_flag in ("LOGIN", "LOGOUT"):
                        # A device event to be saved- user is always
                        # saved to satisfy InfluxDB's requirement of having
                        # at least one key per line data point
                        event_type = '+LOG' if upper_flag == 'LOGIN' else '-LOG'
                        data.append('{m}__EVENTS,type={e} user="{user}" {t}'
                                    .format(m=safe_m,
                                            e=event_type,
                                            user=safe_user,
                                            t=line_fields["timestamp"]))
                    if (data and (i+1) % self.chunk_queries == 0):
                        if not self.dry_run:
                            r = await self.db_client.write(
                                format_line_protocol_body(data)
                            )
                            if self.prints_on:
                                self.print_write_result(
                                    r.code == 204, len(data))
                        data = []

            if not self.dry_run:
                r = await self.db_client.write(format_line_protocol_body(data))
                if self.prints_on:
                    self.print_write_result(r.code == 204, len(data))

    def print_write_result(self, success, num_of_log_lines):
        if success:
            print("{} log lines written successfully to InfluxDb!"
                  .format(num_of_log_lines))
        else:
            print("ERROR: failed to write {} log lines to InfluxDb."
                  .format(num_of_log_lines))

    def parseRaw(self, line, i):
        """
        :param line: a line of datalogger raw file with format
        ts iso8601|epoch ts(s)|train_id|name|type|value|user|flag
        :return: a dictionary with the fields in the line of the
        raw file.

        To deal with influx DB types, the KARABO type is appended
        to the name of the field after an '-' (hifen).

        Vector are left as comma separated ascii strings. 
        """
        tokens = line.split('|')
        line_fields = {}
        try:
            timestamp_s = tokens[1].strip()
            train_id = tokens[2].strip()
            name = tokens[3].strip()
            ktype = tokens[4].strip()
            value = tokens[5].strip()
            user_name = tokens[6].strip()
            flag = tokens[7].strip()
        except IndexError:
            if self.prints_on:
                print("Unable to parse line number {}: '{}' "
                      "in file {}".format(i, line.strip(), self.raw_path))
            return line_fields

        try:
            timestamp_ns = np.int(np.float(timestamp_s) * 1E9)
            train_id = np.uint32(train_id)
        except ValueError:
            if self.prints_on:
                print("Unable to parse timestamp/train_id: '{}' "
                      "in file {}".format(line.strip(), self.raw_path))
            return line_fields

        if value == 'nan' or value == "-nan" or value == "inf":
            if self.prints_on:
                print("{} unsupported in influxDB "
                      "skipping line: '{}'".format(value, line.strip()))
            return line_fields

        if ktype == "BOOL":
            # name += 'b'
            if value == '1':
                value = 't'
            elif value == '0':
                value = 'f'
            else:
                if self.prints_on:
                    print("Bool parameter with undefined value, '{}'. "
                          "skipping line: '{}'".format(value, line.strip()))
                return line_fields
        elif ktype in ("INT8", "UINT8", "INT16", "UINT16", "INT32", "UINT32",
                       "INT64"):
            value = str(value) + "i"
        elif ktype in ("STRING", "VECTOR_STRING", "BYTE_ARRAY", "UINT64"):
            value = '"' + value.replace('\\', '\\\\').replace('"', r'\"') + '"'
        elif ktype == "VECTOR_HASH":
            # `value` is a string representing the XML serialization of
            # we need to decode it and 
            content = decodeXML(value)
            binary = encodeBinary(content)
            value = '"' + base64.b64encode(binary).decode('utf-8') + '"'
        elif ktype.startswith("VECTOR_"):
            value = '"{}"'.format(value)
        if len(ktype) > 0:
            name = "{}-{}".format(name, ktype)

        if user_name == "":
            user_name = "."

        line_fields["timestamp"] = int(timestamp_ns // 1e3)
        line_fields["train_id"] = train_id
        line_fields["name"] = name
        line_fields["value"] = value
        line_fields["user_name"] = user_name
        line_fields["flag"] = flag

        return line_fields


