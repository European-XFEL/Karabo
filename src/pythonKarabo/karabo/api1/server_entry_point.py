
def runSingleDeviceServer(pluginName):
    """ Run an API 1 device server with a single device.
    """
    from karabo.api_1 import DeviceServer, Hash

    configuration = Hash(
        "DeviceServer.serverId", "PythonAPI1-Server-" + pluginName,
        "DeviceServer.Logger.priority", "INFO",
        "DeviceServer.pluginName", pluginName,
    )

    try:
        server = DeviceServer.create(configuration)
        if server:
            server.run()
    except Exception as e:
        print("Exception caught: " + str(e))
