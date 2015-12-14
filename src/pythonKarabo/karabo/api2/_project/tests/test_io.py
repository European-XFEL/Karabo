import os.path as op

import karabo.testing as testing_pkg
from karabo.testing.utils import compare_zip_files, temp_file
from ..io import read_project, write_project

TEST_PROJECT_PATH = op.join(op.dirname(testing_pkg.__file__), 'resources',
                            'reference.krb')


def test_read_project():
    proj = read_project(TEST_PROJECT_PATH)

    with temp_file() as fn:
        write_project(proj, fn)

    # FIXME: Actually look at the project's contents


def test_project_round_trip():
    proj = read_project(TEST_PROJECT_PATH)

    with temp_file() as fn:
        write_project(proj, fn)
        assert compare_zip_files(TEST_PROJECT_PATH, fn)
