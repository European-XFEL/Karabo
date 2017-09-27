#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 27, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from karabo.bound import (
    PythonDevice, Hash, HashMergePolicy, Schema, State,
    EXPERT, KARABO_CLASSINFO,
    BOOL_ELEMENT, OVERWRITE_ELEMENT, SLOT_ELEMENT, STRING_ELEMENT,
    TABLE_ELEMENT
)
from .run_configuration_group import RunControlDataSource

KARABO_SCHEMA_ROW_SCHEMA = 'rowSchema'


@KARABO_CLASSINFO('RunConfigurator', '2.2')
class RunConfigurator(PythonDevice):

    def __init__(self, configuration):
        super(RunConfigurator, self).__init__(configuration)
        self.registerInitialFunction(self.initialization)

    @staticmethod
    def expectedParameters(expected):
        """Description of device parameters
        """
        (
            OVERWRITE_ELEMENT(expected).key('state')
            .setNewOptions(State.INIT, State.NORMAL, State.ERROR)
            .setNewDefaultValue(State.INIT)
            .commit(),

            OVERWRITE_ELEMENT(expected).key('visibility')
            .setNewDefaultValue(EXPERT)
            .commit(),

            SLOT_ELEMENT(expected).key('buildConfigurationInUse')
            .displayedName('Push to DAQ')
            .description('Push current configuration structure to the DAQ Run '
                         'controller.')
            .allowedStates(State.NORMAL)
            .commit(),
        )

        availRow = Schema()
        (
            STRING_ELEMENT(availRow).key('groupId')
            .displayedName('Group')
            .description('Run configuration group name.')
            .assignmentMandatory()
            .reconfigurable()
            .commit(),

            STRING_ELEMENT(availRow).key('description')
            .displayedName('Description')
            .description('Run configuration group description.')
            .assignmentOptional().defaultValue('')
            .reconfigurable()
            .commit(),

            BOOL_ELEMENT(availRow).key('use')
            .displayedName('Use')
            .description('Run configuration group usage flag.')
            .assignmentOptional().defaultValue(False)
            .reconfigurable()
            .commit(),

            TABLE_ELEMENT(expected).key('availableGroups')
            .displayedName('Available group configurations')
            .setColumns(availRow)
            .assignmentOptional().defaultValue([])
            .reconfigurable()
            .commit(),
        )

        sourceRow = Schema()
        RunControlDataSource.expectedParameters(sourceRow)
        (
            BOOL_ELEMENT(sourceRow).key('use')
            .displayedName('Use')
            .assignmentOptional().defaultValue(True)
            .reconfigurable()
            .commit(),

            TABLE_ELEMENT(expected).key('sources')
            .displayedName('Compiled source List')
            .description('Overall list of data sources and their attributes')
            .setColumns(sourceRow)
            .assignmentOptional().defaultValue([])
            .commit(),
        )

    def initialization(self):
        # Define the signals & slots
        self._ss.registerSystemSignal('signalRunConfiguration', Hash, str)
        self._ss.registerSystemSignal('signalGroupSourceChanged', Hash, str)
        self.KARABO_SLOT(self.buildConfigurationInUse)
        self.KARABO_SLOT(self.updateAvailableGroups)
        self.KARABO_SLOT(self.slotGetSourcesInGroup)

        # Register instance handlers
        self.remote().registerInstanceNewMonitor(self._newDeviceHandler)
        self.remote().registerInstanceGoneMonitor(self._deviceGoneHandler)

        # Switch on the heartbeat tracking
        # `_newDeviceHandler` will be called for each instance arriving in the
        # system topology (and `_deviceGoneHandler` for the ones leaving)
        self.remote().enableInstanceTracking()

        self._configurations = {}
        self._groupToDevice = {}

        # Ready to go!
        self.updateState(State.NORMAL)

    def preReconfigure(self, incomingReconfiguration):
        self.log.DEBUG('============ preReconfigure  ===============')
        key = 'availableGroups'
        availableGroups = incomingReconfiguration.get(key, default=None)
        if availableGroups is None:
            return

        if isinstance(availableGroups, list):
            schema = self.getFullSchema()
            if (schema.hasDisplayType(key) and
                    schema.getDisplayType(key) == 'Table'):
                rowSchema = schema.getParameterHash().getAttribute(
                        key, KARABO_SCHEMA_ROW_SCHEMA)
                self._reconfigureAvailableGroups(availableGroups, rowSchema)
                self._updateCompiledSourceList()

        self.log.DEBUG('============  preReconfigure end ============\n')

    def postReconfigure(self):
        self.log.DEBUG('************ postReconfigure ***************')
        self.log.DEBUG('************ availableGroups ***************\n')
        groups = self.get('availableGroups')
        for h in groups:
            self.log.DEBUG('...\n{}'.format(h))

        self.log.DEBUG('************ sources         ***************\n')

        sources = self.get('sources')
        for h in sources:
            self.log.DEBUG('...\n{}'.format(h))

        self._printConfig()

        self.log.DEBUG('******************************************\n\n\n')

    def buildConfigurationInUse(self):
        self.log.DEBUG('buildConfigurationInUse()')

        result = Hash()
        for group in self._configurations.values():
            instance_id = group.get('id')
            self._buildDataSourceProperties(
                group.get('expert'), instance_id, True, False, result)
            self._buildDataSourceProperties(
                group.get('user'), instance_id, False, True, result)
        configuration = Hash('configuration', result)

        self.log.INFO('Current Run Configuration is ...\n{}'
                      ''.format(configuration))

        self._ss.emit('signalRunConfiguration',
                      configuration, self.getInstanceId())

    def updateAvailableGroups(self):
        g = []
        for group in self._configurations.values():
            h = Hash()
            h.set('groupId', group.get('id'))
            h.set('description', group.get('description', default=''))
            h.set('use', group.get('use', default=False))
            g.append(h)

        self.set('availableGroups', g)

    def slotGetSourcesInGroup(self, group):
        result = Hash('group', group, 'instanceId', self.getInstanceId())
        self._makeGroupSourceConfig(result, self._groupToDevice.get(group, ''))
        self.reply(result)

    def _deviceGoneHandler(self, instanceId, instanceInfo):
        inst_type = instanceInfo.get('type', default='unknown')
        classId = instanceInfo.get('classId', default='?')

        if (inst_type != 'device' or classId != 'RunConfigurationGroup'):
            return

        self.log.DEBUG("instanceGoneHandler -->  instanceId  '{}' is erased."
                       "".format(instanceId))

        self.remote().unregisterDeviceMonitor(instanceId)

        if instanceId in self._configurations:
            del self._configurations[instanceId]
            self.updateAvailableGroups()

    def _newDeviceHandler(self, topologyEntry):
        inst_type = topologyEntry.getKeys()[0]  # fails if empty...
        if inst_type != 'device':
            return

        inst = topologyEntry.get(inst_type, default=None)
        instance_id = inst.getKeys()[0] if isinstance(inst, Hash) else '?'
        if instance_id == '?':
            return

        if (not inst.hasAttribute(instance_id, 'classId') or
                inst.getAttribute(instance_id, 'classId')
                != 'RunConfigurationGroup'):
            return

        # Add new configuration group into the map
        self._configurations[instance_id] = Hash()

        # register monitor to device
        self.remote().registerDeviceMonitor(instance_id,
                                            self.deviceUpdatedHandler)

    def deviceUpdatedHandler(self, deviceId, update):
        group = update.get('group', default=None)
        if group is not None:
            self._updateGroupConfiguration(deviceId, group)
            self.updateAvailableGroups()
            self._updateCompiledSourceList()
            # now notify clients
            result = Hash('group', group['id'],
                          'instanceId', self.getInstanceId())
            self._makeGroupSourceConfig(result, deviceId)
            self._ss.emit('signalGroupSourceChanged', result, deviceId)

    def _reconfigureAvailableGroups(self, groups, schema):
        for group in groups:
            group_id = group.get('groupId')
            device_id = self._groupToDevice[group_id]
            self.log.DEBUG('Updating group {} on device {}'
                           ''.format(group_id, device_id))
            existing = self._configurations.get(group_id)
            if existing is not None:
                use = group.get('use')
                existing.set('use', use)
                for src_type in ('expert', 'user'):
                    sources = existing.get(src_type, default=[])
                    for src in sources:
                        src.set('use', use)

    def _updateGroupConfiguration(self, deviceId, update):
        group = self._configurations[deviceId]
        if group.empty() or update.empty():
            # remote().get(..) is potentially blocking and should generally be
            # avoided.
            # Here, however, it will only block if `deviceId` sends an update
            # before the configuration/schema requests triggered by
            # remote().registerDeviceMonitor(..) have not yet been answered.
            group = self.remote().get(deviceId, 'group')
            self._configurations[deviceId] = group
            group.set('use', False)
        else:
            use = group.get('use')
            group.merge(update)
            group.set('use', use)

        self._groupToDevice[group.get('id')] = deviceId
        for src_type in ('expert', 'user'):
            if not group.has(src_type):
                group.set(src_type, [])
            else:
                for s in group.get(src_type):
                    s.set('use', False)

        self.log.DEBUG('Updated RunConfigurationGroup --> instanceId: {}'
                       ''.format(deviceId))

    def _updateCompiledSourceList(self):
        sources = {}
        for name, g in self._configurations.items():
            use = g.get('use')

            self.log.DEBUG('updateCompiledSourceList()  cursor : {}, use : {}'
                           ''.format(name, use))

            expert = g.get('expert')
            self._createSource(expert, sources, use)
            user = g.get('user')
            self._createSource(user, sources, use)

        self.set('sources', list(sources.values()))

    def _buildDataSourceProperties(self, table, groupId, expert, user, result):
        for group in table:
            source_id = group.get('source')
            pipeline = (group.getAttribute('source', 'pipeline')
                        if group.hasAttribute('source', 'pipeline') else False)
            behavior = group.get('behavior')
            monitorOut = group.get('monitored')

            self.log.DEBUG('buildDataSourceProperties source_id : {}, '
                           'pipeline : {}'.format(source_id, pipeline))

            if not group.get('use'):
                continue

            # It was decided not to send all properties to the PCLayer.
            # The call to 'getDataSourceSchemaAsHash()' will be done by PCLayer
            # software like ...
            # ----------------------------------------------------------------
            # int access = 0;
            # if (behavior == 'record-all') access = INIT|READ|WRITE;
            # else if (behavior == 'read-only') access = INIT|READ;
            # else access = INIT;
            # remote().getDataSourceSchemaAsHash(source_id, properties,
            #                                    access);
            # ----------------------------------------------------------------
            # The PCLayer software may call this many times ...

            # Instead here we just send a stub
            # ('data source' granularity level)
            properties = Hash(source_id, Hash())
            properties.setAttribute(source_id, 'configurationGroupId', groupId)
            properties.setAttribute(source_id, 'pipeline', pipeline)
            properties.setAttribute(source_id, 'expertData', expert)
            properties.setAttribute(source_id, 'userData', user)
            properties.setAttribute(source_id, 'behavior', behavior)
            properties.setAttribute(source_id, 'monitorOut', monitorOut)
            result.merge(properties, HashMergePolicy.REPLACE_ATTRIBUTES)

    def _createSource(self, data, sources, use):
        for d in data:
            d.set('use', use)
            if not use:
                continue

            src = d.get('source')
            pipeline = (d.getAttribute('source', 'pipeline')
                        if d.hasAttribute('source', 'pipeline') else False)
            inst_type = d.get('type')
            behavior = d.get('behavior')
            h = Hash('source', src,
                     'type', inst_type,
                     'behavior', behavior,
                     'monitored', d.get('monitored'),
                     'use', use)
            h.setAttribute('source', 'pipeline', pipeline)

            if src in sources:
                existing = sources[src]
                if existing.get('monitored'):
                    h.set('monitored', True)

                exbehavior = existing.get('behavior')
                if (behavior == 'init' or
                        (behavior == 'read-only' and exbehavior != 'init')):
                    h.set('behavior', exbehavior)

            sources[src] = h

    def _makeGroupSourceConfig(self, result, device_id):
        sources = []
        group = self._configurations.get(device_id)
        if group is not None:
            for src_type in ('expert', 'user'):
                src_group = group.get(src_type, default=[])
                for src in src_group:
                    src.erase('use')
                    src.set('access', src_type)
                    sources.append(src)

        result.set('sources', sources)

    def _printConfig(self):
        self.log.DEBUG('\n\nConfigurations are ...\n')
        for deviceId, group in self._configurations.items():
            desc = (group.get('description') if group.has('group') else '')
            self.log.DEBUG('deviceId: {}, groupId: {}, desc: {}, use: {}'
                           ''.format(deviceId, group.get('id'), desc,
                                     group.get('use')))

            for src_type, src_name in (('expert', 'Expert'), ('user', 'User')):
                self.log.DEBUG('\t' + src_name)
                sources = group.get(src_type)
                for src in sources:
                    self.log.DEBUG('\tsource: {}, type: {}, behavior: {}'
                                   ', monitored: {}, use: {}'.format(
                                       src.get('source'), src.get('type'),
                                       src.get('behavior'),
                                       src.get('monitored'), src.get('use')))
