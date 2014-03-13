from hash import Schema
import hashtypes
from enums import AccessMode, NavigationItemTypes
from registry import Monkey
import icons

from choicecomponent import ChoiceComponent
from editableapplylatercomponent import EditableApplyLaterComponent
from editablenoapplycomponent import EditableNoApplyComponent

from treewidgetitems.attributetreewidgetitem import AttributeTreeWidgetItem
from treewidgetitems.commandtreewidgetitem import CommandTreeWidgetItem
from treewidgetitems.imagetreewidgetitem import ImageTreeWidgetItem
from treewidgetitems.propertytreewidgetitem import PropertyTreeWidgetItem

from PyQt4.QtCore import QObject, pyqtSignal
from PyQt4.QtGui import QIcon

from collections import OrderedDict
import weakref


class Type(hashtypes.Type):
    __metaclass__ = Monkey


    def copyAttr(self, item, out, ain=None):
        if ain is None:
            ain = out
        if hasattr(self, ain):
            setattr(item, out, getattr(self, ain))


    def item(self, treeWidget, parent, configuration):
        item = PropertyTreeWidgetItem(configuration.__dict__[self],
                                      treeWidget, parent)
        try:
            item.displayText = self.displayedName
        except AttributeError:
            item.displayText = key.split('.')[-1]
        self.copyAttr(item, 'allowedStates')

        try:
            item.enumeration = self.options
            item.classAlias = "Selection Field"
            item.setIcon(0, icons.enum)
        except AttributeError:
            item.enumeration = None
            item.classAlias = self.classAlias
            item.icon = self.icon

        component = None
        item.editableComponent = None
        if isinstance(configuration, Class):
            if self.accessMode in (AccessMode.INITONLY,
                                       AccessMode.RECONFIGURABLE):
                component = EditableNoApplyComponent
        else:
            if self.accessMode == AccessMode.RECONFIGURABLE:
                component = EditableApplyLaterComponent
        if component is not None:
            item.editableComponent = component(
                classAlias=item.classAlias, key=item.internalKey,
                value=item.defaultValue,
                metricPrefixSymbol=item.metricPrefixSymbol,
                enumeration=item.enumeration,
                unitSymbol=item.unitSymbol, valueType=item.valueType)
        if component is EditableApplyLaterComponent:
            item.editableComponent.signalApplyChanged.connect(
                treeWidget.onApplyChanged)
        return item


class String(hashtypes.String):
    __metaclass__ = Monkey

    classAlias = "Text Field"
    icon = icons.string

    @classmethod
    def populateItem(cls, item, attrs, classtype, treewidget):
        super(String, cls).populateItem(item, attrs, classtype, treewidget)

        try:
            ca = dict(directory='Directory', fileIn='File In',
                      fileOut='fileOut')[self.displayType]
            item.classAlias = ca
            item.setIcon(0, icons.path)
        except AttributeError:
            pass


class Integer(hashtypes.Integer):
    __metaclass__ = Monkey
    classAlias = 'Integer Field'
    icon = icons.int


class Number(hashtypes.Number):
    __metaclass__ = Monkey
    classAlias = "Float Field"
    icon = icons.float


class Bool(hashtypes.Bool):
    __metaclass__ = Monkey
    classAlias = "Toggle Field"
    icon = icons.boolean

    @classmethod
    def populateItem(self, item, attrs, classtype, treewidget):
        item.setIcon(0, QIcon(":boolean"))
        super(Bool, self).populateItem(item, attrs, classtype, treewidget)


class Vector(hashtypes.Vector):
    __metaclass__ = Monkey
    classAlias = 'Histogram'


class Value(QObject):
    signalUpdateComponent = pyqtSignal(object, object, object) # internalKey, value, timestamp
    signalUpdateDisplayValue = pyqtSignal(object, object, object)
    signalHistoricData = pyqtSignal(object, object)

    def __init__(self, instance):
        QObject.__init__(self)
        self._instance = weakref.ref(instance)


    @property
    def instance(self):
        return self._instance()


    @property
    def value(self):
        return self._value


    def set(self, value, timestamp):
        self._value = value
        self.timestamp = timestamp
        self.signalUpdateDisplayValue.emit(self, value, timestamp)
        self.signalUpdateComponent.emit(self, value, timestamp)


    @value.setter
    def value(self, value):
        self._value = value
        self.signalUpdateComponent.emit(self, value, None)
        self.signalUpdateDisplayValue.emit(self, value, None)


    def addComponent(self, component):
        self.signalUpdateComponent.connect(component.onValueChanged)
        self.signalUpdateDisplayValue.connect(component.onDisplayValueChanged)
        if hasattr(self, "_value"):
            self.signalUpdateComponent.emit(key, self.value, self.timestamp)
            self.signalUpdateDisplayValue.emit(key, self.value, self.timestamp)



