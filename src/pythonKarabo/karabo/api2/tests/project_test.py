from contextlib import contextmanager
import os.path as op
from zipfile import ZipFile

from karabo.api2.project import (
    Project, ProjectConfiguration, BaseDeviceGroup, BaseDevice, BaseMacro,
    BaseScene
)
from karabo.testing.utils import temp_file

TEST_PROJECT_PATH = op.join(op.dirname(__file__), 'reference.krb')


@contextmanager
def temp_project():
    with temp_file(suffix=Project.PROJECT_SUFFIX) as fn:
        # Prime the file with a zip header
        with ZipFile(fn, 'w'):
            pass
        proj = Project(fn)
        yield proj


def test_empty_project():
    with temp_project() as proj:
        proj.zip()


def test_read_project():
    proj = Project(TEST_PROJECT_PATH )
    proj.unzip()

    # FIXME: Actually look at the project's contents
