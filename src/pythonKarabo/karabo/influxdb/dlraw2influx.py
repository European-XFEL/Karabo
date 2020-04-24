#!/usr/bin/env python
import argparse
import base64
import json
import os
import os.path as op
import numpy as np
import re
import shutil
import time

from asyncio import get_event_loop
from urllib.parse import urlparse

from karabo.influxdb.client import InfluxDbClient
from karabo.influxdb.dlutils import (
    device_id_from_path, escape_measurement,
    escape_tag_field_key, format_line_protocol_body
)
from karabo.native import decodeXML, encodeBinary

PROCESSED_RAWS_FILE_NAME = '.processed_props.txt'


class DlRaw2Influx():
    def __init__(self, raw_path, topic, user, password, url,
                 device_id, output_dir, dry_run, prints_on,
                 lines_per_write, write_timeout):
        self.raw_path = raw_path
        self.db_name = topic
        self.output_dir = output_dir
        self.dry_run = dry_run
        self.device_id = device_id
        self.prints_on = prints_on

        url_parts = urlparse(url)  # throws for invalid urls.
        protocol = url_parts.scheme
        port = url_parts.port
        host = url_parts.hostname

        self.db_client = InfluxDbClient(
            host=host, port=port, db=self.db_name, protocol=protocol,
            user=user, password=password, request_timeout=write_timeout
            )

        self.chunk_queries = lines_per_write

        if self.device_id is None:  # infers device_id from directory structure
            self.device_id = device_id_from_path(self.raw_path)

        # Derives the proc_out_path and part_proc_out_path using output_dir
        # as the base path and the device_id as the final directory in the
        # path.
        if self.output_dir:
            self.proc_out_path = op.join(
                self.output_dir,
                'processed',
                self.device_id,
                op.basename(f'{self.raw_path}.ok'))
            self.part_proc_out_path = op.join(
                self.output_dir,
                'part_processed',
                self.device_id,
                op.basename(f'{self.raw_path}.err'))
            self.proc_props_path = op.join(
                self.output_dir,
                PROCESSED_RAWS_FILE_NAME)
        else:
            self.proc_out_path = None
            self.part_proc_out_path = None
            self.proc_props_path = None

    async def run(self):
        if self.prints_on:
            print("Connecting to host: {} ..."
                  .format(self.db_client.host))

        if self.prints_on:
            print("Feeding to influxDB file {} ...".format(self.raw_path))

        data = []
        part_proc = False  # indicates the file has been partially processed.
        stats = {
            'start_time': time.time(),
            'end_time': None,
            'elapsed_secs': 0.0,
            'insert_rate': 0.0,
            'lines_written': 0,
            'write_retries': [],
        }
        with open(self.raw_path, 'r') as fp:

            for i, l in enumerate(fp):
                try:
                    line_fields = self.parse_raw(l)
                    if line_fields:
                        # line has been parsed successfully
                        safe_m = escape_measurement(self.device_id)
                        safe_user = escape_tag_field_key(
                                        line_fields["user_name"]
                                    )
                        safe_f = escape_tag_field_key(line_fields["name"])
                        safe_v = line_fields["value"]

                        if len(safe_v) > 0:
                            data.append(
                                '{m},user="{user}" "{f}"={v},_tid="{tid}" {t}'
                                .format(m=safe_m, user=safe_user,
                                        f=safe_f, v=safe_v,
                                        tid=line_fields["train_id"],
                                        t=line_fields["timestamp"]))

                        up_flag = line_fields["flag"].upper()
                        if up_flag in ("LOGIN", "LOGOUT"):
                            # A device event to be saved- user is always
                            # saved to satisfy InfluxDB's requirement of having
                            # at least one key per line data point
                            evt_type = '+LOG' if up_flag == 'LOGIN' else '-LOG'
                            data.append('{m}__EVENTS,type={e} user="{user}" {t}'
                                        .format(m=safe_m, e=evt_type,
                                                user=safe_user,
                                                t=line_fields["timestamp"]))
                        if data and (i+1) % self.chunk_queries == 0:
                            lin_proto_data = format_line_protocol_body(data)
                            data = []
                            if not self.dry_run:
                                r = (
                                      await self.db_client.write_with_retry(
                                                                lin_proto_data)
                                )
                                if r['code'] != 204:
                                    raise Exception(
                                        "Error writing line protocol: "
                                        "{} - {}\nWrite retries: {}\nContent:\n"
                                        "{} ...\n\n".format(
                                            r['code'], r['reason'],
                                            r['retried'],
                                            lin_proto_data[:320]))
                                if r['retried']:
                                    stats['write_retries'].append(r['retried'])
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

            if not self.dry_run and data:
                try:
                    line_protocol_data = format_line_protocol_body(data)
                    r = (
                          await self.db_client.write_with_retry(
                                                            line_protocol_data)
                    )
                    if r['code'] != 204:
                        raise Exception(
                                "{} - {}\nWrite retries: {}\n".format(
                                    r['code'], r['reason'], r['retried']))
                    if r['retried']:
                        stats['write_retries'].append(r['retried'])
                except Exception as exc:
                    part_proc = True
                    if self.part_proc_out_path and not self.dry_run:
                        if not op.exists(op.dirname(self.part_proc_out_path)):
                            os.makedirs(op.dirname(self.part_proc_out_path))
                        with open(self.part_proc_out_path, 'a') as err:
                            err.write("Error writing line protocol: {}\n{} ..."
                                      "\n\n".format(
                                          exc, line_protocol_data[:320]))
                    elif not self.part_proc_out_path:
                        raise

        # Registers successful processing of properties file.
        stats['end_time'] = time.time()
        stats['elapsed_secs'] = stats['end_time']-stats['start_time']
        stats['insert_rate'] = stats['lines_written']/(stats['elapsed_secs'])
        if self.proc_out_path and not self.dry_run and not part_proc:
            if not op.exists(op.dirname(self.proc_out_path)):
                os.makedirs(op.dirname(self.proc_out_path))
            with open(self.proc_out_path, 'w') as ok_file:
                ok_file.write(json.dumps(stats))
            with open(self.proc_props_path, 'a') as proc_props_file:
                proc_props_file.write(f'{self.raw_path}\n')

        return not part_proc  # True if fully successfully processed.

    def parse_raw(self, line):
        """
        :param line: a line of datalogger raw file with format
        ts iso8601|epoch ts(s)|train_id|name|type|value|user|flag
        :return: a dictionary with the fields in the line of the
        raw file.

        To deal with influx DB types, the KARABO type is appended
        to the name of the field after an '-' (hifen).

        Vector are left as comma separated ascii strings.
        """
        if len(line.strip()) == 0:
            # Nothing to parse
            return None

        line_regex = (
            r"^([TZ0-9\.]+)\|([0-9\.]+)\|([0-9]+)\|(.+)\|([0-9A-Z_]*)\|(.*)\|"
            r"([a-z0-9_]*)\|([A-Z]+)$"
        )

        line_fields = {}
        try:
            matches = re.search(line_regex, line)
            timestamp_s = matches.group(2)
            train_id = matches.group(3)
            name = matches.group(4)
            ktype = matches.group(5)
            value = matches.group(6)
            user_name = matches.group(7)
            flag = matches.group(8)
        except Exception as exc:
            raise Exception(
                      "\n\tLine: '{}'\n\tMessage: '{}'"
                      .format(line.strip(), exc))

        try:
            timestamp_ns = np.int(np.float(timestamp_s) * 1E9)
            train_id = np.uint32(train_id)
        except ValueError:
            raise Exception(
                      "Unable to parse timestamp ({}) and/or "
                      "train_id ({}).".format(timestamp_s, train_id))

        if value == 'nan' or value == "-nan" or value == "inf":
            raise Exception(
                      "{} unsupported in influxDB "
                      "skipping line: '{}'".format(value, line.strip()))

        if ktype == "BOOL":
            # name += 'b'
            if value == '1':
                value = 't'
            elif value == '0':
                value = 'f'
            else:
                raise Exception(
                          "Bool parameter with undefined value, '{}'. "
                          "skipping line: '{}'".format(value, line.strip()))
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
