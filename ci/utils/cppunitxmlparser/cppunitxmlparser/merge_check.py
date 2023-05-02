#!/usr/bin/env python
from __future__ import print_function

import argparse
import fnmatch
import os
import sys
import xml.etree.ElementTree as ET

from .merge_cppunitxml import merge_results

def generate_placeholder(path):
    new_root = ET.Element('TestRun')
    fts = ET.SubElement(new_root, 'FailedTests')
    ft = ET.SubElement(fts, 'FailedTest')
    ft.attrib['id'] = '1'
    ET.SubElement(ft, 'Name').text = str(path)
    ET.SubElement(ft, 'FailureType').text = 'Error'
    ET.SubElement(ft, 'Message').text = '%s Test did not run' % path
    stats = ET.SubElement(new_root, 'Statistics')
    ET.SubElement(stats, 'Tests').text = '1'
    ET.SubElement(stats, 'FailuresTotal').text = '1'
    ET.SubElement(stats, 'Errors').text = '1'
    ET.SubElement(stats, 'Failures').text = '0'
    new_tree = ET.ElementTree(new_root)
    new_tree.write(path)


DESCRIPTION = """
Verifies and merges multiple CPPUnit XML files into a single JUnit file.
"""

def main():
    parser = argparse.ArgumentParser(
        description=DESCRIPTION,
        formatter_class=argparse.HelpFormatter)
    parser.add_argument(
        '--result_files', '-f',
        default=[],
        help='list of expected result files',
        nargs='*')
    parser.add_argument(
        '--output', '-o', default='STDOUT',
        help='The outputfile')
    args = parser.parse_args()
    for fn in args.result_files:
         if not os.path.exists(fn):
             generate_placeholder(fn)
    ret = merge_results(args.result_files, args.output)
    exit(ret)


if __name__ == '__main__':
    main()
