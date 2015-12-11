from contextlib import contextmanager
from zipfile import ZipFile

from karabo.testing.utils import temp_file
from ..model import ProjectData, ProjectKeys


@contextmanager
def temp_project():
    with temp_file(suffix=ProjectKeys.PROJECT_SUFFIX) as fn:
        # Prime the file with a zip header
        with ZipFile(fn, 'w'):
            pass
        yield ProjectData(fn)


def test_empty_project():
    with temp_project() as proj:
        proj.zip()
