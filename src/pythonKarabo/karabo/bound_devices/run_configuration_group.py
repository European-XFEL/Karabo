#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 26, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import enum
import os
import os.path as op

from karabo.bound import (
    PythonDevice, Hash, loadFromFile, saveToFile, Schema, State,
    TextSerializerSchema,
    ADMIN, EXPERT, KARABO_CLASSINFO,
    BOOL_ELEMENT, OVERWRITE_ELEMENT, NODE_ELEMENT, SLOT_ELEMENT, 
    STRING_ELEMENT, TABLE_ELEMENT, UINT32_ELEMENT, VECTOR_STRING_ELEMENT
)
from karabo.common.scenemodel.api import (
    BoxLayoutModel, DisplayCommandModel, FixedLayoutModel, LabelModel,
    LineEditModel, SceneModel, TableElementModel, write_scene,
)

OUTPUT_CHANNEL_SEPARATOR = ':'
SAVED_GROUPS_DIR = 'run_config_groups'


class AccessMode(enum.Enum):
    INIT = 1 << 0
    READ = 1 << 1
    WRITE = 1 << 2


@KARABO_CLASSINFO('RunControlDataSource', '2.2')
class RunControlDataSource(object):
    """The description of a single run control data source.
    """
    @staticmethod
    def expectedParameters(expected):
        (
            OVERWRITE_ELEMENT(expected).key('visibility')
            .setNewDefaultValue(ADMIN)
            .commit(),

            STRING_ELEMENT(expected).key('source')
            .displayedName('Source')
            .description("Data source's full name, like "
                         "SASE1/SPB/SAMP/INJ_CAM_1")
            .assignmentOptional().defaultValue('Source')
            .reconfigurable()
            .commit(),

            UINT32_ELEMENT(expected).key("nProperties")
            .displayedName("#")
            .description("Number of properties to record for this source. Not "
                         "active feature yet.")
            .readOnly()
            .initialValue(0)
            .commit(),

            STRING_ELEMENT(expected).key('type')
            .displayedName('Type')
            .description("Data source's type")
            # XXX: Uncommenting the following line causes the GUI to crash...
            # .options('control,instrument')
            .assignmentOptional().defaultValue('control')
            .reconfigurable()
            .commit(),

            STRING_ELEMENT(expected).key('behavior')
            .displayedName('Behavior')
            .description("Configure data source's behavior")
            .options('init,read-only,record-all')
            .assignmentOptional().defaultValue('record-all')
            .reconfigurable()
            .commit(),

            BOOL_ELEMENT(expected).key('monitored')
            .displayedName('Monitor out')
            .description("If true, the selected data will be output to the "
                         "online pipeline outputs in the DAQ's monitoring "
                         "and recording states.")
            .assignmentOptional().defaultValue(False)
            .reconfigurable()
            .commit(),

            BOOL_ELEMENT(expected).key('inUse')
            .displayedName('In use')
            .description("If true, the device's data are being recorded by "
                         "the DAQ. If a device is false, contact ITDM.")
            .readOnly().initialValue(True)
            .commit(),
        )


