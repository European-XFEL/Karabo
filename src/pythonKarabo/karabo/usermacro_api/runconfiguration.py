from numpy import array
from numpy import append

from karabo.middlelayer import connectDevice, getDevices


class RunConfiguration(object):

    configurationTable = []

    def __init__(self):
        print("int called")

    def addDataSource(self, deviceName, groupName):
        """
        Adds data source to 'UserSources' table of given
        configuration group
        """
        msg = '{} is not online!'.format(groupName)
        topology = getDevices()
        assert groupName in topology, msg
        self.configurationTable = connectDevice(groupName)
        msg = '{} is not online!'.format(deviceName)
        assert deviceName in topology, msg
        if self.configurationTable.group.user:
            self.configurationTable.group.user\
                = append(self.configurationTable.group.user.value,
                         array([(deviceName, 'Control', 'record-all',
                                 False)],
                               dtype=self.configurationTable
                               .group.user.value.dtype
                               ))
        else:
            self.configurationTable.group.user\
                = array([(deviceName, 'Control', 'record-all', False)],
                        dtype=[('source', 'O'), ('type', 'O'),
                               ('behavior', 'O'), ('monitored', '?')])

    def configure(self, configuratorName):

        msg = '{} is not online!'.format(configuratorName)
        assert configuratorName in getDevices(), msg
        configurator = connectDevice(configuratorName)
        if len(configurator.availableGroups):
            if len(configurator.sources):
                """ send Data to DAQ """
                configurator.buildConfigurationInUse()

    def start(self, runControllerName):

        msg = '{} is not online!'.format(runControllerName)
        assert runControllerName in getDevices(), msg
        runController = connectDevice(runControllerName)
        runController.configure()
        runController.monitor()
        runController.record()

    def stop(self, runControllerName):

        msg = '{} is not online!'.format(runControllerName)
        assert runControllerName in getDevices(), msg
        runController = connectDevice(runControllerName)
        runController.tune()
        runController.ignore()
