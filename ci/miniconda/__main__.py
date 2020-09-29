import argparse

from builder import builder_main, get_build_args, get_pack_args, packer_main


DESCRIPTION = """
Karabo Environment Packing Manager

This script will manage the running of tests and uploading of the packed conda
environment and built recipe. 

This can be used in conjuction with the build script.
"""

root_ap = argparse.ArgumentParser(description=DESCRIPTION)

operation_kwargs = {
    'type': str,
    'default': 'build',
    'choices': ['build', 'pack']
}

root_ap.add_argument('operation', **operation_kwargs)
get_build_args(parser=root_ap)
get_pack_args(parser=root_ap)

args = root_ap.parse_args()

if args.operation == 'build':
    builder_main(args)
elif args.operation == 'pack':
    packer_main(args)
