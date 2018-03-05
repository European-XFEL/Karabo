#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 27, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import OrderedDict

from karabo.bound import (
    Hash, HashMergePolicy, PythonDevice, Schema, State, VectorHash,
    EXPERT, KARABO_CLASSINFO, KARABO_CONFIGURATION_BASE_CLASS, BOOL_ELEMENT,
    LIST_ELEMENT, OVERWRITE_ELEMENT, SLOT_ELEMENT, STRING_ELEMENT,
    TABLE_ELEMENT, VECTOR_STRING_ELEMENT
)
from karabo.common.scenemodel.api import (
    BoxLayoutModel, DeviceSceneLinkModel, DisplayCommandModel,
    FixedLayoutModel, LabelModel, RunConfiguratorModel, SceneModel, 
    SceneTargetWindow, write_scene
)
from .run_configuration_group import RunControlDataSource


@KARABO_CONFIGURATION_BASE_CLASS
@KARABO_CLASSINFO('_RunConfiguratorGroup', '2.2')
class _RunConfiguratorGroup(object):
    """The description of a single run configurator group
    NOTE: This is an implementation detail for use with LIST_ELEMENT.
    """
    @staticmethod
    def expectedParameters(expected):
        sourceRow = Schema()
        (
            STRING_ELEMENT(expected).key('groupId')
            .displayedName('Group Name')
            .description('Run configuration group name.')
            .assignmentMandatory()
            .reconfigurable().commit(),

            STRING_ELEMENT(expected).key('description')
            .displayedName('Description')
            .description('Run configuration group description.')
            .assignmentOptional().defaultValue('')
            .reconfigurable().commit(),

            BOOL_ELEMENT(expected).key('use')
            .displayedName('Use')
            .description('Run configuration group usage flag.')
            .assignmentOptional().defaultValue(False)
            .reconfigurable().commit(),

            # Sub-schema of the 'sources' table
            STRING_ELEMENT(sourceRow).key('source')
            .displayedName('Source')
            .assignmentOptional().defaultValue('Source')
            .reconfigurable().commit(),

            STRING_ELEMENT(sourceRow).key('type')
            .displayedName('Type')
            .options('control,instrument')
            .assignmentOptional().defaultValue('control')
            .reconfigurable().commit(),

            STRING_ELEMENT(sourceRow).key('behavior')
            .displayedName('Behavior')
            .options('init,read-only,record-all')
            .assignmentOptional().defaultValue('record-all')
            .reconfigurable().commit(),

            BOOL_ELEMENT(sourceRow).key('monitored')
            .displayedName('Monitor out')
            .assignmentOptional().defaultValue(False)
            .reconfigurable().commit(),

            STRING_ELEMENT(sourceRow).key('access')
            .displayedName('Access')
            .options('expert,user')
            .assignmentOptional().defaultValue('expert')
            .reconfigurable().commit(),

            TABLE_ELEMENT(expected).key('sources')
            .displayedName('Compiled source List')
            .description('Overall list of data sources and their attributes')
            .setColumns(sourceRow)
            .assignmentOptional().defaultValue([])
            .commit(),
        )


