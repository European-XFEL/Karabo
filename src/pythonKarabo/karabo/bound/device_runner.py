# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import argparse
import copy
import os
import threading

from pkg_resources import iter_entry_points

from karabo.bound import Configurator, EventLoop, Hash, PythonDevice


def _load_entrypoint(classid):
    """Load the module containing `classid` so that it gets registered"""
    for ep in iter_entry_points('karabo.bound_device'):
        if ep.name == classid:
            ep.load()
            return
    raise RuntimeError('Device class is not installed!')


def _parse_config_argument(config_str):
    """Turn a commandline device configuration into a Hash"""
    config = Hash()
    for param in config_str.split(' '):
        key, value = param.split('=')
        config.set(key, value)

    return config


def _run_device(classid, config):
    """Run a single Bound API device in this process

    NOTE: This function should not be expected to return
    """
    if "_connection_" in config:
        PythonDevice.connectionParams = copy.copy(config['_connection_'])
        config.erase('_connection_')

    if "Logger.file.filename" not in config:
        deviceId = config["_deviceId_"]
        defaultLog = "device-{}.log".format(deviceId.replace(os.path.sep, "_"))
        config["Logger.file.filename"] = defaultLog

    t = threading.Thread(target=EventLoop.work)
    t.start()
    try:
        device = Configurator(PythonDevice).create(classid, config)
        device._finalizeInternalInitialization()
    except BaseException:
        EventLoop.stop()
        raise
    finally:
        t.join()


def main():
    description = ('Start a Python Bound device independent of a server. '
                   'This is NOT recommended except if you want to debug a '
                   'device directly (via pdb or gdb)')
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('--classid', required=True,
                        help='A Bound API device class installed locally')
    parser.add_argument('--config', default='',
                        help='A list of space separated key=value pairs '
                             'giving the device configuration.')
    ns = parser.parse_args()

    _load_entrypoint(ns.classid)
    config = _parse_config_argument(ns.config)
    _run_device(ns.classid, config)


if __name__ == '__main__':
    main()
