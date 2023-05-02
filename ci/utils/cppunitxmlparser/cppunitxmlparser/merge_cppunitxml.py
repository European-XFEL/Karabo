#!/usr/bin/env python
from __future__ import print_function

import argparse
import fnmatch
import os
import sys
import xml.etree.ElementTree as ET


def merge_results(xml_files, output_file):
    failures = 0
    tests = 0
    errors = 0

    new_root = ET.Element('testsuites')

    for file_name in xml_files:
        tree = ET.parse(file_name)
        test_run = tree.getroot()
        test_suite = ET.SubElement(new_root, 'testsuite')
        tests_ = int(test_run.findtext("./Statistics/Tests"))
        fails = int(test_run.findtext("./Statistics/Failures"))
        errors_ = int(test_run.findtext("./Statistics/Errors"))
        failures += fails
        errors += errors_
        tests += tests_
        test_suite.attrib['failures'] = str(fails)
        test_suite.attrib['tests'] = str(tests_)
        test_suite.attrib['errors'] = str(errors_)
        test_suite.attrib['time'] = "N/A"
        for ftest in test_run.findall(".//FailedTest"):
            tc = ET.SubElement(test_suite, 'testcase')
            tc.attrib['id'] = "{}_{}".format(file_name, ftest.attrib['id'])
            tc.attrib['name'] = ftest.findtext("./Name")
            tc.attrib['time'] = "N/A"
            failure = ET.SubElement(tc, 'failure')
            failure.attrib['type'] = ftest.findtext("./FailureType")
            msg = "{}:{} :{}".format(
                ftest.findtext("./Location/File") or "Filename Missing",
                ftest.findtext("./Location/Line") or "Line Number Missing",
                ftest.findtext("./FailureType")
            )
            failure.attrib['message'] = msg
            failure.text = "{}\n{}".format(msg, ftest.findtext("./Message"))
        for test in test_run.findall(".//Test"):
            tc = ET.SubElement(test_suite, 'testcase')
            tc.attrib['id'] = "{}_{}".format(file_name, test.attrib['id'])
            tc.attrib['name'] = test.findtext("./Name")
            tc.attrib['time'] = "N/A"

    new_root.attrib['failures'] = '%s' % failures
    new_root.attrib['tests'] = '%s' % tests
    new_root.attrib['errors'] = '%s' % errors
    new_root.attrib['time'] = 'N/A'
    new_tree = ET.ElementTree(new_root)
    if output_file == 'STDOUT':
        ET.dump(new_tree)
    else:
        new_tree.write(output_file)
    return (errors + failures)


DESCRIPTION = """
Merge multiple CPPUnit XML files into a JUnit single results file.
Output dumps to stdout.
"""
 
def main():
    parser = argparse.ArgumentParser(description=DESCRIPTION,
                                     formatter_class=argparse.HelpFormatter)
    parser.add_argument('--file-glob', '-g', default='*.xml',
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

    ret = merge_results(xml_files, args.output)
    exit(ret)


if __name__ == '__main__':
    main()