@KARABO_CLASSINFO('RunConfigurator', '2.2')
class RunConfigurator(PythonDevice):

    def __init__(self, configuration):
        super(RunConfigurator, self).__init__(configuration)
        self.registerInitialFunction(self.initialization)

    @staticmethod
    def expectedParameters(expected):
        """Description of device parameters
        """
        availGroupsRow = Schema()
        (
            STRING_ELEMENT(availGroupsRow).key('groupId')
            .displayedName('Group')
            .description('Run configuration group name.')
            .assignmentMandatory()
            .reconfigurable()
            .commit(),

            STRING_ELEMENT(availGroupsRow).key('description')
            .displayedName('Description')
            .description('Run configuration group description.')
            .assignmentOptional().defaultValue('')
            .reconfigurable()
            .commit(),

            BOOL_ELEMENT(availGroupsRow).key('use')
            .displayedName('Use')
            .description('Run configuration group usage flag.')
            .assignmentOptional().defaultValue(False)
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
        )

        (
            OVERWRITE_ELEMENT(expected).key('state')
            .setNewOptions(State.INIT, State.NORMAL, State.ERROR)
            .setNewDefaultValue(State.INIT).commit(),

            OVERWRITE_ELEMENT(expected).key('visibility')
            .setNewDefaultValue(EXPERT)
            .commit(),

            SLOT_ELEMENT(expected).key('buildConfigurationInUse')
            .displayedName('Push to DAQ')
            .description('Push current configuration structure to the DAQ Run '
                         'controller.')
            .allowedStates(State.NORMAL)
            .commit(),

            LIST_ELEMENT(expected).key('configurations')
            .displayedName('Configurations')
            .description('All configuration groups and their sources')
            .appendNodesOfConfigurationBase(_RunConfiguratorGroup)
            # This signals to the GUI what type of widget to use
            .setSpecialDisplayType('RunConfigurator')
            .assignmentOptional().defaultValue([])
            .reconfigurable()
            .commit(),

            TABLE_ELEMENT(expected).key('availableGroups')
            .displayedName('Available group configurations')
            .setColumns(availGroupsRow)
            .assignmentOptional().defaultValue([])
            .reconfigurable().commit(),

            TABLE_ELEMENT(expected).key('sources')
            .displayedName('Compiled source List')
            .description('Overall list of data sources and their attributes')
            .setColumns(sourceRow)
            .assignmentOptional().defaultValue([])
            .commit(),

            VECTOR_STRING_ELEMENT(expected).key('availableScenes')
            .readOnly().initialValue(['scene', 'link'])
            .commit(),

        )

    def initialization(self):
        # Define the signals & slots
        self._ss.registerSystemSignal('signalRunConfiguration', Hash, str)
        self._ss.registerSystemSignal('signalGroupSourceChanged', Hash, str)
        self.KARABO_SLOT(self.buildConfigurationInUse)
        self.KARABO_SLOT(self.updateAvailableGroups)
        self.KARABO_SLOT(self.slotGetSourcesInGroup)
        self.KARABO_SLOT(self.requestScene)

        # Register instance handlers
        self.remote().registerInstanceNewMonitor(self._newDeviceHandler)
        self.remote().registerInstanceGoneMonitor(self._deviceGoneHandler)

        # Switch on the heartbeat tracking
        # `_newDeviceHandler` will be called for each instance arriving in the
        # system topology (and `_deviceGoneHandler` for the ones leaving)
        self.remote().enableInstanceTracking()

        self._runConfigGroups = OrderedDict()
        self._groupToDevice = {}

        # Ready to go!
        self.updateState(State.NORMAL)

    def preReconfigure(self, incomingReconfiguration):
        """Watch for 'availableGroups' and 'configurations' in incoming
        device configurations.
        """
        needs_update = False

        groups = incomingReconfiguration.get('availableGroups', default=None)
        if groups is not None and isinstance(groups, VectorHash):
            self._updateAvailableGroups(groups)
            incomingReconfiguration.erase('availableGroups')
            needs_update = True

        configs = incomingReconfiguration.get('configurations', default=None)
        if configs is not None and isinstance(configs, VectorHash):
            self._updateConfigurations(configs)
            incomingReconfiguration.erase('configurations')
            needs_update = True

        if needs_update:
            self._updateDependentProperties()

    # ----------------------------
    # SLOT methods

    def buildConfigurationInUse(self):
        result = Hash()
        for group in self._runConfigGroups.values():
            instance_id = group.get('id')
            self._gatherSourceProperties(group.get('expert'), instance_id,
                                         True, False, result)
            self._gatherSourceProperties(group.get('user'), instance_id,
                                         False, True, result)
        configuration = Hash('configuration', result)

        self.log.INFO('Current Run Configuration is ...\n{}'
                      ''.format(configuration))

        self._ss.emit('signalRunConfiguration',
                      configuration, self.getInstanceId())

    def updateAvailableGroups(self):
        """Do nothing. This SLOT is deprecated.
        """
        pass

    def slotGetSourcesInGroup(self, group):
        device_id = self._groupToDevice.get(group, '')
        result = Hash('group', group,
                      'instanceId', self.getInstanceId(),
                      'sources', self._getGroupSources(device_id))
        self.reply(result)

    def requestScene(self, params):
        """Fulfill a scene request from another device.

        NOTE: Required by Scene Supply Protocol, which is defined in KEP 21.
              The format of the reply is also specified there.

        :param params: A `Hash` containing the method parameters
        """
        payload = Hash('success', False)

        name = params.get('name', default='')
        if name == 'scene':
            payload.set('success', True)
            payload.set('name', name)
            payload.set('data', _createScene(self.getInstanceId()))
        elif name == 'link':
            groups = {val['id']: group
                      for group, val in self._runConfigGroups.items()}
            payload.set('success', True)
            payload.set('name', name)
            payload.set('data', _createLink(self.getInstanceId(), groups))

        self.reply(Hash('type', 'deviceScene',
                        'origin', self.getInstanceId(),
                        'payload', payload))

    # ----------------------------
    # Callback methods

    def _deviceGoneHandler(self, instanceId, instanceInfo):
        inst_type = instanceInfo.get('type', default='unknown')
        classId = instanceInfo.get('classId', default='?')

        if (inst_type != 'device' or classId != 'RunConfigurationGroup'):
            return

        self.log.DEBUG("instanceGoneHandler -->  instanceId  '{}' is erased."
                       "".format(instanceId))

        self.remote().unregisterDeviceMonitor(instanceId)

        if instanceId in self._runConfigGroups:
            del self._runConfigGroups[instanceId]
            self._updateDependentProperties()

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
        self._runConfigGroups[instance_id] = Hash()

        # register monitor to device
        self.remote().registerDeviceMonitor(instance_id,
                                            self._deviceUpdatedHandler)

    def _deviceUpdatedHandler(self, deviceId, update):
        group = update.get('group', default=None)
        if group is not None:
            self._updateRunConfiguratorGroupInstance(deviceId, group)
            self._updateDependentProperties()
            # now notify clients
            result = Hash('group', group['id'],
                          'instanceId', self.getInstanceId(),
                          'sources', self._getGroupSources(deviceId))
            self._ss.emit('signalGroupSourceChanged', result, deviceId)

    # ----------------------------
    # Private methods

    def _gatherSourceProperties(self, table, groupId, expert, user, result):
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

    def _getGroupSources(self, device_id):
        sources = []
        group = self._runConfigGroups.get(device_id)
        if group is not None:
            for src_type in ('expert', 'user'):
                src_group = group.get(src_type, default=[])
                for src in src_group:
                    src.erase('use')
                    src.set('access', src_type)
                    sources.append(src)
        return sources

    def _updateAvailableGroups(self, groups):
        """Update the `_runConfigGroups` dict when the `availableGroups`
        property is modified.
        """
        for group in groups:
            use = group.get('use')
            group_id = group.get('groupId')
            device_id = self._groupToDevice[group_id]
            self.log.DEBUG('Updating group {} on device {}'
                           ''.format(group_id, device_id))

            existing = self._runConfigGroups.get(device_id)
            if existing is not None:
                existing.set('use', use)
                for src_type in ('expert', 'user'):
                    sources = existing.get(src_type, default=[])
                    for src in sources:
                        src.set('use', use)

    def _updateConfigurations(self, configs):
        """Update the `_runConfigGroups` dict when the `configurations`
        property is modified.
        """
        for group in configs:
            use = group.get('_RunConfiguratorGroup.use')
            group_id = group.get('_RunConfiguratorGroup.groupId')
            device_id = self._groupToDevice[group_id]
            existing = self._runConfigGroups.get(device_id)
            if existing is not None:
                existing.set('use', use)
                for src_type in ('expert', 'user'):
                    sources = existing.get(src_type, default=[])
                    for src in sources:
                        src.set('use', use)

    def _updateDependentProperties(self):
        """Update the `configurations`, `availableGroups`, and `sources`
        properties after an action modifies the `_runConfigGroups` dict.
        """
        h = Hash()
        configurations, available_groups, sources = [], [], {}

        source_attrs = ('source', 'type', 'behavior', 'monitored')
        for name, group in self._runConfigGroups.items():
            group_id = group.get('id')
            description = group.get('description', default='')
            use = group.get('use', default=False)

            # `configurations`
            grp_sources = []
            for src_type in ('expert', 'user'):
                src_group = group.get(src_type, default=[])
                for src in src_group:
                    src_config = Hash('access', src_type)
                    [src_config.set(a, src.get(a)) for a in source_attrs]
                    grp_sources.append(src_config)
            grp_config = Hash('groupId', group_id, 'use', use,
                              'description', description,
                              'sources', grp_sources)
            configurations.append(Hash('_RunConfiguratorGroup', grp_config))

            # `availableGroups`
            available_groups.append(Hash('groupId', group_id,
                                         'description', description,
                                         'use', use))

            # `sources`
            _creatSource(group.get('expert'), sources, use)
            _creatSource(group.get('user'), sources, use)

        h.set('configurations', configurations)
        h.set('availableGroups', available_groups)
        h.set('sources', list(sources.values()))
        self.set(h)

    def _updateRunConfiguratorGroupInstance(self, deviceId, update):
        """Update the local configuration Hash stored for a
        RunConfigurationGroup device instance.
        """
        group = self._runConfigGroups[deviceId]
        if group.empty() or update.empty():
            # remote().get(..) is potentially blocking and should generally be
            # avoided.
            # Here, however, it will only block if `deviceId` sends an update
            # before the configuration/schema requests triggered by
            # remote().registerDeviceMonitor(..) have not yet been answered.
            group = self.remote().get(deviceId, 'group')
            self._runConfigGroups[deviceId] = group
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


