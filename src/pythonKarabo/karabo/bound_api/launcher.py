import ctypes
import os.path
import signal
import sys
import threading

from pkg_resources import working_set

from karabo.bound import (EventLoop, Hash, TextSerializerHash,
                          BinarySerializerHash)

_, command, namespace, name = sys.argv

entrypoint = next(working_set.iter_entry_points(namespace, name))

if command == "schema":
    # print the schema of a device class to stdout
    schema = entrypoint.load().getSchema(entrypoint.name)
    h = Hash()
    h[name] = schema
    ser = BinarySerializerHash.create("Bin")
    sys.stdout.buffer.write(ser.save(h))
elif command == "run":
    # run a device class. The configuration is read from stdin.
    config = sys.stdin.read()
    ser = TextSerializerHash.create("Xml")
    config = ser.load(config)

    # in case our parent process (the device server) dies,
    # tell linux to terminate ourselves.
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
