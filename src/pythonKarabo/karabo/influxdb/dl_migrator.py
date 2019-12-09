import argparse
from asyncio import get_event_loop
import os
import re

from karabo.influxdb import DlRaw2Influx, DlSchema2Influx

class DlMigrator():
    def __init__(self, path, topic, host, port=8086, device_id=None,
                 dry_run=True, prints_on=True):
        self.path = path
        self.dry_run = dry_run
        self.port = port
        self.host = host
        self.device_id = device_id
        self.chunk_queries = 8000  # optimal values are between 5k and 10k
        self.prints_on = prints_on
        self.db_name = topic

    async def run(self):
        for dir_, _, files in os.walk(self.path):
            if not dir_.endswith("raw"):
                continue
            device_id = os.path.relpath(dir_, self.path)[:-4]
            for filename in files:
                if filename == "archive_schema.txt":
                    path = os.path.join(dir_, filename)
                    print("Inserting Schemas in file {}".format(path))
                    await self.insert_schema(path, device_id)
                if re.match("^archive_[0-9]+\.txt$", filename):
                    print("Inserting Configurations in file {}".format(path))
                    path = os.path.join(dir_, filename)
                    await self.insert_archive(path, device_id)

    async def insert_archive(self, path, device_id):
        conf2db = DlRaw2Influx(path=path, topic=self.db_name, host=self.host,
            port=self.port, dry_run=self.dry_run, device_id=device_id)
        await conf2db.run()

    async def insert_schema(self, path, device_id):
        schema2db = DlSchema2Influx(path=path, topic=self.db_name,
            host=self.host, port=self.port, dry_run=self.dry_run,
            device_id=device_id)
        await schema2db.run()


def main():
    parser = argparse.ArgumentParser(
        description="parse Karabo schema text file and insert data in "
                    "influxDb.")
    parser.add_argument("topic", help="Karabo topic")
    parser.add_argument("path",
                        help="full path of schema file")
    parser.add_argument("host", help="influxDb hostname")
    parser.add_argument("port", help="influxDb port")
    parser.add_argument("-d", "--device_id", dest="device_id", action="store")
    parser.add_argument("--dry_run", dest="dry_run", action="store_true")
    parser.add_argument("--quiet", dest="prints_on", action="store_false")
    parser.set_defaults(dry_run=False)
    args = parser.parse_args()
    o = DlMigrator(**vars(args))
    get_event_loop().run_until_complete(o.run())
