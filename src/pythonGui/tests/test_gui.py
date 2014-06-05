from unittest import TestCase, main
import util # assure sip api is set first
from PyQt4.QtCore import QObject, QMimeData, QPoint, Qt, pyqtSignal
from PyQt4.QtGui import QDropEvent, QWidget
import icons
from manager import Manager
import manager
import network
import globals
import widget

from karabo.hash import Hash, XMLParser
from karabo.hashtypes import Schema_
from itertools import count

from os import path
import sys
import traceback

class Network(QObject):
    signalServerConnectionChanged = pyqtSignal(bool)
    signalUserChanged = pyqtSignal()
    called = [ ]

    def __getattr__(self, attr):
        def f(*args):
            self.called.append((attr, args))
        return f


class DropEvent(QDropEvent):
    def source(self):
        return Manager().deviceData["testdevice"].parameterEditor


class TestWidget(widget.EditableWidget, widget.DisplayWidget):
    category = "Digit"
    alias = "Test Widget"

    instance = None

    def __init__(self, box, parent):
        super(TestWidget, self).__init__(box)
        self.widget = QWidget(parent)
        TestWidget.instance = self


    def setReadOnly(self, ro):
        assert not ro, "The example TestWidget is read only"


    def valueChanged(self, box, value, timestamp=None):
        self.value = value


network.network = Network()
net = network.network
from gui import init


class Tests(TestCase):
    directory = path.dirname(__file__)
    def setUp(self):
        sys.excepthook = self.excepthook
        self.app = init([])
        self.excepttype = None

        r = XMLParser()
        with open(path.join(self.directory, "schema.xml"), "r") as fin:
            self.testschema = Schema_("testschema", r.read(fin.read()))
        with open(path.join(self.directory, "configuration.xml"), "r") as fin:
            self.testconfiguration = r.read(fin.read())["ParameterTest"]
        globals.GLOBAL_ACCESS_LEVEL=2

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
        Manager().handle_systemTopology(dict(systemTopology=h))
        self.assertTrue(Manager().systemTopology.has("testserver"))
        self.assertTrue(Manager().systemTopology.has("testdevice"))
        self.assertTrue(Manager().systemTopology.has("incompatible"))
        self.assertFalse(Manager().systemTopology.has("something"))


    def schema(self):
        h = Hash("serverId", "testserver", "classId", "testclass")
        h["schema"] = self.testschema
        cls = manager.getClass("testserver", "testclass")
        Manager().handle_classSchema(h)

        self.assertEqual(cls.type, "class")
        self.assertEqual(cls.value.int32.value, 1234)
        root = cls.parameterEditor.invisibleRootItem()
        for i in count():
            node = root.child(i)
            if node.box.path[0] == "int32":
                break
        self.assertEqual(node.displayComponent.widget.text(), "0x4D2")
        self.assertEqual(node.text(0), "32 bit integer")


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
        for f, arg in net.called:
            if f == function:
                return arg
        self.fail("network method {} not called".format(function))


    def project(self):
        net.called = [ ]
        Manager().projectTopology.projectOpen(path.join(self.directory,
                                              "project.krb"))
        self.assertCalled("onGetDeviceSchema")

        root = Manager().projectTopology.invisibleRootItem()
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

        self.assertEqual(Manager().deviceData["testdevice"].visible, 4)


    def stop(self):
        Manager().handle_instanceGone(dict(instanceId="testdevice"))
        root = Manager().projectTopology.invisibleRootItem()
        devices = root.child(0).child(0)
        self.assertIcon(devices.child(0).icon(), icons.deviceOffline)
        Manager().handle_instanceGone(dict(instanceId="testdevice"))
        Manager().handle_instanceGone(dict(instanceId="incompatible"))
        Manager().handle_instanceGone(dict(instanceId="testserver"))
        root = Manager().projectTopology.invisibleRootItem()
        devices = root.child(0).child(0)
        self.assertFalse(Manager().systemTopology.has("testserver"))

        self.assertIcon(devices.child(0).icon(), icons.deviceOfflineNoServer)
        self.assertIcon(devices.child(1).icon(), icons.deviceOfflineNoServer)
        self.assertIcon(devices.child(2).icon(), icons.deviceOfflineNoServer)
        self.assertIcon(devices.child(3).icon(), icons.deviceOfflineNoServer)


    def getItem(self, name):
        pe = Manager().deviceData["testdevice"].parameterEditor
        return pe.findItems(name, Qt.MatchExactly)[0]


    def scene(self):
        scene = Manager().projectTopology.projects[0].scenes[0]
        Manager().handle_deviceSchema(dict(deviceId="testdevice",
                                           schema=self.testschema))
        testdevice = Manager().deviceData["testdevice"]
        Manager().handle_configurationChanged(dict(
            deviceId="testdevice", configuration=self.testconfiguration))
        self.assertEqual(testdevice.value.targetSpeed.value, 0.5)
        Manager().signalSelectNewNavigationItem.emit("testdevice")

        self.getItem("Target Conveyor Speed").setSelected(True)
        self.assertEqual(len(testdevice.parameterEditor.selectedItems()), 1)
        mime = QMimeData()
        mime.setData("sourceType", "ParameterTreeWidget")
        de = DropEvent(QPoint(100, 100), Qt.CopyAction, mime, Qt.LeftButton,
                       Qt.NoModifier, QDropEvent.Drop)
        self.assertEqual(testdevice.visible, 4)
        scene.dropEvent(de)
        self.assertEqual(testdevice.visible, 6)

        Manager().handle_configurationChanged(dict(
            deviceId="testdevice",
            configuration=Hash("targetSpeed", 1.5)))
        self.assertEqual(testdevice.value.targetSpeed.value, 1.5)
        self.assertEqual(TestWidget.instance.value, 1.5)

        net.called = [ ]
        scene.clean()
        self.assertEqual(len(net.called), 2)
        self.assertEqual(net.called[0][0], "onRemoveVisibleDevice")
        self.assertEqual(net.called[1][0], "onRemoveVisibleDevice")
        self.assertEqual(testdevice.visible, 0)
        self.assertEqual(Manager().deviceData["incompatible"].visible, 0)


    def test_gui(self):
        self.systemTopology()
        self.schema()
        self.project()
        self.stop()
        self.systemTopology() # restart the stopped stuff
        self.scene()
        self.assertException()


if __name__ == "__main__":
    main()
