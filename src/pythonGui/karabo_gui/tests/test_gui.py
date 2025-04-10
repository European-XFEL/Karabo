from itertools import count
import os
import sys
import traceback
import unittest
import zipfile

# assure sip api is set first
import sip
sip.setapi("QString", 2)
sip.setapi("QVariant", 2)
sip.setapi("QUrl", 2)

from PyQt4.QtCore import QObject, QMimeData, QPoint, Qt, pyqtSignal
from PyQt4.QtGui import QApplication, QDropEvent, QWidget

from karabo.middlelayer import (
    AccessLevel, Hash, Integer, ProjectAccess, Schema, XMLParser
)
from karabo_gui.docktabwindow import Dockable
import karabo_gui.gui as gui
import karabo_gui.icons as icons
import karabo_gui.network as network
import karabo_gui.globals as globals
from karabo_gui.topology import getClass, Manager
import karabo_gui.widget as widget

net = None
TEST_DIR = os.path.dirname(__file__)
PROJ_DIR = os.path.join(TEST_DIR, 'project')
PROJECT_PATH = os.path.join(TEST_DIR, "project.krb")


def _create_project_file():
    def strip_leading(path):
        path = path[len(PROJ_DIR):]
        if path[0] == '/':
            path = path[1:]
        return path

    with zipfile.ZipFile(PROJECT_PATH, 'w') as zfp:
        for root, dirs, files in os.walk(PROJ_DIR):
            for dir in dirs:
                dirpath = os.path.join(root, dir)
                zippath = strip_leading(dirpath) + '/'
                zfp.write(dirpath, arcname=zippath,
                          compress_type=zipfile.ZIP_STORED)
            for filename in files:
                filepath = os.path.join(root, filename)
                zippath = strip_leading(filepath)
                zfp.write(filepath, arcname=zippath)


class DockableWidget(Dockable, QWidget):
    pass


class Network(QObject):
    signalServerConnectionChanged = pyqtSignal(bool)
    signalUserChanged = pyqtSignal()
    called = [ ]

    def __getattr__(self, attr):
        def f(*args, **kwargs):
            self.called.append((attr, args, kwargs))
        return f


class DropEvent(QDropEvent):
    def source(self):
        return Manager().deviceData["testdevice"].parameterEditor


class TestWidget(widget.EditableWidget, widget.DisplayWidget):
    category = Integer
    alias = "Test Widget"

    instance = None
    value = None

    def __init__(self, box, parent):
        super(TestWidget, self).__init__(box)
        self.widget = QWidget(parent)
        self.proxy = parent
        TestWidget.instance = self

    def setReadOnly(self, ro):
        assert not ro, "The example TestWidget is read only"

    def valueChanged(self, box, value, timestamp=None):
        self.value = value


