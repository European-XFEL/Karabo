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
import base64
import json
import os
import os.path as op
import re
import struct
import time
from urllib.parse import urlparse

import numpy as np

from karabo.influx_db.client import InfluxDbClient
from karabo.influx_db.dlutils import (
    device_id_from_path, escape_measurement, escape_tag_field_key,
    format_line_protocol_body)
from karabo.native import decodeXML, encodeBinary

PROCESSED_RAWS_FILE_NAME = '.processed_props.txt'


class LineBuffer():
    def __init__(self, user=None, tid=0, ts=None):
        self.line_key = (user, int(tid), ts)
        self.value_dict = {}

    def __bool__(self):
        return self.line_key != (None, 0, None) or bool(self.value_dict)

    def __setitem__(self, key, value):
        self.value_dict[key] = value

    def to_line_protocol(self, measurement):
        if self.line_key[2] is None:
            return ""
        kv = ','.join((f'{k}={v}' for k, v in self.value_dict.items()))
        user, tid, ts = self.line_key
        tid_field = "" if tid <= 0 else f',_tid={tid}i'
        return f'{measurement},karabo_user="{user}" {kv}{tid_field} {ts}'

    def __repr__(self):
        return f"line_key {self.line_key} {self.value_dict}"


class KnownRawIssueException(Exception):
    """A known issue has been found on an input raw property file."""


