from unittest import TestCase, main
import gui
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
from karabo.enums import AccessLevel
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


network.network = Network()
net = network.network


class Tests(TestCase):
    directory = path.dirname(__file__)
    def setUp(self):
        sys.excepthook = self.excepthook
        self.app = gui.init([])
        self.excepttype = None

        r = XMLParser()
        with open(path.join(self.directory, "schema.xml"), "r") as fin:
            self.testschema = Schema_("testschema", r.read(fin.read()))
        with open(path.join(self.directory, "configuration.xml"), "r") as fin:
            self.testconfiguration = r.read(fin.read())["ParameterTest"]
        globals.GLOBAL_ACCESS_LEVEL = AccessLevel.OPERATOR

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
        Manager().handle_systemTopology(h)
        self.assertTrue(Manager().systemTopology.has("testserver"))
        self.assertTrue(Manager().systemTopology.has("testdevice"))
        self.assertTrue(Manager().systemTopology.has("incompatible"))
        self.assertFalse(Manager().systemTopology.has("something"))


    def findNode(self, cls, name):
        root = cls.parameterEditor.invisibleRootItem()
        for i in count():
            node = root.child(i)
            if node.box.path[0] == name:
                return node


    def schema(self):
        cls = manager.getClass("testserver", "testclass")
        Manager().handle_classSchema("testserver", "testclass", self.testschema)

        self.assertEqual(cls.type, "class")
        self.assertEqual(cls.value.int32, 1234)
        node = self.findNode(cls, "int32")
        self.assertTrue(node.font(0).bold())
        self.assertEqual(node.displayComponent.widget.text(), "0x4D2")
        self.assertEqual(node.editableComponent.widgetFactory.value, 1234)
        self.assertEqual(node.text(0), "32 bit integer")
        cls.dispatchUserChanges(dict(int32=3456))
        self.assertEqual(node.editableComponent.widgetFactory.value, 3456)

        node = self.findNode(cls, "int8")
        self.assertFalse(node.font(0).bold())
        cls.fromHash(Hash("int8", 42))
        self.assertEqual(node.displayComponent.widget.text(), "42")

        node = self.findNode(cls, "output")
        widget = node.editableComponent.widgetFactory.widget
        self.assertEqual(widget.currentText(), "BinaryFile")
        cls.dispatchUserChanges(dict(output=dict(
                        TextFile=dict(filename="abc"))))
        self.assertEqual(widget.currentText(), "TextFile")
        cls.dispatchUserChanges(dict(output=dict(
                        BinaryFile=dict(filename="abc"))))
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

        Manager().projectTopology.projectSaveAs("/tmp/test.krb")
        Manager().projectTopology.onCloseProject()
        Manager().projectTopology.projectOpen("/tmp/test.krb")


    def stop(self):
        Manager().handle_instanceGone("testdevice", "device")
        root = Manager().projectTopology.invisibleRootItem()
        devices = root.child(0).child(0)
        self.assertIcon(devices.child(0).icon(), icons.deviceOffline)
        Manager().handle_instanceGone("testdevice", "device")
        Manager().handle_instanceGone("incompatible", "device")
        Manager().handle_instanceGone("testserver", "server")
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
        Manager().handle_deviceSchema("testdevice", self.testschema)
        testdevice = Manager().deviceData["testdevice"]
        Manager().handle_deviceConfiguration("testdevice", self.testconfiguration)
        self.assertEqual(testdevice.value.targetSpeed, 0.5)
        Manager().signalSelectNewNavigationItem.emit("testdevice")

        self.getItem("Target Conveyor Speed").setSelected(True)
        self.assertEqual(len(testdevice.parameterEditor.selectedItems()), 1)
        mime = QMimeData()
        mime.setData("sourceType", "ParameterTreeWidget")
        de = DropEvent(QPoint(100, 100), Qt.CopyAction, mime, Qt.LeftButton,
                       Qt.NoModifier, QDropEvent.Drop)
#        self.assertEqual(testdevice.visible, 4)
        scene.dropEvent(de)
#        self.assertEqual(testdevice.visible, 6)

        self.assertEqual(TestWidget.instance.value, 0.5)
        testdevice.dispatchUserChanges(dict(targetSpeed=3.5))
        self.assertEqual(TestWidget.instance.value, 3.5)
        self.assertEqual(testdevice.value.targetSpeed, 0.5)
        component = TestWidget.instance.proxy.parent().component
        panel = gui.window.configurationPanel
        testdevice.dispatchUserChanges(dict(targetSpeed=0.5))
        self.assertIcon(component.acApply.icon(), icons.applyGrey)
        #self.assertFalse(panel.pbApplyAll.isEnabled())
        TestWidget.instance.value = 2.5
        TestWidget.instance.onEditingFinished(2.5)
        self.assertIcon(component.acApply.icon(), icons.apply)
        self.assertTrue(panel.pbApplyAll.isEnabled())
        Manager().handle_deviceConfiguration("testdevice", Hash("targetSpeed", 1.5))
        self.assertIcon(component.acApply.icon(), icons.applyConflict)
        self.assertTrue(panel.pbApplyAll.isEnabled())
        self.assertEqual(testdevice.value.targetSpeed, 1.5)

        net.called = [ ]
        scene.clean()
        self.assertEqual(len(net.called), 2)
        self.assertEqual(net.called[0][0], "onStopMonitoringDevice")
        self.assertEqual(net.called[1][0], "onStopMonitoringDevice")
        self.assertEqual(testdevice.visible, 0)
        self.assertEqual(Manager().deviceData["incompatible"].visible, 0)


    def test_gui(self):
        self.systemTopology()
        self.schema()
        self.project()
        self.stop()
        self.systemTopology() # restart the stopped stuff
        self.scene()
        self.schema()
        self.assertException()


if __name__ == "__main__":
    main()
