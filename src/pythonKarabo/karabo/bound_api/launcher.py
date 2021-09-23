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
    PluginLoader, PythonDevice, TextSerializerHash)


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
        config = sys.stdin.read()
        ser = TextSerializerHash.create("Xml")
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

        t = threading.Thread(target=EventLoop.work)
        t.start()
        try:
            # Creating device via Configurator runs config validation and thus
            # ensures that defaults are available.
            device = Configurator(PythonDevice).create(name, config)
            device._finalizeInternalInitialization()
        except BaseException:
            EventLoop.stop()
            raise
        finally:
            t.join()


if __name__ == "__main__":
    main()
