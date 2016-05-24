import argparse
import csv

from karabo.middlelayer_api._project.device import DeviceData
from karabo.middlelayer_api._project.io import write_project
from karabo.middlelayer_api._project.model import ProjectData


def create_project(src, dest):
    """ Create a Karabo project.
    """
    proj = ProjectData(dest)

    for dev in iter_devices(src):
        proj.addDevice(dev)

    write_project(proj, dest)


def iter_devices(path):
    """ Generator function for devices read from a spreadsheet file.
    """
    with open(path, 'r') as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            if len(row) > 0:
                deviceId, classId, serverId = row
                yield DeviceData(serverId, classId, deviceId, 'ignore')


def main():
    description = ('Create a Karabo project file from a spreadsheet file '
                   'which contains device descriptions')
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('-s', '--source',
                        help='The spreadsheet file to read devices from.')
    parser.add_argument('-d', '--dest',
                        help='The project file to write.')

    ns = parser.parse_args()
    create_project(ns.source, ns.dest)

if __name__ == '__main__':
    main()
