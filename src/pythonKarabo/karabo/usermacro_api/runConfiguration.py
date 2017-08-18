from numpy import array
from numpy import append

from karabo.middlelayer import (connectDevice, KaraboError, lock, Proxy, State,
                                waitUntilNew, lock, Proxy, String, Slot,
                                getDevices)


class RunConfiguration(object):

    configurationTable = []

    def __init__(self):
        print("int called")

    def addDataSource(self, deviceName, groupName):

        msg = '{} is not online!'.format(groupName)
        assert groupName in getDevices(), msg
        self.configurationTable = connectDevice(groupName)
        msg = '{} is not online!'.format(deviceName)
        assert deviceName in getDevices(), msg
        self.configurationTable.group.user\
            = append(self.configurationTable.group.user.value,
                     array([(deviceName, 'Control', 'record-all',
                             False)],
                           dtype=self.configurationTable.group.user.value.dtype))

    def configure(self, configuratorName):

        msg = '{} is not online!'.format(configuratorName)
        assert configuratorName in getDevices(), msg
        configurator= connectDevice(configuratorName)
        if len(configurator.availableGroups):
            if len(configurator.sources):
                """ send Data to DAQ """
                configurator.buildConfigurationInUse()

    def start(self, runControllerName):

        msg = '{} is not online!'.format(runControllerName)
        assert runControllerName in getDevices(), msg
        runController= connectDevice(runControllerName)
        runController.configure()
        runController.monitor()
        runController.record()

    def stop(self, runControllerName):

        msg = '{} is not online!'.format(runControllerName)
        assert runControllerName in getDevices(), msg
        runController= connectDevice(runControllerName)
        runController.tune()
        runController.ignore()


