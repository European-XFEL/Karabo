import os
import tempfile
import xml.etree.ElementTree as ET

from ..merge_check import generate_placeholder
from ..merge_cppunitxml import merge_results
from ..merge_junit import merge_junit

FILES = [
    'coreTest.xml',
    'ioTest.xml',
    'logTest.xml',
    'netTest.xml',
    'utilTest.xml',
    'xmsTest.xml'
]

def test_merge():
    with tempfile.TemporaryFile() as f:
        here = os.path.dirname(__file__)
        fns = [os.path.join(here, fn) for fn in FILES]
        ret = merge_results(fns, f)
        f.seek(0)
        root = ET.parse(f).getroot()
        assert root.attrib['errors'] == '1'
        assert root.attrib['failures'] == '1'
        assert root.attrib['tests'] == '245'
        assert ret == 2, f"{ret}"


def test_check():
    files = [
        'logTest.xml',
        'missingFile.xml']
    with tempfile.TemporaryFile() as f:
        here = os.path.dirname(__file__)
        fns = [os.path.join(here, fn) for fn in files]
        generate_placeholder(fns[1])
        ret = merge_results(fns, f)
        # remove placeholder
        os.remove(fns[1])
        f.seek(0)
        root = ET.parse(f).getroot()
        assert root.attrib['errors'] == '1'
        assert root.attrib['failures'] == '0'
        assert root.attrib['tests'] == '4'
        assert ret == 1, f"{ret}"


def test_merge_junit():
    here = os.path.dirname(__file__)
    fns = [os.path.join(here, fn) for fn in FILES]
    junit1 = os.path.join(here, '1testjunit.xml')
    sum_ = merge_results(fns, junit1)
    files = [
        'logTest.xml',
        'missingFile.xml']
    fns = [os.path.join(here, fn) for fn in files]
    generate_placeholder(fns[1])
    junit2 = os.path.join(here, '2testjunit.xml')
    sum_ += merge_results(fns, junit2)
    with tempfile.TemporaryFile() as f:
        total = merge_junit([junit1, junit2], f)
        # remove temporary files
        os.remove(junit1)
        os.remove(junit2)
        assert total == sum_
        assert total == 3
        f.seek(0)
        root = ET.parse(f).getroot()
        assert root.attrib['errors'] == '2'
        assert root.attrib['failures'] == '1'