class Tests(unittest.TestCase):
    def setUp(self):
        sys.excepthook = self.excepthook
        self.app = QApplication([])
        gui.init(self.app)
        self.excepttype = None

        r = XMLParser()
        with open(os.path.join(TEST_DIR, "schema.xml"), "r") as fin:
            self.testschema = Schema("testschema", hash=r.read(fin.read()))
        with open(os.path.join(TEST_DIR, "configuration.xml"), "r") as fin:
            self.testconfiguration = r.read(fin.read())["ParameterTest"]
        globals.GLOBAL_ACCESS_LEVEL = AccessLevel.OPERATOR

        network.network = Network()
        self.net = network.network
        _create_project_file()

    def tearDown(self):
        if os.path.exists(PROJECT_PATH):
            os.unlink(PROJECT_PATH)

    def excepthook(self, type, value, tb):
        traceback.print_exception(type, value, tb)
        self.excepttype = type
        self.exceptvalue = value
        self.traceback = tb

    def assertException(self):
        if self.excepttype is not None:
            raise self.exceptvalue

    def systemTopology(self):
        s = Hash("testserver", None)
        s["testserver", "deviceClasses"] = ["testclass"]
        s["testserver", "visibilities"] = [5]
        d = Hash("testdevice", None, "incompatible", None)
        d["testdevice", "serverId"] = "testserver"
        d["testdevice", "classId"] = "testclass"
        d["incompatible", "serverId"] = "testserver"
        d["incompatible", "classId"] = "testclass"
        h = Hash("device", d, "server", s)
        manager = Manager()
        manager.handle_systemTopology(h)
        self.assertTrue(manager.systemTopology.has("testserver"))
        self.assertTrue(manager.systemTopology.has("testdevice"))
        self.assertTrue(manager.systemTopology.has("incompatible"))
        self.assertFalse(manager.systemTopology.has("something"))

    def findNode(self, cls, name):
        root = cls.parameterEditor.invisibleRootItem()
        for i in count():
            node = root.child(i)
            if node.box.path[0] == name:
                return node

    def schema(self):
        cls = getClass("testserver", "testclass")
        manager = Manager()
        manager.handle_classSchema("testserver", "testclass", self.testschema)

        self.assertEqual(cls.type, "class")
        self.assertEqual(cls.value.int32, 1234)
        node = self.findNode(cls, "int32")
        self.assertTrue(node.font(0).bold())
        self.assertEqual(node.displayComponent.widget.text(), "0x4D2")
        self.assertEqual(node.editableComponent.widgetFactory.value, 1234)
        self.assertEqual(node.text(0), "32 bit integer")
        cls.dispatchUserChanges(Hash('int32', 3456))
        self.assertEqual(node.editableComponent.widgetFactory.value, 3456)

        node = self.findNode(cls, "int8")
        self.assertFalse(node.font(0).bold())
        cls.fromHash(Hash("int8", 42))
        self.assertEqual(node.displayComponent.widget.text(), "42")

        node = self.findNode(cls, "output")
        widget = node.editableComponent.widgetFactory.widget
        self.assertEqual(widget.currentText(), "BinaryFile")
        cls.dispatchUserChanges(Hash('output', Hash('TextFile', Hash('filename', "abc"))))
        self.assertEqual(widget.currentText(), "TextFile")
        cls.dispatchUserChanges(Hash('output', Hash('BinaryFile', Hash('filename', "abc"))))
        self.assertEqual(widget.count(), 4)
        self.assertEqual(widget.itemText(1), "Hdf5File")
        widget.setCurrentIndex(1)
        gui.window.navigationPanel.onSelectNewNavigationItem(
            "testserver.testclass")
        gui.window.configurationPanel.onInitDevice()
        widget.setCurrentIndex(0)

    def findIcon(self, a):
        for k in dir(icons):
            try:
                if a.cacheKey() == getattr(icons, k).cacheKey():
                    return k
            except AttributeError:
                pass
            except TypeError:
                pass
        return a

    def assertIcon(self, a, b):
        assert a.cacheKey() == b.cacheKey(), "{} is not {}".format(
                                        self.findIcon(a), self.findIcon(b))

    def assertCalled(self, function):
        for f, arg, kwargs in self.net.called:
            if f == function:
                return arg
        self.fail("network method {} not called".format(function))

    def project(self):
        self.net.called = [ ]
        manager = Manager()
        manager.projectTopology.projectOpen(PROJECT_PATH, ProjectAccess.LOCAL)
        self.assertCalled("onGetDeviceSchema")

        root = manager.projectTopology.invisibleRootItem()
        devices = root.child(0).child(0)
        self.assertEqual(devices.text(), "Devices")
        self.assertEqual(devices.child(0).text(), "testdevice")
        self.assertIcon(devices.child(0).icon(), icons.deviceInstance)
        self.assertEqual(devices.child(1).text(), "noserver")
        self.assertIcon(devices.child(1).icon(), icons.deviceOfflineNoServer)
        self.assertEqual(devices.child(2).text(), "noplugin")
        self.assertIcon(devices.child(2).icon(), icons.deviceOfflineNoPlugin)
        self.assertEqual(devices.child(3).text(), "incompatible")
        self.assertIcon(devices.child(3).icon(), icons.deviceIncompatible)
        self.assertEqual(devices.child(4).text(), "offline")
        self.assertIcon(devices.child(4).icon(), icons.deviceOffline)

        manager.projectTopology.projectSaveAs("/tmp/test.krb",
                                                ProjectAccess.LOCAL)
        manager.projectTopology.onCloseProject()
        manager.projectTopology.projectOpen("/tmp/test.krb",
                                              ProjectAccess.LOCAL)

    def stop(self):
        manager = Manager()
        manager.handle_instanceGone("testdevice", "device")
        root = manager.projectTopology.invisibleRootItem()
        devices = root.child(0).child(0)
        self.assertIcon(devices.child(0).icon(), icons.deviceOffline)
        manager.handle_instanceGone("testdevice", "device")
        manager.handle_instanceGone("incompatible", "device")
        manager.handle_instanceGone("testserver", "server")
        root = manager.projectTopology.invisibleRootItem()
        devices = root.child(0).child(0)
        self.assertFalse(manager.systemTopology.has("testserver"))

        self.assertIcon(devices.child(0).icon(), icons.deviceOfflineNoServer)
        self.assertIcon(devices.child(1).icon(), icons.deviceOfflineNoServer)
        self.assertIcon(devices.child(2).icon(), icons.deviceOfflineNoServer)
        self.assertIcon(devices.child(3).icon(), icons.deviceOfflineNoServer)

    def getItem(self, name):
        pe = Manager().deviceData["testdevice"].parameterEditor
        return pe.findItems(name, Qt.MatchExactly)[0]

    def scene(self):
        manager = Manager()
        scene = manager.projectTopology.projects[0].scenes[0]
        manager.handle_deviceSchema("testdevice", self.testschema)
        testdevice = manager.deviceData["testdevice"]
        manager.handle_deviceConfiguration("testdevice", self.testconfiguration)
        self.assertEqual(testdevice.value.targetSpeed, 0.5)
        manager.signalSelectNewNavigationItem.emit("testdevice")

        self.getItem("Target Conveyor Speed").setSelected(True)
        self.assertEqual(len(testdevice.parameterEditor.selectedItems()), 1)

        self.assertEqual(TestWidget.instance.value, 0.5)
        testdevice.dispatchUserChanges(Hash('targetSpeed', 3.5))
        self.assertEqual(TestWidget.instance.value, 3.5)
        self.assertEqual(testdevice.value.targetSpeed, 0.5)
        component = TestWidget.instance.proxy.parent().component
        panel = gui.window.configurationPanel
        testdevice.dispatchUserChanges(Hash('targetSpeed', 0.5))
        self.assertIcon(component.acApply.icon(), icons.applyGrey)
        #self.assertFalse(panel.pbApplyAll.isEnabled())
        TestWidget.instance.value = 2.5
        TestWidget.instance.onEditingFinished(2.5)
        self.assertIcon(component.acApply.icon(), icons.apply)
        self.assertTrue(panel.pbApplyAll.isEnabled())
        manager.handle_deviceConfiguration("testdevice", Hash("targetSpeed", 1.5))
        self.assertIcon(component.acApply.icon(), icons.applyConflict)
        self.assertTrue(panel.pbApplyAll.isEnabled())
        self.assertEqual(testdevice.value.targetSpeed, 1.5)

    def test_gui(self):
        self.systemTopology()
        self.schema()
        self.project()
        self.stop()
        self.systemTopology() # restart the stopped stuff
        self.scene()
        self.schema()
        self.assertException()

if __name__ == '__main__':
    unittest.main()
