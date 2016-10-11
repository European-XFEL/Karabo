import argparse
import os.path as op
import sys

from karabo.common.scenemodel.api import BaseIconsModel, DisplayIconsetModel
from karabo.middlelayer import Hash, Project, XMLWriter
from karabo.middlelayer_api.project import BaseDevice, BaseDeviceGroup

FIXERS = []


def fixer_func(func):
    FIXERS.append(func)
    return func


def read_project(path):
    """ Reads a project file.
    """
    class _Device(BaseDevice):
        def __init__(self, *args, **kw):
            super(_Device, self).__init__(*args, **kw)
            self.descriptor = None

        def toXml(self):
            return XMLWriter().write(Hash(self.classId, self.initConfig))

    class _DeviceGroup(BaseDeviceGroup):
        def __init__(self, id, serverId, classId, ifexists, **kw):
            super(_DeviceGroup, self).__init__(
                serverId, classId, id, ifexists, **kw)
            self.descriptor = None

        def toXml(self):
            return XMLWriter().write(Hash(self.classId, self.initConfig))

    factories = {'Device': _Device, 'DeviceGroup': _DeviceGroup}
    project = Project(path)
    project.unzip(factories=factories)
    return project


@fixer_func
def scene_fixer(project):
    """ Upgrade scene files from 1.4 to consolidated (1.5-ish).

    This means:
    * For each scene, move icon data from the project to the scene.
    * Remove icon resources from the project.
    """
    def _get_image_data(model):
        if model.data:
            return  # data trait is already set!
        url = model.image
        model.data = project.getURL(url)

    def _update_icon_model(parent_model):
        for child in parent_model.children:
            if isinstance(child, BaseIconsModel):
                for icon_data in child.values:
                    _get_image_data(icon_data)
            elif isinstance(child, DisplayIconsetModel):
                _get_image_data(child)
            else:
                if hasattr(child, "children"):
                    _update_icon_model(child)

    for scene_model in project.scenes:
        # Recursively set all icon model data
        _update_icon_model(scene_model)

    # Kill the resources
    project.resources = {}


def upgrade_project(old, new):
    """ Upgrade a project from the oldest version readable to the latest
    version of the file format.
    """
    project = read_project(old)
    for fixer in FIXERS:
        fixer(project)
    project.zip(filename=new)


def main():
    description = ('Upgrade scene files to the latest version of the file '
                   'format.')
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('-i', '--input',
                        help='The project file containing scenes to upgrade')
    parser.add_argument('-o', '--output',
                        help='The destination (new) project file.')

    ns = parser.parse_args()

    if op.abspath(ns.input) == op.abspath(ns.output):
        print("Please give a NEW filename for the output!")
        sys.exit(1)

    upgrade_project(ns.input, ns.output)

if __name__ == '__main__':
    main()
