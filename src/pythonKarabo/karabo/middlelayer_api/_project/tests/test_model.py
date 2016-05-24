from contextlib import contextmanager
from zipfile import ZipFile

from karabo.middlelayer_api.hash import Hash
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
    config = Hash('maximumSpeed', 10.0)
    NAME = 'conveyor0'
    device = DeviceData('pythonServer', 'ConveyorPy', NAME, 'ignore',
                        config=config)
    group = DeviceGroupData('pythonServer', 'ConveyorPy', 'group', 'ignore',
                            config=config)
    with temp_project() as proj:
        group.addDevice(device)
        proj.addDevice(group)
        assert proj.getDevice('group') is group


def test_macro():
    macro = MacroData('mac', code='print("Hello World")')
    with temp_project() as proj:
        proj.addMacro(macro)
        assert proj.getMacro('mac') is macro


def test_monitor():
    monitor = MonitorData('mon')
    with temp_project() as proj:
        proj.addMonitor(monitor)
        assert proj.getMonitor('mon') is monitor


def test_project_config():
    config = ProjectConfigurationData('conf')
    with temp_project() as proj:
        proj.addConfiguration('devID', config)
        assert 'devID' in proj.configurations


def test_resource():
    category = 'projections'
    data = b'here is some data'
    with temp_project() as proj:
        name = proj.addResource(category, data)
        assert proj.getResource(category, name) == data


def test_scene():
    data = '<xml/>'
    scene = SceneData('sceeny', data)
    with temp_project() as proj:
        proj.addScene(scene)
        assert proj.getScene('sceeny') is scene
