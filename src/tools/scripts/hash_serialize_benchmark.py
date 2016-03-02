import argparse


def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-a', '--api', default=1, type=int,
                        help='Which API to benchmark')

    args = parser.parse_args()
    print(args)

if __name__ == '__main__':
    main()
