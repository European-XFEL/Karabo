from qtpy.QtCore import QModelIndex, Qt

from karabogui.alarms.alarm_model import AlarmModel
from karabogui.alarms.const import (
    ADD_UPDATE_TYPE, ALARM_DATA, ALARM_HIGH, ALARM_WARNING_TYPES,
    INIT_UPDATE_TYPE, INTERLOCK, INTERLOCK_TYPES, REMOVE_UPDATE_TYPE,
    AlarmEntry)
from karabogui.alarms.filter_model import AlarmFilterModel
from karabogui.testing import GuiTestCase


def _data(update_type=INIT_UPDATE_TYPE):
    entries = []
    for i in range(10):
        data = AlarmEntry(
            id=i,
            timeOfFirstOccurrence='2017-04-20T09:32:22 UTC',
            timeOfOccurrence='2017-04-20T09:32:22 UTC',
            deviceId='dev{}'.format(i),
            property='rent',
            type=ALARM_HIGH,
            description='rent is too damn high',
            acknowledge=(True, True))
        entries.append(data)

    # we add an interlock
    data = AlarmEntry(
        id=200,
        timeOfFirstOccurrence='2017-04-20T09:32:22 UTC',
        timeOfOccurrence='2017-04-20T09:32:22 UTC',
        deviceId='dev{}'.format(i),
        property='rent',
        type=INTERLOCK,
        description='rent is too damn high',
        acknowledge=(True, True))
    entries.append(data)

    ret = {
        'instance_id': 'Karabo_AlarmService',
        'update_types': _type(update_type),
        'alarm_entries': entries
    }

    return ret


def _type(typ):
    return [typ for _ in range(10)]


class TestModel(GuiTestCase):
    def setUp(self):
        super(TestModel, self).setUp()
        self.alarm_model = AlarmModel()
        data = _data(INIT_UPDATE_TYPE)
        self.alarm_model.init_alarms_info(data)
        self.model = AlarmFilterModel(self.alarm_model)

    def tearDown(self):
        pass

    def test_headers(self):
        hdrs = [v for k, v in ALARM_DATA.items()]
        assert self.alarm_model.headers == hdrs

    def test_initAlarms(self):
        mdl = AlarmModel()
        data = _data([])
        mdl.init_alarms_info(data)
        # INIT Does not matter, we plainly initialize with all the info we have
        self.assertNotEqual(mdl.all_entries, [])

    def test_columnCount_filter_model(self):
        self.assertEqual(self.model.columnCount(), 8)

    def test_rowCount_filter_model(self):
        self.assertEqual(self.model.rowCount(), 10)

    def test_columnCount_alarm(self):
        self.assertEqual(self.alarm_model.columnCount(), 8)

    def test_rowCount_alarm(self):
        self.assertEqual(self.alarm_model.rowCount(), 11)

    def test_headerData(self):
        header = self.alarm_model.headerData(
            section=0,
            role=Qt.DisplayRole,
            orientation=Qt.Horizontal)
        self.assertEqual(header, 'ID')

    def test_update_alarms(self):
        mdl = AlarmModel()
        data = _data(INIT_UPDATE_TYPE)
        mdl.init_alarms_info(data)
        self.assertEqual(mdl.rowCount(), 11)

    def test_update_alarms_remove(self):
        self.assertEqual(self.alarm_model.rowCount(), 11)
        data = _data(REMOVE_UPDATE_TYPE)
        self.alarm_model.update_alarms_info(data)
        self.assertEqual(self.alarm_model.rowCount(), 1)

    def test_update_alarms_realarm(self):
        self.assertEqual(self.alarm_model.rowCount(), 11)

        data = _data(REMOVE_UPDATE_TYPE)
        self.alarm_model.update_alarms_info(data)
        self.assertEqual(self.alarm_model.rowCount(), 1)

        data = _data(ADD_UPDATE_TYPE)
        self.alarm_model.update_alarms_info(data)
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
