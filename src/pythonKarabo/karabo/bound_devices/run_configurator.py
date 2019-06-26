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
from karabo.common.api import (
    KARABO_SCHEMA_DISPLAY_TYPE_RUNCONFIGURATOR as DT_RUNCONF,
    KARABO_SCHEMA_DISPLAY_TYPE_SCENES as DT_SCENES)
from karabo.common.scenemodel.api import (
    BoxLayoutModel, DeviceSceneLinkModel, DisplayCommandModel,
    FixedLayoutModel, LabelModel, RunConfiguratorModel, SceneModel,
    SceneTargetWindow, write_scene
)
from .run_configuration_group import RunControlDataSource

OUTPUT_CHANNEL_SEPARATOR = ':'


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
            .reconfigurable().commit(),
        )


@KARABO_CLASSINFO('RunConfigurator', '2.2')
class RunConfigurator(PythonDevice):
    """RunConfigurator karabo device

    The RunConfigurator device aggregates all RunConfigurationGroups into a
    single table and -on demand- sends this table to the DAQ in a compatible
    format. It does not check whether the sources exist or not!

    The operation of the device:
    The RunConfigurator device constantly monitors the system topology.
    Whenever a RunConfigurationGroup device comes up, it ads it to the
    available configuration groups. You can select which groups you want to
    record and push your selection to the DAQ. This will convert the selection
    to the Hash format required by the DAQ and send it to reconfigure the
    RunController.
    """
    def __init__(self, configuration):
        super(RunConfigurator, self).__init__(configuration)

        # Declare member variables
        self._runConfigGroups = OrderedDict()
        self._groupToDevice = {}

        # Define the signals & slots
        self._ss.registerSystemSignal('signalRunConfiguration', Hash, str)
        self.KARABO_SLOT(self.buildConfigurationInUse)
        self.KARABO_SLOT(self.requestScene)

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
            .setSpecialDisplayType(DT_RUNCONF)
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
            .setSpecialDisplayType(DT_SCENES)
            .readOnly().initialValue(['scene', 'link'])
            .commit(),

        )

    def initialization(self):
        # Register instance handlers for devices joining/leaving the topology
        # `_newDeviceHandler` will be called for each new instance joining
        # `_deviceGoneHandler` will be called for each instance leaving
        # If the device is a RunConfigurationGroup they add/remove it from the
        # list of available groups.
        self.remote().registerInstanceNewMonitor(self._newDeviceHandler)
        self.remote().registerInstanceGoneMonitor(self._deviceGoneHandler)

        # Switch on the heartbeat tracking
        self.remote().enableInstanceTracking()

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

    ###########################################################################
    #                         Slot methods                                    #
    ###########################################################################

    def buildConfigurationInUse(self, dummy=None):
        """This is the slot that initiates DAQ reconfiguration

        It follows good WYSIWYG principles and generates the Hash() of active
        sources from the displayed entries 'configurations' table. Each entry
        is added by _gatherSourceProperties(...), so that function does the
        actual formatting. Finally it emits a signal with the prepared Hash.
        """
        result = Hash()
        for group in self.get('configurations'):
            self._gatherSourceProperties(group, result)

        if result.empty():
            self.log.WARN("Skipping sending empty configuration to DAQ "
                          "to protect the RunController.")
            return

        configuration = Hash('configuration', result)

        self.log.INFO('Current Run Configuration is ...\n%s' % configuration)

        self._ss.emit('signalRunConfiguration',
                      configuration, self.getInstanceId())

    def _gatherSourceProperties(self, group, result):
        """This method formats an entry in `configurations` to the Hash()
        format required by the DAQ. It does not use any non-visible information
        as this is a high risk bug source...).
        """
        # Abort if the group is not used
        if not group.get('_RunConfiguratorGroup.use'):
            return

        groupId = group.get('_RunConfiguratorGroup.groupId')

        for source in group.get('_RunConfiguratorGroup.sources'):
            source_id = source.get('source')
            # This is the same as in RunConfigurationGroup
            pipeline = True if OUTPUT_CHANNEL_SEPARATOR in source_id else False
            behavior = source.get('behavior')
            monitorOut = source.get('monitored')

            expert = True if source.get('access') == 'expert' else False
            user = True if source.get('access') == 'user' else False

            self.log.DEBUG('buildDataSourceProperties source_id : {}, '
                           'pipeline : {}'.format(source_id, pipeline))

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

    ###########################################################################
    #                            Callback methods                             #
    ###########################################################################
    #
    #     These methods register RunConfigurationGroups going up or down and
    # being updated. Once triggered, they update the internal state and call
    # for reconfiguration.

    def _deviceGoneHandler(self, instanceId, instanceInfo):
        """This method is called whenever a device goes down. If it was one of
        the RunConfigurationGroups, it removes it from the list of available
        groups and triggers a reconfiguration.
        """
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
        """This method is called whenever a new device is registered in the
        system topology. If it's a RunConfigurationGroup, it will automatically
        append it to the available groups and registers a monitor for updates.
        """
        inst_type = topologyEntry.getKeys()[0]  # fails if empty...
        if inst_type != 'device':
            return

        inst = topologyEntry.get(inst_type, default=None)
        instance_id = inst.getKeys()[0] if isinstance(inst, Hash) else '?'
        if instance_id == '?':
            return

        # If the new device is a RunConfiguratorGroup
        #     We have already checked that its a device with valid id
        if inst.getAttribute(instance_id, 'classId') \
                == 'RunConfigurationGroup':
            # Add new configuration group into the map
            self._runConfigGroups[instance_id] = Hash()
            # register monitor to device
            self.remote().registerDeviceMonitor(
                instance_id, self._deviceUpdatedHandler)

    def _deviceUpdatedHandler(self, deviceId, update):
        """This method is called whenever it's associated RunConfigurationGroup
         is updated. It takes care of properly updating `self.configurations`
         and all the internal states by first updating the internal state then
         triggering a reconfiguration.
         """
        group = update.get('group', default=None)
        if group is not None:
            self._updateRunConfiguratorGroupInstance(deviceId, group)
            self._updateDependentProperties()

    ###########################################################################
    #                         State update methods                            #
    ###########################################################################
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

        I.E.: It generates the visible entries from the internal state.
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

            # `existing sources but check before for None`
            expert_sources = group.get('expert')
            user_sources = group.get('user')
            if expert_sources is not None:
                _creatSource(expert_sources, sources, use)
            if user_sources is not None:
                _creatSource(user_sources, sources, use)

        h.set('configurations', configurations)
        h.set('availableGroups', available_groups)
        h.set('sources', list(sources.values()))

        # Assignment of empty hash for table element is invalid, so we purge
        if len(configurations) == 0:
            while len(self.get('configurations')) > 0:
                del self.get('configurations')[0]
        self.set(h)

    def _updateRunConfiguratorGroupInstance(self, deviceId, update):
        """Update the local configuration Hash (internal state) stored for a
        RunConfigurationGroup device instance.
        """
        group = self._runConfigGroups[deviceId]
        use_group = group.get('use', default=False)

        if group.empty() or update.empty():
            # remote().get(..) is potentially blocking and should generally be
            # avoided. Here, however, it will only block if `deviceId` sends
            # an update before the configuration/schema requests triggered by
            # remote().registerDeviceMonitor(..) have not yet been answered.
            group = self.remote().get(deviceId, 'group')
            self._runConfigGroups[deviceId] = group
            group.set('use', False)
        else:
            group.merge(update)
            group.set('use', use_group)

        self._groupToDevice[group.get('id')] = deviceId
        for src_type in ('expert', 'user'):
            if not group.has(src_type):
                group.set(src_type, [])
            else:
                for s in group.get(src_type):
                    s.set('use', use_group)

        msg = 'Updated RunConfigurationGroup --> instanceId: %s\n' \
              'The new value is:\n%s' % (deviceId, group)
        self.log.DEBUG(msg)

    ###########################################################################
    #                         Scene related methods                           #
    ###########################################################################
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


def _createScene(instance_id):
    DEFAULT_FONT = ',11,-1,5,50,0,0,0,0,0'
    label = LabelModel(font=DEFAULT_FONT, foreground='#000000',
                       text='Available group configurations',
                       height=31, width=205, x=4, y=4)
    table = RunConfiguratorModel(
        keys=[instance_id + '.configurations'],
        height=400, width=600, x=4, y=36,
        parent_component='EditableApplyLaterComponent')
    # button = DisplayCommandModel(
    #     keys=[instance_id + '.buildConfigurationInUse'],
    #     height=29, width=101, x=495, y=440)
    link = DeviceSceneLinkModel(
        keys=[instance_id + '.availableScenes'], target='link',
        font=DEFAULT_FONT, foreground='#000000', frame_width=1,
        text='Run Groups', target_window=SceneTargetWindow.Dialog,
        height=29, width=101, x=495, y=475)

    layout = FixedLayoutModel(height=490, width=600, x=4, y=4,
                              children=[table, label, link])

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
