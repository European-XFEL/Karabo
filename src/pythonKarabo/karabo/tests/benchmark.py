# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import argparse
import time

import numpy as np


def benchmark_read_func(read_func, hash_buffer, num_iters=10):
    """ Runs `read_func` `num_iters` times on the data in `read_buffer` and
    returns the avg. call duration.
    """
    start_time = time.perf_counter()
    for _ in range(num_iters):
        read_func(hash_buffer)
    elapsed_time = time.perf_counter() - start_time

    return elapsed_time / num_iters


def benchmark_wrapper(func, args):
    total_time = 0
    perf_time, count = 0, 0
    iter_counts = (10, 100, 1000, 10000, 100000)
    for count in iter_counts:
        start_time = time.time()
        perf_time = func(*args, num_iters=count)
        total_time += time.time() - start_time
        if total_time > 2.0:
            break

    return perf_time, count


def create_array_hash(hash_factory):
    return hash_factory(
        "data", np.zeros(200000),
        "width", 400,
        "height", 500,
        "colorspace", "luminance"
    )


def create_flat_hash(hash_factory):
    return hash_factory(
        "bool", True,
        "bytes", b"deadbeef",
        "float", 3.14,
        "integer", 42,
        "bigint", 1 << 20,
        "stringlist", ["one", "two", "three"],
        "hashlist", [hash_factory("foo", 123), hash_factory("bar", False)]
    )


def create_deep_hash(hash_factory):
    return hash_factory(
        "a.b.c", True,
        "foo.bar.baz", b"deadbeef",
        "foo.bar.qux", 3.14,
        "a.b.z", 42,
        "x.y.z", 1 << 20,
        "three.two.one", ["one", "two", "three"],
        "hashlist", [hash_factory("foo", 123), hash_factory("bar", False)]
    )


def get_bound_funcs_bin():
    from karabo.bound import BinarySerializerHash, Hash

    serializer = BinarySerializerHash.create("Bin")

    def create_hash(*args):
        h = Hash()
        for k, v in zip(args[::2], args[1::2]):
            h.set(k, v)
        return h

    def read_func(buffer):
        return serializer.load(buffer)

    def write_func(hsh):
        return serializer.save(hsh)

    return (read_func, write_func, create_hash)


def get_bound_funcs_xml():
    from karabo.bound import Hash, TextSerializerHash

    serializer = TextSerializerHash.create("Xml")

    def create_hash(*args):
        h = Hash()
        for k, v in zip(args[::2], args[1::2]):
            h.set(k, v)
        return h

    def read_func(buffer):
        return serializer.load(buffer)

    def write_func(hsh):
        return serializer.save(hsh)

    return (read_func, write_func, create_hash)


def get_middlelayer_funcs_bin():
    from karabo.native import Hash, decodeBinary, encodeBinary

    return (decodeBinary, encodeBinary, Hash)


def get_middlelayer_funcs_xml():
    from karabo.native import Hash, decodeXML, encodeXML

    return (decodeXML, encodeXML, Hash)


def get_hash_buffer(hash_create_func, write_func, hash_factory):
    return write_func(hash_create_func(hash_factory))


def run_benchmark(args):
    BINARY_FUNCS = [get_bound_funcs_bin, get_middlelayer_funcs_bin]
    XML_FUNCS = [get_bound_funcs_xml, get_middlelayer_funcs_xml]

    RUNS = [
        ('Array', create_array_hash),
        ('Flat', create_flat_hash),
        ('Deep', create_deep_hash),
    ]
    API_NAMES = {1: 'Bound', 2: 'Middlelayer'}
    FACTORY_NAMES = {1: 'Binary', 2: 'XML'}
    MSG = '{}: API {} {} took {:.6f} s/call | {:.2f} calls/s ({} iterations)'
    FUNC_FACTORY = BINARY_FUNCS if args.mode == 1 else XML_FUNCS
    read_func, write_func, hash_factory = FUNC_FACTORY[args.api - 1]()
    for name, create_func in RUNS:
        buffer = get_hash_buffer(create_func, write_func, hash_factory)
        f_args = (read_func, buffer)
        avg_time, iterations = benchmark_wrapper(benchmark_read_func, f_args)
        print(MSG.format(name, API_NAMES[args.api], FACTORY_NAMES[args.mode],
                         avg_time, 1 / avg_time, iterations))


def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-a', '--api', default=1, type=int,
                        help='Which API to benchmark')
    parser.add_argument('-m', '--mode', default=1, type=int,
                        help='Which mode to use: 1 is binary, 2 is xml')

    args = parser.parse_args()
    if 1 <= args.api <= 2 and 1 <= args.mode <= 2:
        run_benchmark(args)


if __name__ == '__main__':
    main()