class DlRaw2Influx():
    def __init__(self, raw_path, topic, user, password, url,
                 device_id, output_dir, dry_run,
                 lines_per_write, write_timeout, workload_id):
        self.raw_path = raw_path
        self.db_name = topic
        self.output_dir = output_dir
        self.dry_run = dry_run
        self.device_id = device_id
        self.workload_id = workload_id

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
        self.stats = {}

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
            self.warn_out_path = op.join(
                self.output_dir,
                'part_processed',
                self.device_id,
                op.basename(f'{self.raw_path}.warn'))
            self.proc_props_path = op.join(
                self.output_dir,
                PROCESSED_RAWS_FILE_NAME)
        else:
            self.proc_out_path = None
            self.part_proc_out_path = None
            self.proc_props_path = None

        self.buf = None
        self.data = []

    async def run(self):
        part_proc = False  # indicates the file has been partially processed.
        self.stats = {
            'start_time': time.time(),
            'end_time': None,
            'elapsed_secs': 0.0,
            'insert_rate': 0.0,
            'lines_processed': 0,
            'write_retries': [],
        }
        with open(self.raw_path) as fp:
            for i, line in enumerate(fp):
                try:
                    self._process_line(line)
                    await self._send_data()
                except KnownRawIssueException as exc:
                    self._handle_knownRawIssue(exc, i)
                except Exception as exc:
                    part_proc = True
                    self._handle_genericException(exc, i)
            # end of file, flush
            self._flush_buffer()
            try:
                await self._send_data(force_send=True)
            except Exception as exc:
                part_proc = True
                self._handle_genericException(exc, i)
            self._write_end_stats(part_proc)
        return not part_proc

    def _process_line(self, file_line):
        line_fields = self.parse_raw(file_line)
        if not line_fields:
            return
        # line has been parsed successfully
        safe_m = escape_measurement(self.device_id)
        safe_user = escape_tag_field_key(line_fields["user_name"])
        safe_f = escape_tag_field_key(line_fields["name"])
        safe_v = line_fields["value"]
        safe_tid = line_fields["train_id"]
        safe_time = line_fields["timestamp"]

        up_flag = line_fields["flag"].upper()
        if up_flag in ("LOGIN", "LOGOUT"):
            # A device event to be saved- user is always
            # saved to satisfy InfluxDB's requirement of having
            # at least one key per line data point
            evt_type = '+LOG' if up_flag == 'LOGIN' else '-LOG'
            self.data.append(
                f'{safe_m}__EVENTS,type="{evt_type}" '
                f'karabo_user="{safe_user}" {safe_time}'
            )
            self.stats['lines_processed'] = self.stats['lines_processed'] + 1
            return

        if not self.buf:
            # The unique identifier is empty. Initialize it.
            self.buf = LineBuffer(safe_user, safe_tid, safe_time)
        elif self.buf.line_key != (safe_user, safe_tid, safe_time):
            # The record key of this `file_line` does not match the current
            # buffer (e.g. a new timestamp).
            # We append the buffer content to data
            # and updates the record key.
            self._flush_buffer()
            self.buf = LineBuffer(safe_user, safe_tid, safe_time)
        if safe_v:
            self.buf[safe_f] = safe_v
        self.stats['lines_processed'] = self.stats['lines_processed'] + 1

    def _flush_buffer(self):
        """ append a line from buffer to the data queue

        """
        if not self.buf:
            return
        device_id = escape_tag_field_key(self.device_id)
        line = self.buf.to_line_protocol(device_id)
        self.data.append(line)

    async def _send_data(self, force_send=False):
        if self.dry_run:
            return
        if force_send or len(self.data) >= self.chunk_queries:
            line_data = format_line_protocol_body(self.data)
            self.data = []
            r = await self.db_client.write_with_retry(line_data)
            if r['code'] != 204:
                rs = r['reason']
                msg = f"""Error writing line protocol: {r['code']} - {rs}
                    Write retries: {r['retried']}
                    Content:
                    {line_data} ...
                """
                raise Exception(msg)
            if r['retried']:
                self.stats['write_retries'].append(r['retried'])

    def _handle_knownRawIssue(self, exc, line_number):
        """Handles KnownRawIssueExceptions saving as much data as posible

        Known issues in input files should generate .warn files
        in the partially processed directory, but should be
        ignored regarding the partially processed status of the
        input file.
        """
        if self.warn_out_path and not self.dry_run:
            if not op.exists(op.dirname(self.warn_out_path)):
                os.makedirs(op.dirname(self.warn_out_path))
                with open(self.warn_out_path, 'a') as warn:
                    warn.write(f"Known issue. Line {line_number}: {exc}\n")

    def _handle_genericException(self, exc, line_number):
        """Handles a GenericException in file parsing

        log exceptions to an .err file in the
        partially processed directory if the path is defined.
        Otherwise just re-raises the exception and
        keep the previously existing behavior of stopping on
        first error.
        """
        if self.part_proc_out_path and not self.dry_run:
            if not op.exists(op.dirname(self.part_proc_out_path)):
                os.makedirs(op.dirname(self.part_proc_out_path))
            with open(self.part_proc_out_path, 'a') as err:
                err.write(f"Error at line {line_number}: {exc}\n")
        elif not self.part_proc_out_path:
            raise

    def _write_end_stats(self, part_proc):
        """Write Stats To File"""
        raw_update_epoch = op.getmtime(self.raw_path)
        self.stats['end_time'] = time.time()
        elapsed = self.stats['end_time'] - self.stats['start_time']
        self.stats['elapsed_secs'] = elapsed
        self.stats['insert_rate'] = self.stats['lines_processed'] / elapsed
        if self.proc_out_path and not self.dry_run and not part_proc:
            if not op.exists(op.dirname(self.proc_out_path)):
                os.makedirs(op.dirname(self.proc_out_path))
            with open(self.proc_out_path, 'w') as ok_file:
                ok_file.write(json.dumps(self.stats))
            with open(self.proc_props_path, 'a') as proc_props_file:
                proc_props_file.write(
                    f'{raw_update_epoch}|{self.workload_id}|{self.raw_path}\n')

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

        line_regex = r"^([TZ0-9\.]+)\|([0-9\.]+)\|([0-9]+)\|(.+)\|((?:)|(?:[A-Z][0-9A-Z_]+))\|(.*)\|([a-z0-9_]*)\|([A-Z]+)$"  # noqa

        line_fields = {}
        matches = re.search(line_regex, line)
        if matches is None:
            raise KnownRawIssueException(f"Incomplete input line: {line}")
        timestamp_s = matches.group(2)
        train_id = matches.group(3)
        name = matches.group(4)
        ktype = matches.group(5)
        value = matches.group(6)
        user_name = matches.group(7)
        flag = matches.group(8)

        try:
            timestamp_ns = int(float(timestamp_s) * 1E9)
            train_id = np.uint32(train_id)
        except ValueError:
            raise Exception(
                "Unable to parse timestamp ({}) and/or "
                "train_id ({}).".format(timestamp_s, train_id))

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
        elif ktype in ("FLOAT", "DOUBLE"):
            # if the value is not finite, convert it into a string and mangle
            # the type adding an `_INF`
            if value in ("nan", "-inf", "inf", "-nan"):
                ktype = f"{ktype}_INF"
                value = f'"{value}"'
        elif ktype in ("INT8", "UINT8", "INT16", "UINT16", "INT32", "UINT32",
                       "INT64"):
            value = str(value) + "i"
        elif ktype == "UINT64":
            # cast like a INT64
            sv = struct.unpack('l',
                               struct.pack('P', int(f"{int(value):b}", 2)))[0]
            value = str(sv) + "i"
        elif ktype == "STRING":
            escaped = value.replace('\\', '\\\\').replace('"', r'\"')
            value = f'"{escaped}"'
        elif ktype == "VECTOR_HASH":
            # `value` is a string representing the XML serialization of
            # we need to decode it and
            content = decodeXML(value)
            binary = encodeBinary(content)
            value = '"' + base64.b64encode(binary).decode('utf-8') + '"'
        elif ktype == "VECTOR_STRING":
            # `value` is a string that will be reinterpreted as a vector
            # of comma separated strings this will be serialized into a JSON
            # string and encoded using the base64 algorithm in a string
            json_seq = json.dumps(value.split(","), ensure_ascii=False)
            binary = base64.b64encode(json_seq.encode('utf-8'))
            value = f'"{binary.decode("utf-8")}"'
        elif ktype == "VECTOR_STRING_BASE64":
            # VECTOR_STRING_BASE64 is already formatted with the format
            # expected by the influxLogReader for VECTOR_STRING
            # here we put it into quotes to tell influx to treat it as a
            # string and change the ktype to VECTOR_STRING
            ktype = "VECTOR_STRING"
            value = f'"{value}"'
        elif ktype == "VECTOR_UINT8":
            binary = base64.b64decode(value)
            vec = [str(v) for v in struct.unpack('B'*len(binary), binary)]
            value = '"{}"'.format(",".join(vec))
        elif ktype == "BYTE_ARRAY" or ktype.startswith("VECTOR_"):
            value = f'"{value}"'
        elif ktype == "CHAR":
            size = len(value)
            if size > 1:
                raise Exception(
                    f'Char property {name} abnormally long: {value}')
            elif size == 0:
                binary = base64.b64encode(b'\0')
            elif size == 1:
                binary = base64.b64encode(value[0].encode('utf-8'))
            value = f'"{binary.decode("utf-8")}"'
        if len(ktype) > 0:
            name = f"{name}-{ktype}"

        if user_name == "":
            user_name = "."

        line_fields["timestamp"] = int(timestamp_ns // 1e3)
        line_fields["train_id"] = train_id
        line_fields["name"] = name
        line_fields["value"] = value
        line_fields["user_name"] = user_name
        line_fields["flag"] = flag
        # bail out if value is empty and not an escaped string,
        # or if an integer was created for an empty value
        # Note: the LOGOUT event has a non valid key `.`
        # and an empty value. In this case, no exception is raised
        if not value.strip() and name != '.':
            raise Exception(f"Empty value for {name}@{train_id}")
        if value.strip() == "i":
            raise Exception(f"Empty integer for {name}@{train_id}")
        # bail out for empty names
        if not name.strip():
            raise Exception(f"Empty name for {value}@{train_id}")
        return line_fields