@KARABO_CLASSINFO('RunConfigurationGroup', '2.2')
class RunConfigurationGroup(PythonDevice):

    def __init__(self, configuration):
        # always call PythonDevice constructor first!
        super(RunConfigurationGroup, self).__init__(configuration)
        # Define first function to be called after the constructor has finished
        self.registerInitialFunction(self.initialization)

    @staticmethod
    def expectedParameters(expected):
        """Description of device parameters
        """
        sourceSchema = Schema()
        RunControlDataSource.expectedParameters(sourceSchema)

        (
            OVERWRITE_ELEMENT(expected).key('state')
            .setNewOptions(State.INIT, State.NORMAL, State.ERROR)
            .setNewDefaultValue(State.INIT)
            .commit(),

            OVERWRITE_ELEMENT(expected).key('visibility')
            .setNewDefaultValue(EXPERT)
            .commit(),

            NODE_ELEMENT(expected).key('group')
            .displayedName('Group')
            .description('Structure describing data sources logically '
                         'belonging together.')
            .commit(),

            STRING_ELEMENT(expected).key('group.id')
            .displayedName('Name')
            .description('Name of run configuration group.')
            .assignmentMandatory()
            .commit(),

            STRING_ELEMENT(expected).key('group.description')
            .displayedName('Description')
            .description('Description of current run configuration group.')
            .assignmentOptional().noDefaultValue()
            .reconfigurable()
            .commit(),

            TABLE_ELEMENT(expected).key('group.expert')
            .displayedName('Mandatory sources')
            .description('Expert configurations for mandatory data sources')
            .setColumns(sourceSchema)
            .assignmentOptional().noDefaultValue()
            .reconfigurable()
            .commit(),

            TABLE_ELEMENT(expected).key('group.user')
            .displayedName('Optional sources')
            .description('User selectable data sources.')
            .setColumns(sourceSchema)
            .assignmentOptional().noDefaultValue()
            .reconfigurable()
            .commit(),

            NODE_ELEMENT(expected).key('owner')
            .displayedName('Owner')
            .description('The person to contact regarding usage of this group')
            .commit(),

            STRING_ELEMENT(expected).key('owner.name')
            .displayedName('Name')
            .description('Contact person name')
            .assignmentMandatory()
            .commit(),

            STRING_ELEMENT(expected).key('owner.email')
            .displayedName('Email')
            .description('Contact person email')
            .assignmentOptional().noDefaultValue()
            .commit(),

            STRING_ELEMENT(expected).key('owner.lastVerifiedDate')
            .displayedName('Last Verified Date')
            .description('The date it was last checked for DAQ compliance')
            .assignmentOptional().noDefaultValue()
            .reconfigurable()
            .commit(),

            # Converted internally into group_saveGroupConfiguration
            SLOT_ELEMENT(expected).key('group.saveGroupConfiguration')
            .displayedName('Save configuration')
            .description("Push the button to save configuration in "
                         "run_config_group' folder.")
            .commit(),

            VECTOR_STRING_ELEMENT(expected).key('availableScenes')
            .readOnly().initialValue(['scene'])
            .commit(),

        )

    def initialization(self):
        # Define the signals & slots
        self._ss.registerSystemSignal('signalGetGroup', str, Hash)
        self.KARABO_SLOT(self.slotGetGroup)
        self.KARABO_SLOT(self.group_saveGroupConfiguration)
        self.KARABO_SLOT(self.requestScene)

        os.makedirs(SAVED_GROUPS_DIR, exist_ok=True)

        group = Hash()
        path = op.join(SAVED_GROUPS_DIR, self.getInstanceId() + '.xml')
        if op.exists(path):
            loadFromFile(group, path)
        self.set('group', group)

        self.updateState(State.NORMAL)

    def preReconfigure(self, incomingReconfiguration):
        self.log.DEBUG('RunConfigurationGroup.preReconfigure ... '
                       'incomingReconfiguration ==> ...\n'
                       '{}'.format(incomingReconfiguration))

        if not incomingReconfiguration.has('group'):
            return

        inputGroup = incomingReconfiguration.get('group')
        currentGroup = self.get('group')

        if inputGroup.has('expert'):
            experts = []
            currentExperts = currentGroup.get('expert', default=[])
            inputExperts = inputGroup.get('expert')
            experts = self._fillTable(currentExperts, inputExperts)
            # update expert table
            inputGroup.set('expert', experts)

        if inputGroup.has('user'):
            currentUsers = currentGroup.get('user', default=[])
            inputUsers = inputGroup.get('user')
            users = self._fillTable(currentUsers, inputUsers)
            # set new version of user table
            inputGroup.set('user', users)

    def slotGetGroup(self):
        self._ss.emit('signalGetGroup',
                      self.getInstanceId(), self.get('group'))

    def group_saveGroupConfiguration(self):
        """The group.saveGroupConfiguration Slot
        """
        group = self.get('group')
        path = op.join(SAVED_GROUPS_DIR, self.getInstanceId() + '.xml')
        saveToFile(group, path)

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
            payload.set('data', _generateDeviceScene(self.getInstanceId()))

        self.reply(Hash('type', 'deviceScene',
                        'origin', self.getInstanceId(),
                        'payload', payload))

    def _fillTable(self, currentSources, inputSources):
        retSources = []
        for source in inputSources:
            deviceId = source.get('source')

            if OUTPUT_CHANNEL_SEPARATOR in deviceId:
                source.setAttribute('source', 'pipeline', False)
            else:
                source.setAttribute('source', 'pipeline', True)

            retSources.append(source)

            if source.getAttribute('source', 'pipeline'):
                continue  # output channel already added for the source

            ochannels = self.remote().getOutputChannelNames(deviceId)
            if _findDataSource(currentSources, deviceId) is None:
                # ... is new one ... get its output channels and add them
                # to the retSources ...
                for name in ochannels:
                    fullName = deviceId + OUTPUT_CHANNEL_SEPARATOR + name
                    retSources.append(_buildSource(fullName))
            else:
                # ... exists. Check if its output channels are in the
                # retSources and ... if not, add them to the retSources ...
                for name in ochannels:
                    fullName = deviceId + OUTPUT_CHANNEL_SEPARATOR + name
                    if _findDataSource(currentSources, fullName) is None:
                        # Existing device still missing output channel
                        retSources.append(_buildSource(fullName))

        # The update of properties causes timeout's, no async features
        # available, temporarily disabled feature
        # self._updateNProperties(retSources)
        return retSources

    def _updateNProperties(self, sources):
        for source in sources:
            sourceId = source.get("source")
            behavior = source.get("behavior")
            accessMode = AccessMode.INIT.value
            if behavior == "init":
                accessMode |= AccessMode.READ.value
            elif behavior == "record-all":
                accessMode |= (AccessMode.READ.value | AccessMode.WRITE.value)

            props = self.remote().getDataSourceSchemaAsHash(sourceId,
                                                            accessMode)
            source.set("nProperties", len(props.getPaths()))


