import os.path
import sys
import threading

from pkg_resources import working_set

from karabo.bound import EventLoop, Hash, TextSerializerHash

_, command, namespace, name = sys.argv

entrypoint = next(working_set.iter_entry_points(namespace, name))

if command == "schema":
    schema = entrypoint.load().getSchema(entrypoint.name)
    h = Hash()
    h[name] = schema
    ser = TextSerializerHash.create("Xml")
    print(ser.save(h))
elif command == "run":
    config = sys.stdin.read()
    ser = TextSerializerHash.create("Xml")
    config = ser.load(config)

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

    device = entrypoint.load().create(name, config)
    device._finalizeInternalInitialization()
    t.join()
