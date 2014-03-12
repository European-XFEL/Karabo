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

from PyQt4.QtGui import QIcon

from collections import OrderedDict


class Type(hashtypes.Type):
    __metaclass__ = Monkey


    def copyAttr(self, item, out, ain=None):
        if ain is None:
            ain = out
        if hasattr(self, ain):
            setattr(item, out, getattr(self, ain))


    def item(self, key, treeWidget, parent, isClasstype):
        item = PropertyTreeWidgetItem(key, treeWidget, parent)
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
        if isClasstype:
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


class Schema(hashtypes.Descriptor):
    def __init__(self):
        object.__setattr__(self, "__dict__",  OrderedDict())

    def __getattr__(self, attr):
        return self.__dict__[attr]

    def __setattr__(self, attr, value):
        self.__dict__[attr] = value


    def __delattr__(self, attr):
        del self.__dict__[attr]


    @staticmethod
    def parse(key, hash, attrs, parent=None):
        nodes = (Schema.parseLeaf, Schema.parse, ChoiceOfNodes.parse,
                 Schema.parse)
        self = Schema()
        ral = 0 if parent is None else parent.requiredAccessLevel
        self.requiredAccessLevel = max(attrs.get('requiredAccessLevel', 0), ral)
        for k, h, a in hash.iterall():
            setattr(self, k, nodes[a['nodeType']](k, h, a, self))
        return self


    @staticmethod
    def parseLeaf(key, hash, attrs, parent):
        ret = Type.fromname[attrs['valueType']]()
        for k, v in attrs.iteritems():
            setattr(ret, k, v)
        return ret


    def item(self, key, treeWidget, parent, isClasstype):
        return
        if self.displayType == "Image":
            item = self._createImageItem(key, hash, attrs, parent)
        elif self.displayType == "Slot":
            return self._createCommandItem(key, hash, attrs, parent)
        else:
            item = self._createPropertyItem(key, hash, attrs, parent)
        self._item(key, treeWidget, parent)


    def _item(self, key, treeWidget, parent, isClasstype):
        for k, v in self.__dict__.iteritems():
            if isinstance(v, hashtypes.Descriptor):
                v.item(key + '.' + k, treeWidget, parent, isClasstype)


    def fillWidget(self, path, treeWidget, isClasstype):
        self._item(path, treeWidget, None, isClasstype)
        treeWidget.resizeColumnToContents(0)


class ChoiceOfNodes(hashtypes.Descriptor):
    @staticmethod
    def parse(key, hash, attrs, parent=None):
        return None


class SchemaReader(object):
    def setDeviceType(self, deviceType):
        self.deviceType = deviceType


    def readSchema(self, path, schema):
        if schema is None:
            return
        return Schema.parse('', schema.hash, {})


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
