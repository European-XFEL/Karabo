import os.path as op

from ..configuration import ProjectConfigurationData
from ..device import DeviceData, DeviceGroupData
from ..io import read_project, write_project
from ..macro import MacroData
from ..monitor import MonitorData
from ..scene import SceneData
import karabo.testing as testing_pkg
from karabo.testing.utils import temp_file

TEST_PROJECT_PATH = op.join(op.dirname(testing_pkg.__file__), 'resources',
                            'reference.krb')
FACTORIES = {
    'Device': DeviceData,
    'DeviceGroup': DeviceGroupData,
    'Macro': MacroData,
    'Monitor': MonitorData,
    'ProjectConfiguration': ProjectConfigurationData,
    'Scene': SceneData,
}


def test_read_project():
    proj = read_project(TEST_PROJECT_PATH, FACTORIES)

    with temp_file() as fn:
        write_project(proj, fn)

    # FIXME: Actually look at the project's contents
