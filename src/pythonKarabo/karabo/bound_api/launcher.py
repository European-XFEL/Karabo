import ctypes
import os.path
import signal
import sys
import threading
import copy

from pkg_resources import working_set

from karabo.bound import (EventLoop, Hash, TextSerializerHash,
                          BinarySerializerHash, OVERWRITE_ELEMENT,
                          PythonDevice)


def main():
    # this helper script is only used in the integration tests, hence the
    # lack of documentation.
    command, namespace, name = sys.argv[1:4]

    entrypoint = next(working_set.iter_entry_points(namespace, name))

    if command == "schema" or command == "schemaOverwriteVersion":
        # print the schema of a device class to stdout
        schema = entrypoint.load().getSchema(entrypoint.name)
        if command != "schema":  # i.e. schemaOverwriteVersion
            # manipulate for unit test: use fixed karabo version
            # and fixed connection defaults (not from environment variables)
            (OVERWRITE_ELEMENT(schema)
             .key("karaboVersion")
             .setNewDefaultValue("UNKNOWN")
             .commit(),
             )
            h = schema.getParameterHash()
            for key in h["Logger.network.connection"]:
                (OVERWRITE_ELEMENT(schema)
                 .key(f"Logger.network.connection.{key}.brokers")
                 .setNewDefaultValue("tcp://localhost:7777")
                 .commit(),
                 OVERWRITE_ELEMENT(schema)
                 .key(f"Logger.network.connection.{key}.domain")
                 .setNewDefaultValue("karabo")
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

        # If log filename not specified, make use of device name to avoid
        # that different processes write to the same file.
        # TODO:
        # "karabo.log" is default from RollingFileAppender::expectedParameters
        # Unfortunately, the GUI sends full config, including defaults.
        if ("Logger.file.filename" not in config
                or config["Logger.file.filename"] == "karabo.log"):
            deviceId = config["_deviceId_"]
            defaultLog = "device-{}.log".format(
                deviceId.replace(os.path.sep, "_"))
            config["Logger.file.filename"] = defaultLog

        # Load the module containing classid so that it gets registered.
        t = threading.Thread(target=EventLoop.work)
        t.start()
        try:
            device = entrypoint.load().create(name, config)
            device._finalizeInternalInitialization()
        except:
            EventLoop.stop()
            raise
        finally:
            t.join()


if __name__ == "__main__":
    main()
