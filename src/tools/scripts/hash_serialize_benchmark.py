import argparse
import time

import numpy as np

from krbgui.data.api import Hash, read_binary_hash, write_binary_hash


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


def create_array_hash():
    return Hash(
        "data", np.zeros(200000),
        "width", 400,
        "height", 500,
        "colorspace", "luminance"
    )


def create_flat_hash():
    return Hash(
        "bool", True,
        "bytes", b"deadbeef",
        "float", 3.14,
        "integer", 42,
        "bigint", 1 << 60,
        "complex", 1+2j,
        "stringlist", ["one", "two", "three"],
        "hashlist", [Hash("foo", 123), Hash("bar", False)]
    )


def create_deep_hash():
    return Hash(
        "a.b.c", True,
        "foo.bar.baz", b"deadbeef",
        "foo.bar.qux", 3.14,
        "a.b.z", 42,
        "x.y.z", 1 << 60,
        "one.two.three", 1+2j,
        "three.two.one", ["one", "two", "three"],
        "hashlist", [Hash("foo", 123), Hash("bar", False)]
    )


def get_hash_buffer(hash_create_func, write_func):
    return write_func(hash_create_func())


def run_benchmark(args):
    API_FUNCS = [
        (read_binary_hash, write_binary_hash),
    ]
    RUNS = [
        ('Array', create_array_hash),
        ('Flat', create_flat_hash),
        ('Deep', create_deep_hash),
    ]

    read_func, write_func = API_FUNCS[args.api - 1]
    for name, create_func in RUNS:
        buffer = get_hash_buffer(create_func, write_func)
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
