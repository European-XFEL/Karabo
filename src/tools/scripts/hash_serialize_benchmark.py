import argparse
import time

import numpy as np


def benchmark_read_func(read_func, hash_buffer, num_iters=10):
    """ Runs `read_func` `num_iters` times on the data in `read_buffer` and
    returns the avg. call duration.
    """
    elapsed_time = 0
    for i in range(num_iters):
        start_time = time.perf_counter()
        read_func(hash_buffer)
        elapsed_time += time.perf_counter() - start_time

    return elapsed_time / num_iters


def benchmark_wrapper(func, args):
    perf_time, count = 0, 0
    iter_counts = (10, 100, 1000, 10000, 100000)
    for count in iter_counts:
        start_time = time.time()
        perf_time = func(*args, num_iters=count)
        elapsed_time = time.time() - start_time
        if elapsed_time > 2.0:
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


def get_api_1_funcs():
    from karabo.api_1 import Hash, BinarySerializerHash

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


def get_api_2_funcs():
    from karabo.api_2 import BinaryParser, Hash

    parser = BinaryParser()

    def read_func(buffer):
        return parser.read(buffer)

    def write_func(hsh):
        return hsh.encode("Bin")

    return (read_func, write_func, Hash)


def get_api_refactor_funcs():
    from krbgui.data.api import Hash, read_binary_hash, write_binary_hash

    return (read_binary_hash, write_binary_hash, Hash)


def get_hash_buffer(hash_create_func, write_func, hash_factory):
    return write_func(hash_create_func(hash_factory))


def run_benchmark(args):
    API_FUNCS = [
        get_api_1_funcs(), get_api_2_funcs(), get_api_refactor_funcs()
    ]
    RUNS = [
        ('Array', create_array_hash),
        ('Flat', create_flat_hash),
        ('Deep', create_deep_hash),
    ]

    read_func, write_func, hash_factory = API_FUNCS[args.api - 1]
    for name, create_func in RUNS:
        buffer = get_hash_buffer(create_func, write_func, hash_factory)
        f_args = (read_func, buffer)
        avg_time, iterations = benchmark_wrapper(benchmark_read_func, f_args)
        print('{}: API {} ran in {} secs ({} iterations)'.format(
                name, args.api, avg_time, iterations))


def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-a', '--api', default=1, type=int,
                        help='Which API to benchmark')

    args = parser.parse_args()
    run_benchmark(args)

if __name__ == '__main__':
    main()