def _createScene(instance_id):
    DEFAULT_FONT = ',11,-1,5,50,0,0,0,0,0'
    label = LabelModel(font=DEFAULT_FONT, foreground='#000000',
                       text='Available group configurations',
                       height=31, width=205, x=4, y=4)
    table = RunConfiguratorModel(
        keys=[instance_id + '.configurations'],
        height=400, width=600, x=4, y=36,
        parent_component='EditableApplyLaterComponent')
    button = DisplayCommandModel(
        keys=[instance_id + '.buildConfigurationInUse'],
        height=29, width=101, x=495, y=440)
    link = DeviceSceneLinkModel(
        keys=[instance_id + '.availableScenes'], target='link',
        font=DEFAULT_FONT, foreground='#000000', frame_width=1,
        text='Run Groups', target_window=SceneTargetWindow.Dialog,
        height=29, width=101, x=495, y=475)

    layout = FixedLayoutModel(height=490, width=600, x=4, y=4,
                              children=[button, table, label, link])

    exp_0 = LabelModel(font=DEFAULT_FONT,
                       height=28, width=338, x=644, y=15,
                       parent_component='DisplayComponent',
                       text='NOTE: You must click the green check mark')
    exp_1 = LabelModel(font=DEFAULT_FONT,
                       height=28, width=321, x=650, y=74,
                       parent_component='DisplayComponent',
                       text='to apply your changes before clicking the')
    exp_2 = LabelModel(font=DEFAULT_FONT,
                       height=28, width=231, x=656, y=155,
                       parent_component='DisplayComponent',
                       text=' "Push to DAQ" button.')
    exp_layout = BoxLayoutModel(direction=2,
                                height=77, width=346, x=126, y=443,
                                children=[exp_0, exp_1, exp_2])

    return write_scene(SceneModel(children=[layout, exp_layout],
                                  height=530, width=610))


