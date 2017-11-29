from PyQt4.QtCore import QModelIndex, Qt

from karabogui.testing import GuiTestCase
from ..model import AlarmModel
from ..const import (ACKNOWLEDGE, ADD_UPDATE_TYPE, AlarmEntry,
                     ALARM_DATA, ALARM_HIGH,
                     ALARM_TYPE, DEVICE_ID, INIT_UPDATE_TYPE,
                     PROPERTY, REMOVE_UPDATE_TYPE)


def _data():
    ret = []
    for i in range(10):
        data = AlarmEntry(
            id=i,
            timeOfFirstOccurrence='2017-04-20T09:32:22 UTC',
            timeOfOccurrence='2017-04-20T09:32:22 UTC',
            deviceId='dev{}'.format(i),
            property='rent',
            type=ALARM_HIGH,
            description='rent is too damn high',
            acknowledge=(True, True),
            showDevice=None)
        ret.append(data)
    return ret


def _type(typ):
    return [typ for i in range(10)]


class TestModel(GuiTestCase):
    def setUp(self):
        super(TestModel, self).setUp()
        self.model = AlarmModel(instanceId='Bob')
        self.model.initAlarms('Bob', [], _data())

    def tearDown(self):
        pass

    def test_headers(self):
        hdrs = [v for k, v in ALARM_DATA.items()]
        assert self.model.headers == hdrs

    def test_initAlarms(self):
        mdl = AlarmModel(instanceId='Vinny')
        mdl.initAlarms('jenny', [], _data())
        assert mdl.allEntries == []

    def test_columnCount(self):
        assert self.model.columnCount() == 9

    def test_rowCount(self):
        assert self.model.rowCount() == 10

    def test_headerData(self):
        header = self.model.headerData(
            section=0,
            role=Qt.DisplayRole,
            orientation=Qt.Horizontal)
        assert header == 'ID'

    def test_updateAlarms(self):
        mdl = AlarmModel(instanceId='Vinny')
        mdl.initAlarms('Vinny', _type(INIT_UPDATE_TYPE), _data())
        assert mdl.rowCount() == 10

    def test_updateAlarms_wrongdev(self):
        mdl = AlarmModel(instanceId='Vinny')
        ret = mdl.initAlarms('jenny', _type(INIT_UPDATE_TYPE), _data())
        assert ret is None

    def test_updateAlarms_remove(self):
        self.model.updateAlarms('Bob',  _type(REMOVE_UPDATE_TYPE), _data())
        assert self.model.rowCount() == 0

    def test_updateAlarms_realarm(self):
        self.model.updateAlarms('Bob',  _type(REMOVE_UPDATE_TYPE), _data())
        assert self.model.rowCount() == 0
        self.model.updateAlarms('Bob',  _type(ADD_UPDATE_TYPE), _data())

    def test_updateFilter_device_id(self):
        self.model.updateFilter(filterType=DEVICE_ID, text='dev0')
        assert self.model.rowCount() == 1

    def test_updateFilter_wrong_property(self):
        self.model.updateFilter(filterType=PROPERTY, text='prettyness')
        assert self.model.rowCount() == 0

    def test_updateFilter_property(self):
        self.model.updateFilter(filterType=PROPERTY, text='rent')
        assert self.model.rowCount() == 10

    def test_updateFilter_wrongtype(self):
        self.model.updateFilter(filterType=ALARM_TYPE, text='warnLow')
        assert self.model.rowCount() == 0

    def test_updateFilter_righttype(self):
        self.model.updateFilter(filterType=ALARM_TYPE, text='alarmHigh')
        assert self.model.rowCount() == 10

    def test_updateFilter_ack(self):
        self.model.updateFilter(filterType=ACKNOWLEDGE, text='acknowledge')
        assert self.model.rowCount() == 10

    def test_data_bad_index(self):
        assert self.model.data(QModelIndex(), Qt.DisplayRole) is None

    def test_data_id(self):
        idx = self.model.createIndex(0, 0)
        cell = self.model.data(idx, Qt.DisplayRole)
        assert cell == 0
        idx = self.model.createIndex(1, 0)
        cell = self.model.data(idx, Qt.DisplayRole)
        assert cell == 1

    def test_data_alarmtype(self):
        idx = self.model.createIndex(0, 5)
        cell = self.model.data(idx, Qt.DisplayRole)
        assert cell == 'alarmHigh'

    def test_data_badrole(self):
        idx = self.model.createIndex(0, 0)
        cell = self.model.data(idx, Qt.CheckStateRole)
        assert cell is None
