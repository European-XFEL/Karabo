#!/usr/bin/env python

# originally from https://gist.github.com/cgoldberg/4320815
# Copyright (c) 2012 Corey Goldbergberg
# MIT Licensed
from __future__ import print_function

import argparse
import fnmatch
import os
import xml.etree.ElementTree as ET

def merge_junit(xml_files, output_file):
    failures = 0
    tests = 0
    errors = 0
    time = 0.0
    cases = []

    for file_name in xml_files:
        tree = ET.parse(file_name)
        test_suite = tree.getroot()
        failures += int(test_suite.attrib['failures'])
        tests += int(test_suite.attrib['tests'])
        errors += int(test_suite.attrib['errors'])
        try:
            time += float(test_suite.attrib['time'])
        except ValueError:
            # cannot convert time
            pass
        cases.append(test_suite.getchildren())

    new_root = ET.Element('testsuite')
    new_root.attrib['failures'] = '%s' % failures
    new_root.attrib['tests'] = '%s' % tests
    new_root.attrib['errors'] = '%s' % errors
    new_root.attrib['time'] = '%s' % time
    for case in cases:
        new_root.extend(case)
    new_tree = ET.ElementTree(new_root)
    if output_file == 'STDOUT':
        ET.dump(new_tree)
    else:
        new_tree.write(output_file)

    if errors or failures:
        for item in new_root.findall('.//testcase/failure'):
            print("======FOUND FAILURE=====")
            print("= Attributes are: ")
            print(item.attrib)
            print("= Text is: ")
            print(item.text)
            print("============================")
    return (errors + failures)


DESCRIPTION = """
Verifies and merges multiple JUnit XML files into a single JUnit file.
"""

def main():
    parser = argparse.ArgumentParser(
        description=DESCRIPTION,
        formatter_class=argparse.HelpFormatter)
    parser.add_argument('--file-glob', '-g', default='*.junit.xml',
                        help='The glob used to locate the XML files.')
    parser.add_argument('--directory', '-d', required=True,
                        help='The root directory of the XML files.')
    parser.add_argument('--output', '-o', default='STDOUT',
                        help='The outputfile')
    args = parser.parse_args()

    xml_files = []
    for fn in os.listdir(args.directory):
         if fnmatch.fnmatch(fn, args.file_glob):
             xml_files.append(os.path.join(args.directory, fn))

    ret = merge_junit(xml_files, args.output)
    exit(ret)

if __name__ == '__main__':
    main()