def _buildSource(name):
    source = Hash('source', name, 'type', 'control',
                  'behavior', 'read-only', 'monitored', False)
    source.setAttribute('source', 'pipeline', True)
    return source


def _findDataSource(hashes, instance_id):
    for hsh in hashes:
        if hsh.get('source', default='') == instance_id:
            return hsh
    return None


def _generateDeviceScene(instance_id):
    DEFAULT_FONT = ",10,-1,5,50,0,0,0,0,0"

    button_key = instance_id + '.group.saveGroupConfiguration'
    button = DisplayCommandModel(keys=[button_key],
                                 x=475, y=525, height=30, width=135)
    user_label = LabelModel(text="User Sources", font=DEFAULT_FONT,
                            x=25, y=315, height=30, width=90)
    expert_label = LabelModel(text="Expert Sources", font=DEFAULT_FONT,
                              x=20, y=100, height=30, width=100)

    descr_key = instance_id + '.group.description'
    descr_label = LabelModel(text="Description", font=DEFAULT_FONT,
                             x=40, y=50, height=30, width=80)
    descr_disp = LineEditModel(klass='DisplayLineEdit', keys=[descr_key],
                               x=120, y=50, height=30, width=170)
    descr_edit = LineEditModel(klass='EditableLineEdit', keys=[descr_key],
                               x=290, y=50, height=30, width=170)
    descr_layout = BoxLayoutModel(direction=0,
                                  x=35, y=50, height=45, width=430,
                                  children=[descr_label, descr_disp,
                                            descr_edit])

    name_key = instance_id + '.group.id'
    name_label = LabelModel(text="Name", font=DEFAULT_FONT,
                            x=80, y=15, height=30, width=40)
    name_disp = LineEditModel(klass='DisplayLineEdit', keys=[name_key],
                              x=120, y=15, height=30, width=130)
    name_layout = BoxLayoutModel(direction=0, x=75, y=10, height=40, width=180,
                                 children=[name_label, name_disp])

    # Generate the table schema
    column_schema = Schema()
    RunControlDataSource.expectedParameters(column_schema)
    serializer = TextSerializerSchema.create('Xml')
    column_schema = serializer.save(column_schema)

    expert_key = instance_id + '.group.expert'
    expert_table = TableElementModel(klass='EditableTableElement',
                                     keys=[expert_key],
                                     column_schema=column_schema,
                                     x=120, y=90, height=210, width=500)

    user_key = instance_id + '.group.user'
    user_table = TableElementModel(klass='EditableTableElement',
                                   keys=[user_key],
                                   column_schema=column_schema,
                                   x=120, y=310, height=210, width=500)

    root_layout = FixedLayoutModel(x=10, y=10, height=550, width=600,
                                   children=[button, user_label, expert_label,
                                             descr_layout, name_layout,
                                             expert_table, user_table])

    exp_0 = LabelModel(font=',11,-1,5,50,0,0,0,0,0',
                       height=72, width=111, x=644, y=15,
                       parent_component='DisplayComponent',
                       text='NOTE: You must click the green check mark')
    exp_1 = LabelModel(font=',11,-1,5,50,0,0,0,0,0',
                       height=72, width=110.0, x=650, y=74,
                       parent_component='DisplayComponent',
                       text='to apply your changes before clicking the')
    exp_2 = LabelModel(font=',11,-1,5,50,0,0,0,0,0',
                       height=72, width=111, x=656, y=155,
                       parent_component='DisplayComponent',
                       text=' "Save Configuration" button.')
    exp_layout = BoxLayoutModel(direction=2,
                                height=77, width=346, x=126, y=531,
                                children=[exp_0, exp_1, exp_2])
    return write_scene(SceneModel(children=[root_layout, exp_layout]))
