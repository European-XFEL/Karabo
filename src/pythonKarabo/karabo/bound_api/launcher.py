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
import copy
import ctypes
import signal
import sys
import threading
from contextlib import redirect_stdout
from io import StringIO

from pkg_resources import working_set

from karabo.bound import (
    OVERWRITE_ELEMENT, BinarySerializerHash, Configurator, EventLoop, Hash,
    PluginLoader, PythonDevice, SignalSlotable)


def main():
    # this helper script is only used in the integration tests, hence the
    # lack of documentation.
    command, namespace, name = sys.argv[1:4]

    entrypoint = next(working_set.iter_entry_points(namespace, name))

    if command == "schema" or command == "schemaOverwriteVersion":
        # print the schema of a device class to stdout
        f = StringIO()
        # plugin loading might print something to stdout
        # this will ruin the binary integrity of the schema output.
        # To be safe, we cache it and output it to stderr.
        with redirect_stdout(f):
            schema = entrypoint.load().getSchema(entrypoint.name)
        sys.stderr.buffer.write(f.getvalue().encode('utf-8'))
        if command != "schema":  # i.e. schemaOverwriteVersion
            # manipulate for unit test: use fixed karabo version
            # and fixed connection defaults (not from environment variables)
            (OVERWRITE_ELEMENT(schema)
             .key("karaboVersion")
             .setNewDefaultValue("UNKNOWN")
             .commit(),
             )
        h = Hash()
        h[name] = schema
        ser = BinarySerializerHash.create("Bin")
        sys.stdout.buffer.write(ser.save(h))
    elif command == "run":
        # run a device class. The configuration is read from stdin.
        config = sys.stdin.buffer.read()
        ser = BinarySerializerHash.create("Bin")
        config = ser.load(config)
        if '_connection_' in config:
            PythonDevice.connectionParams = copy.copy(config['_connection_'])
            config.erase('_connection_')

        # in case our parent process (the device server) dies,
        # tell linux to terminate this device process
        PR_SET_PDEATHSIG = 1  # from linux/prctl.h
        libc = ctypes.cdll.LoadLibrary("libc.so.6")
        libc.prctl(PR_SET_PDEATHSIG, signal.SIGTERM)

        if "_logger_" in config:
            # Yet unused when started from MDL, but equivalent to
            # bound_api.device.launchPythonDevice()
            PythonDevice._loggerCfg = copy.copy(config['_logger_'])
            config.erase('_logger_')

        if "timeServerId" in config:
            PythonDevice.timeServerId = copy.copy(config["timeServerId"])
            config.erase("timeServerId")
        # PluginLoader registers device classes so Configurator knows them
        loader = PluginLoader.create("PythonPluginLoader",
                                     Hash("pluginNamespace", namespace))
        loader.update()

        serverId = "__none__"
        if "_serverId_" in config:
            serverId = config["_serverId_"]
        deviceId = config["_deviceId_"]
        device = None
        t = threading.Thread(target=EventLoop.work)
        t.start()
        try:
            # Creating device via Configurator runs config validation and thus
            # ensures that defaults are available.
            try:
                device = Configurator(PythonDevice).create(name, config)
            except Exception as e:
                # create a signal slotable to act as a flight data recorder
                # to inform the server that an exception occurred
                # during __init__
                fdr = SignalSlotable(deviceId)
                fdr.start()
                fdr.call(
                    serverId, "slotDeviceUp", deviceId, False, str(e))
                raise e
            device._finalizeInternalInitialization()
        except BaseException as e:
            EventLoop.stop()
            raise e
        finally:
            t.join()


if __name__ == "__main__":
    main()
