from contextlib import contextmanager
from zipfile import ZipFile

from karabo.api2.hash import Hash
from karabo.testing.utils import temp_file

from ..constants import PROJECT_SUFFIX
from ..configuration import ProjectConfigurationData
from ..device import DeviceData, DeviceGroupData
from ..macro import MacroData
from ..model import ProjectData
from ..monitor import MonitorData
from ..scene import SceneData


@contextmanager
def temp_project():
    with temp_file(suffix=PROJECT_SUFFIX) as fn:
        # Prime the file with a zip header
        with ZipFile(fn, 'w'):
            pass
        yield ProjectData(fn)


def test_device():
    config = Hash('maximumSpeed', 10.0)
    NAME = 'conveyor0'
    device = DeviceData('pythonServer', 'ConveyorPy', NAME, 'ignore',
                        config=config)
    with temp_project() as proj:
        proj.addDevice(device)
        assert proj.getDevice(NAME) is device
        assert proj.getDevice('blah') is None
        assert device in proj.getDevices([NAME])


def test_device_group():
    with temp_project() as proj:
        pass


def test_macro():
    with temp_project() as proj:
        pass


def test_monitor():
    with temp_project() as proj:
        pass


def test_project_config():
    with temp_project() as proj:
        pass


def test_resource():
    with temp_project() as proj:
        pass


def test_scene():
    with temp_project() as proj:
        pass