def _createLink(instance_id, run_groups):
    DEFAULT_FONT = ',11,-1,5,50,0,0,0,0,0'
    label = LabelModel(font=DEFAULT_FONT, foreground='#000000',
                       text='Links to Run Configuration Groups',
                       height=31, width=205, x=4, y=4)
    group_layouts = []
    for group_name, group_device_id in run_groups.items():
        key = "{}.availableScenes".format(group_device_id)
        link = DeviceSceneLinkModel(
            keys=[key], target="scene", font=DEFAULT_FONT, frame_width=1,
            foreground='#000000', target_window=SceneTargetWindow.Dialog,
            text=group_name, width=205)
        group_layouts.append(BoxLayoutModel(children=[link]))

    groups = BoxLayoutModel(direction=2, y=60, x=4, children=group_layouts)
    return write_scene(SceneModel(children=[label, groups]))


def _creatSource(sources, existing_sources, use):
    for s in sources:
        s.set('use', use)
        if not use:
            continue

        src_id = s.get('source')
        pipeline = (s.getAttribute('source', 'pipeline')
                    if s.hasAttribute('source', 'pipeline') else False)
        behavior = s.get('behavior')
        h = Hash('source', src_id,
                 'type', s.get('type'),
                 'behavior', behavior,
                 'monitored', s.get('monitored'),
                 'use', use)
        h.setAttribute('source', 'pipeline', pipeline)

        if src_id in existing_sources:
            existing = existing_sources[src_id]
            if existing.get('monitored'):
                h.set('monitored', True)

            exbehavior = existing.get('behavior')
            if (behavior == 'init' or
                    (behavior == 'read-only' and exbehavior != 'init')):
                h.set('behavior', exbehavior)

        existing_sources[src_id] = h
