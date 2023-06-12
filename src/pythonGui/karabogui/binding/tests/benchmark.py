# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
import argparse
import os.path as op
import time
from glob import glob

from karabo.native import Schema, decodeBinary
from karabogui.binding.api import apply_configuration, build_binding

TEST_DATA_DIR = op.join(op.dirname(__file__), 'data')
TIME_BASE = 1e6


def _iter_schemas_and_configs():
    for name in glob(op.join(TEST_DATA_DIR, '*.schema')):
        classname = op.splitext(op.basename(name))[0]
        with open(name, 'rb') as fp:
            hsh = decodeBinary(fp.read())

        config_filename = op.splitext(name)[0] + '.config'
        with open(config_filename, 'rb') as fp:
            config = decodeBinary(fp.read())

        yield Schema(name=classname, hash=hsh), config


def inner_wrapper(func, args, num_iters=10):
    """ Runs `func` `num_iters` times on the data in `read_buffer` and
    returns the avg. call duration.
    """
    start_time = time.process_time()
    for i in range(num_iters):
        func(*args)
    elapsed_time = time.process_time() - start_time

    return elapsed_time / num_iters


def benchmark_wrapper(func, args):
    total_time = 0
    perf_time, count = 0, 0
    iter_counts = (10, 100, 1000, 10000, 100000)
    for count in iter_counts:
        start_time = time.perf_counter()
        perf_time = inner_wrapper(func, args, num_iters=count)
        total_time += time.perf_counter() - start_time
        if total_time > 1.0:
            break

    return perf_time, count


def run_benchmark(version):
    API_FUNCS = {1: (build_binding, apply_configuration)}
    API_NAMES = {1: 'Current'}
    MSG = '{} took {:.2f} us/call | {:.2f} calls/s ({} iterations)'
    build_func, config_func = API_FUNCS[version]
    print('*** Running tests for', API_NAMES[version], 'API')
    for schema, config in _iter_schemas_and_configs():
        args = (schema,)
        avg_time, iters = benchmark_wrapper(build_func, args)
        print(MSG.format('Build ' + schema.name, avg_time * TIME_BASE,
                         1 / avg_time, iters))

        args = (config, build_func(schema))
        avg_time, iters = benchmark_wrapper(config_func, args)
        print(MSG.format('Config ' + schema.name, avg_time * TIME_BASE,
                         1 / avg_time, iters))


def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-v', '--version', default=1, type=int,
                        help='Which version to benchmark')

    args = parser.parse_args()
    if args.version in (1,):
        run_benchmark(args.version)


if __name__ == '__main__':
    main()