class Object(object):
    def __init__(self):
        for v in type(self).__dict__.itervalues():
            if isinstance(v, hashtypes.Descriptor):
                self.__dict__[v] = Value(self)


class Device(Object):
    pass


class Class(Object):
    pass


class Schema(hashtypes.Descriptor):
    def __init__(self):
        self.dict = OrderedDict()
        self.classes = { }


    def __set__(self, instance, value):
        for k, v in value.iteritems():
            setattr(instance, k, v)


    @staticmethod
    def parse(key, hash, attrs, parent=None):
        nodes = (Schema.parseLeaf, Schema.parse, ChoiceOfNodes.parse,
                 Schema.parse)
        self = Schema()
        ral = 0 if parent is None else parent.requiredAccessLevel
        self.requiredAccessLevel = max(attrs.get('requiredAccessLevel', 0), ral)
        for k, h, a in hash.iterall():
            self.dict[k] = nodes[a['nodeType']](k, h, a, self)
        return self


    @staticmethod
    def parseLeaf(key, hash, attrs, parent):
        ret = Type.fromname[attrs['valueType']]()
        for k, v in attrs.iteritems():
            setattr(ret, k, v)
        return ret


    def item(self, treeWidget, parent, configuration):
        return
        if self.displayType == "Image":
            item = self._createImageItem(key, hash, attrs, parent)
        elif self.displayType == "Slot":
            return self._createCommandItem(key, hash, attrs, parent)
        else:
            item = self._createPropertyItem(key, hash, attrs, parent)
        self._item(key, treeWidget, parent)


    def _item(self, treeWidget, parent, configuration):
        for k, v in self.dict.iteritems():
            if isinstance(v, hashtypes.Descriptor):
                v.item(treeWidget, parent, configuration)


    def fillWidget(self, treeWidget, configuration):
        self._item(treeWidget, None, configuration)
        treeWidget.resizeColumnToContents(0)


    def getClass(self, base):
        if base not in self.classes:
            self.classes[base] = type(str(self.name), (base,), self.dict)
        return self.classes[base]


class ChoiceOfNodes(hashtypes.Descriptor):
    @staticmethod
    def parse(key, hash, attrs, parent=None):
        return None


