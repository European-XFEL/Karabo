from PyQt4.QtCore import QModelIndex, Qt
from karabogui.testing import GuiTestCase

from ..const import (
    ADD_UPDATE_TYPE, AlarmEntry, ALARM_DATA, ALARM_HIGH,
    ALARM_WARNING_TYPES, INIT_UPDATE_TYPE, INTERLOCK, INTERLOCK_TYPES,
    REMOVE_UPDATE_TYPE)
from ..filter_model import AlarmFilterModel
from ..model import AlarmModel


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

    # we add an interlock
    data = AlarmEntry(
        id=200,
        timeOfFirstOccurrence='2017-04-20T09:32:22 UTC',
        timeOfOccurrence='2017-04-20T09:32:22 UTC',
        deviceId='dev{}'.format(i),
        property='rent',
        type=INTERLOCK,
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
        self.alarm_model = AlarmModel(instanceId='Bob')
        self.alarm_model.initAlarms('Bob', [], _data())
        self.model = AlarmFilterModel(self.alarm_model)

    def tearDown(self):
        pass

    def test_headers(self):
        hdrs = [v for k, v in ALARM_DATA.items()]
        assert self.alarm_model.headers == hdrs

    def test_initAlarms(self):
        mdl = AlarmModel(instanceId='Vinny')
        mdl.initAlarms('jenny', [], _data())
        self.assertEqual(mdl.all_entries, [])

    def test_columnCount_filter_model(self):
        self.assertEqual(self.model.columnCount(), 9)

    def test_rowCount_filter_model(self):
        self.assertEqual(self.model.rowCount(), 10)

    def test_columnCount_alarm(self):
        self.assertEqual(self.alarm_model.columnCount(), 9)

    def test_rowCount_alarm(self):
        self.assertEqual(self.alarm_model.rowCount(), 11)

    def test_headerData(self):
        header = self.alarm_model.headerData(
            section=0,
            role=Qt.DisplayRole,
            orientation=Qt.Horizontal)
        self.assertEqual(header, 'ID')

    def test_update_alarms(self):
        mdl = AlarmModel(instanceId='Vinny')
        mdl.initAlarms('Vinny', _type(INIT_UPDATE_TYPE), _data())
        self.assertEqual(mdl.rowCount(), 11)

    def test_update_alarms_wrongdev(self):
        mdl = AlarmModel(instanceId='Vinny')
        ret = mdl.initAlarms('jenny', _type(INIT_UPDATE_TYPE), _data())
        self.assertIsNone(ret)

    def test_update_alarms_remove(self):
        self.assertEqual(self.alarm_model.rowCount(), 11)
        self.alarm_model.updateAlarms('Bob', _type(REMOVE_UPDATE_TYPE),
                                      _data())
        self.assertEqual(self.alarm_model.rowCount(), 1)

    def test_update_alarms_realarm(self):
        self.assertEqual(self.alarm_model.rowCount(), 11)
        self.alarm_model.updateAlarms('Bob', _type(REMOVE_UPDATE_TYPE),
                                      _data())
        self.assertEqual(self.alarm_model.rowCount(), 1)
        self.alarm_model.updateAlarms('Bob', _type(ADD_UPDATE_TYPE), _data())
        self.assertEqual(self.alarm_model.rowCount(), 11)

    # ------------------------------------------------------------
    # Filter test

    def test_updateFilter_alarms(self):
        self.model.updateFilter(filter_type=ALARM_WARNING_TYPES)
        self.assertEqual(self.model.rowCount(), 10)

    def test_updateFilter_interlocks(self):
        self.model.updateFilter(filter_type=INTERLOCK_TYPES)
        self.assertEqual(self.model.rowCount(), 1)

    # ------------------------------------------------------------
    # Basic alarm model stress test

    def test_data_bad_index(self):
        self.assertIsNone(self.alarm_model.data(QModelIndex(), Qt.DisplayRole))

    def test_data_id(self):
        idx = self.alarm_model.createIndex(0, 0)
        cell = self.alarm_model.data(idx, Qt.DisplayRole)
        self.assertEqual(cell, 0)
        idx = self.alarm_model.createIndex(1, 0)
        cell = self.alarm_model.data(idx, Qt.DisplayRole)
        self.assertEqual(cell, 1)

    def test_data_alarmtype(self):
        idx = self.alarm_model.createIndex(0, 5)
        cell = self.alarm_model.data(idx, Qt.DisplayRole)
        self.assertEqual(cell, 'alarmHigh')

    def test_data_badrole(self):
        idx = self.alarm_model.createIndex(0, 0)
        cell = self.alarm_model.data(idx, Qt.CheckStateRole)
        self.assertIsNone(cell)
