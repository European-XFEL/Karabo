import argparse

from builder import get_build_args, get_pack_args


DESCRIPTION = """
Karabo Environment Packing Manager

This script will manage the running of tests and uploading of the packed conda
environment and built recipe. 

This can be used in conjuction with the build script.
"""

root_ap = argparse.ArgumentParser(
    description=DESCRIPTION,
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)
sps = root_ap.add_subparsers(metavar='')
get_build_args(sub_parser=sps)
get_pack_args(sub_parser=sps)

args = root_ap.parse_args()
if not hasattr(args, 'klass'):
    root_ap.print_help()
    root_ap.exit()
b = args.klass(args)
b.run()
