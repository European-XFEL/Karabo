#!/usr/bin/env python
import sys
import xml.etree.ElementTree as ET

def main():
    """Merge multiple CPPUnit XML files into a JUnit single results file.
    Output dumps to stdout.
    example usage:
        $ python merge_xml.py results1.xml results2.xml > results.xml
    """
    args = sys.argv[1:]
    if not args:
        usage()
        sys.exit(2)
    if '-h' in args or '--help' in args:
        usage()
        sys.exit()
    merge_results(args[:])


def merge_results(xml_files):
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
    ET.dump(new_tree)


def usage():
    print(main.__doc__)
 
if __name__ == '__main__':
    main()