class SchemaReader(object):
    def setDeviceType(self, deviceType):
        self.deviceType = deviceType


    def readSchema(self, schema):
        if schema is None:
            return
        ret = Schema.parse('', schema.hash, {})
        ret.name = schema.name
        return ret


    def parse(self, key, hash, attrs, parent=None):
        nodes = (self.parseLeaf, self.parseNode, self.parseChoiceOfNodes,
                 self.parseListOfNodes)
        item = nodes[attrs['nodeType']](key, hash, attrs, parent)
        ral = 0 if parent is None else parent.requiredAccessLevel
        item.requiredAccessLevel = max(attrs.get('requiredAccessLevel', 0), ral)
        return item


    def copyAttr(self, item, attrs, out, ain=None):
        if ain is None:
            ain = out
        if ain in attrs:
            setattr(item, out, attrs[ain])


    def _createPropertyItem(self, key, hash, attrs, parent):
        item = PropertyTreeWidgetItem(key, self.treeWidget, parent)
        item.displayText = attrs.get('displayedName', key.split('.')[-1])
        self.copyAttr(item, attrs, 'allowedStates')
        return item


    def _createCommandItem(self, key, hash, attrs, parent):
        item = CommandTreeWidgetItem(key.split('.')[-1], key,
                                     self.treeWidget, parent)
        item.classAlias = "Command"

        if self.deviceType == NavigationItemTypes.DEVICE:
            item.enabled = True

        item.displayText = attrs.get('displayedName', key.split('.')[-1])
        self.copyAttr(item, attrs, 'allowedStates')
        return item


    def _createImageItem(self, key, hash, attrs, parent):
        item = ImageTreeWidgetItem(key, self.treeWidget, parent)
        item.classAlias = "Image View"

        if self.deviceType == NavigationItemTypes.DEVICE:
            item.enabled = True

        self.copyAttr(item, attrs, 'displayText', 'displayedName')
        self.copyAttr(item, attrs, 'allowedStates')
        return item


    def parseLeaf(self, key, hash, attrs, parent):
        item = self._createPropertyItem(key, hash, attrs, parent)
        self._setAssignment(item, attrs)
        self.copyAttr(item, attrs, 'alias')
        self.copyAttr(item, attrs, 'tags')
        self.copyAttr(item, attrs, 'description')
        self.copyAttr(item, attrs, 'defaultValue')
        self.copyAttr(item, attrs, 'metricPrefixSymbol')
        self.copyAttr(item, attrs, 'unitSymbol')
        item.valueType = Type.fromname[attrs['valueType']]
        item.valueType.populateItem(
            item, attrs, self.deviceType == NavigationItemTypes.CLASS,
            self.treeWidget)
        keys = dict(warnLow='Warn Low', warnHigh='Warn High',
                    alarmLow='Alarm Low', alarmHigh='Alarm High')
        if self.deviceType == NavigationItemTypes.CLASS:
            accessMode = AccessMode.INITONLY
            keys.update(minInc='Minimum inclusive', maxInc='Maximum inclusive',
                        minExc='Minimum exclusive', maxExc='Maximum exclusive')
        else:
            accessMode = AccessMode.RECONFIGURABLE
        for k, v in keys.iteritems():
            if k in attrs:
                fullPath = key + "@" + k
                value = attrs[k]

                aitem = AttributeTreeWidgetItem(fullPath, self.treeWidget, item)
                aitem.setText(0, v)

                aitem.classAlias = item.classAlias
                if item.classAlias == "Integer Field":
                    aitem.setIcon(0, QIcon(":int-attribute"))
                else:
                    aitem.setIcon(0, QIcon(":float-attribute"))

                editableComponent = None
                if self.deviceType == NavigationItemTypes.CLASS:
                    editableComponent = EditableNoApplyComponent(
                        classAlias=aitem.classAlias, key=aitem.internalKey,
                        value=value, valueType=aitem.valueType)
                else:
                    aitem.displayComponent.onValueChanged(key, value)

                    if accessMode == AccessMode.RECONFIGURABLE:
                        editableComponent = EditableApplyLaterComponent(
                            classAlias=aitem.classAlias, key=aitem.internalKey,
                            value=value, valueType=aitem.valueType)
                        editableComponent.signalApplyChanged.connect(
                            self.treeWidget.onApplyChanged)
                aitem.editableComponent = editableComponent
        return item


    def _setAssignment(self, item, attrs):
        if attrs.get('assignment', None) == 1: # Mandatory
            f = item.font(0)
            f.setBold(True)
            item.setFont(0, f)


    def parseNode(self, key, hash, attrs, parent):
        if attrs.get('displayType') == "Image":
            item = self._createImageItem(key, hash, attrs, parent)
        elif attrs.get('displayType') == "Slot":
            return self._createCommandItem(key, hash, attrs, parent)
        else:
            item = self._createPropertyItem(key, hash, attrs, parent)

        self.copyAttr(item, attrs, 'alias')
        self.copyAttr(item, attrs, 'tags')
        self.copyAttr(item, attrs, 'description')

        for k in hash:
            self.parse(key + '.' + k, hash[k], hash[k, ...], item)
        return item


    def parseChoiceOfNodes(self, key, hash, attrs, parent):
        item = self._createPropertyItem(key, hash, attrs, parent)
        self._setAssignment(item, attrs)
        self.copyAttr(item, attrs, 'defaultValue')

        item.isChoiceElement = True
        item.classAlias = "Choice Element"

        item.editableComponent = None
        component = None

        if self.deviceType == NavigationItemTypes.CLASS:
            if attrs['accessMode'] in (AccessMode.INITONLY,
                                       AccessMode.RECONFIGURABLE,
                                       AccessMode.UNDEFINED):
                component = EditableNoApplyComponent
        else:
            if attrs['accessMode'] == AccessMode.RECONFIGURABLE:
                component = EditableApplyLaterComponent
            else:
                component = ChoiceComponent
        if component is not None:
            item.editableComponent = component(
                item.classAlias, key=item.internalKey, value=item.defaultValue)
        if component is EditableApplyLaterComponent:
            item.editableComponent.signalApplyChanged.connect(
                treewidget.onApplyChanged)

        for i, k in enumerate(hash):
            childItem = self.parse(key + "." + k, hash[k], hash[k, ...], item)

            if item.defaultValue:
                if k != item.defaultValue:
                    childItem.setHidden(True)
            else:
                if i > 0:
                    childItem.setHidden(True)

            if item.editableComponent is not None:
                item.editableComponent.addParameters(itemToBeAdded=childItem)
        return item


    def parseListOfNodes(self, key, hash, attrs, parent):
        for k in hash:
            self.parse(key + "." + k, hash[k], hash[k, ...], parent)
