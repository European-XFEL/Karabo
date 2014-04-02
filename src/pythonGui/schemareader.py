from hash import Schema, Hash
import hashtypes
from enums import AccessMode, NavigationItemTypes
from registry import Monkey
import icons
from timestamp import Timestamp
from util import Weak

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
from functools import partial


class Box(QObject):
    """This class represents one value of a device or a device class.
    It has signals that are emitted whenever the value changes.

    Those signals keep the GUI consistent: all widgets connect to them
    and are updated.

    Note that the network is *not* connected to those signals, as this
    would end up in an endless loop as every change coming from the
    network is returned to it. """

    signalNewDescriptor = pyqtSignal(object)
    signalUpdateComponent = pyqtSignal(object, object, object) # internalKey, value, timestamp
    signalHistoricData = pyqtSignal(object, object)

    def __init__(self, path, descriptor, configuration):
        QObject.__init__(self)
        self.path = path
        self._descriptor = descriptor
        self.configuration = configuration
        self.timestamp = None
        self._value = Dummy(path, configuration)
        self.current = None # Support for choice of nodes


    def key(self):
        return self.configuration.path + '.' + '.'.join(self.path)


    configuration = Weak()


    @property
    def value(self):
        return self._value


    @property
    def descriptor(self):
        return self._descriptor


    @descriptor.setter
    def descriptor(self, d):
        self._descriptor = d
        if self.hasValue() and isinstance(d, Schema):
            self._value = d.getClass()(self)
        self.signalNewDescriptor.emit(self)


    def __getattr__(self, attr):
        return partial(getattr(self.descriptor, attr), self)


    def _set(self, value, timestamp):
        self._value = value
        self.timestamp = timestamp
        self.signalUpdateComponent.emit(self, value, timestamp)


    def hasValue(self):
        return not isinstance(self._value, Dummy)


    def addWidget(self, widget):
        if widget.typeChanged is not None:
            self.signalNewDescriptor.connect(widget.typeChanged)
            if self.descriptor is not None:
                widget.typeChanged(self)
        if widget.valueChanged is not None:
            self.signalUpdateComponent.connect(widget.valueChanged)
            if self.hasValue():
                widget.valueChanged(self, self.value, self.timestamp)


    def addComponent(self, component):
        self.signalUpdateComponent.connect(component.onDisplayValueChanged)
        if self.hasValue():
            component.onDisplayValueChanged(self, self.value)


def _copyAttr(self, item, out, ain=None):
    if ain is None:
        ain = out
    if hasattr(self, ain):
        setattr(item, out, getattr(self, ain))


class Type(hashtypes.Type):
    __metaclass__ = Monkey
    icon = icons.undefined


    def set(self, box, value, timestamp=None):
        box._set(value, timestamp)


    def setDefault(self, box):
        if hasattr(self, "defaultValue"):
            self.set(box, self.defaultValue)


    def item(self, treeWidget, parent, box, isClass):
        item = PropertyTreeWidgetItem(box, treeWidget, parent)
        try:
            item.displayText = self.displayedName
        except AttributeError:
            item.displayText = box.path[-1]
        _copyAttr(self, item, 'allowedStates')

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
        if isClass:
            if self.accessMode & (
                    AccessMode.INITONLY | AccessMode.RECONFIGURABLE):
                component = EditableNoApplyComponent
        else:
            if self.accessMode & AccessMode.RECONFIGURABLE:
                component = EditableApplyLaterComponent
        if component is not None:
            item.editableComponent = component(item.classAlias, box, treeWidget)
        if component is EditableApplyLaterComponent:
            item.editableComponent.signalApplyChanged.connect(
                treeWidget.onApplyChanged)
        return item


class Char(hashtypes.Char):
    __metaclass__ = Monkey

    classAlias = "Text Field"
    icon = icons.string


class String(hashtypes.String):
    __metaclass__ = Monkey

    classAlias = "Text Field"
    icon = icons.string

    @classmethod
    def populateItem(cls, item, attrs, classtype, treewidget):
        super(String, cls).populateItem(item, attrs, classtype, treewidget)

        try:
            ca = dict(directory='Directory', fileIn='File In',
                      fileOut='File Out')[self.displayType]
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


class Object(object):
    def __init__(self, box):
        for k, v in type(self).__dict__.iteritems():
            if isinstance(v, hashtypes.Descriptor):
                b = getattr(box.value, k, None)
                if b is None:
                    b = Box(box.path + (k,), v, box.configuration)
                else:
                    b.descriptor = v
                self.__dict__[k] = b


    def __setattr__(self, key, value):
        getattr(self, key).set(value)


class Dummy(object):
    """this class represents a not-yet-loaded value.
    it seems to contain all possible attributes, as we don't know yet
    which attributes might come..."""
    def __init__(self, path, configuration):
        self._path = path
        self._configuration = configuration


    def __getattr__(self, attr):
        r = Box(self._path + (unicode(attr),), None, self._configuration)
        setattr(self, attr, r)
        return r


class Schema(hashtypes.Descriptor):
    def __init__(self, name='DUNNO'):
        self.dict = OrderedDict()
        self.cls = None
        self.name = name


    @classmethod
    def parse(cls, key, hash, attrs, parent=None):
        nodes = (Schema.parseLeaf, Schema.parse, ChoiceOfNodes.parse,
                 ListOfNodes.parse)
        self = cls(key)
        ral = 0 if parent is None else parent.requiredAccessLevel
        self.requiredAccessLevel = max(attrs.get('requiredAccessLevel', 0), ral)
        for k, h, a in hash.iterall():
            self.dict[k] = nodes[a['nodeType']](k, h, a, self)
        self.displayedName = key
        for k, v in attrs.iteritems():
            setattr(self, k, v)
        self.classAlias = dict(Image="Image View", Slot="Command").get(
            attrs.get('displayType', None), "Value Field")
        self.key = key
        return self


    @staticmethod
    def parseLeaf(key, hash, attrs, parent):
        ret = Type.fromname[attrs['valueType']]()
        ret.displayedName = key
        for k, v in attrs.iteritems():
            setattr(ret, k, v)
        return ret


    def item(self, treeWidget, parent, configuration, isClass):
        if self.displayType == "Image":
            item = ImageTreeWidgetItem(configuration, treeWidget, parent)

            item.enabled = not isClass
        elif self.displayType == "Slot":
            item = CommandTreeWidgetItem(self.key, configuration,
                                         treeWidget, parent)

            item.enabled = not isClass
        else:
            item = PropertyTreeWidgetItem(configuration, treeWidget, parent)
        item.displayText = self.displayedName
        _copyAttr(self, item, 'allowedStates')
        self._item(treeWidget, item, configuration, isClass)
        return item


    def _item(self, treeWidget, parent, box, isClass):
        for k, v in self.dict.iteritems():
            if isinstance(v, hashtypes.Descriptor):
                try:
                    c = getattr(box.value, k)
                except AttributeError:
                    print 'missing {} in {}'.format(k, box.value)
                else:
                    v.item(treeWidget, parent, c, isClass)


    def fillWidget(self, treeWidget, configuration, isClass):
        self._item(treeWidget, None, configuration, isClass)
        treeWidget.resizeColumnToContents(0)


    def getClass(self):
        if self.cls is None:
            self.cls = type(str(self.name), (Object,), self.dict)
        return self.cls


    def toHash(self, box):
        ret = Hash()
        for k, v in box.value.__dict__.iteritems():
            try:
                if v.hasValue():
                    ret[k] = v.toHash()
            except AttributeError as e:
                print 'tH', k, e
                pass
        return ret


    def fromHash(self, box, value, timestamp=None):
        if isinstance(box._value, Dummy):
            box._value = self.getClass()(box)
        for k, v, a in value.iterall():
            try:
                vv = getattr(box._value, k)
            except AttributeError:
                print 'schemaset: no {} in {} (to {})'.format(k, box._value, v)
                continue
            try:
                ts = Timestamp.fromHashAttributes(a)
            except KeyError:
                ts = None
            try:
                s = vv.fromHash
            except AttributeError:
                print 'bullshit in', k, vv, vv.descriptor
            else:
                s(v, ts)
        box._set(box._value, timestamp)


    def setDefault(self, box):
        box._set(self.getClass()(box), None)
        for k, v in self.dict.iteritems():
            getattr(box.value, k).setDefault()


class ChoiceBox(Box):
    def __init__(self, path, descriptor, configuration):
        NodeBox.__init__(self, path, descriptor, configuration)
        self.choices = { }
        for k, v in descriptor.choices.iteritems():
            box = SchemaBox(path + (k,), v, configuration)
            box.set(v.getClass()(path, configuration))
            self.choices[k] = box


    def set(self, value, timestamp=None):
        try:
            c = value in self.choices
        except TypeError:
            c = False
        if c:
            self._value = self.choices[value]
            return
        for k, v, a in value.iterall():
            self._value = self.choices[k]
            self._value.set(v, timestamp)
            return # there should be only one entry in the hash


class ChoiceOfNodes(Schema):
    @classmethod
    def parse(cls, key, hash, attrs, parent=None):
        self = super(ChoiceOfNodes, cls).parse(key, hash, attrs, parent)
        self.classAlias = 'Choice Element'
        print type(self)
        return self


    def item(self, treeWidget, parent, box, isClass):
        item = PropertyTreeWidgetItem(box, treeWidget, parent)
        try:
            item.displayText = self.displayedName
        except AttributeError:
            item.displayText = box.path[-1]
        _copyAttr(self, item, 'allowedStates')
        if self.assignment == 1: # Mandatory
            f = item.font(0)
            f.setBold(True)
            item.setFont(0, f)
        _copyAttr(self, item, 'defaultValue')

        item.isChoiceElement = True
        item.classAlias = "Choice Element"

        item.editableComponent = None
        component = None

        if isClass:
            if self.accessMode & (AccessMode.INITONLY |
                                  AccessMode.RECONFIGURABLE):
                component = EditableNoApplyComponent
        else:
            if False: #attrs['accessMode'] & AccessMode.RECONFIGURABLE:
                component = EditableApplyLaterComponent
            else:
                component = ChoiceComponent
        if component is not None:
            item.editableComponent = component(item.classAlias, box, treeWidget)
        if component is EditableApplyLaterComponent:
            item.editableComponent.signalApplyChanged.connect(
                self.treeWidget.onApplyChanged)

        defaultValue = item.defaultValue
        for k, v in self.dict.iteritems():
            childItem = v.item(treeWidget, item, getattr(box.value, k), isClass)

            if defaultValue is None:
                childItem.setHidden(True)
                defaultValue = False
            elif  k != defaultValue:
                childItem.setHidden(True)

            if item.editableComponent is not None:
                item.editableComponent.addParameters(itemToBeAdded=childItem)
        return item


    def fromHash(self, box, value, timestamp=None):
        for k in value:
            box.current = k
            break # there should be only one entry in the hash
        Schema.fromHash(self, box, value, timestamp)


    def setDefault(self, box):
        if hasattr(self, 'defaultValue'):
            box.current = self.defaultValue
        Schema.setDefault(self, box)


    def toHash(self, box):
        ret = Schema.toHash(self, box)
        return Hash(box.current, ret[box.current])


    def set(self, box, value, timestamp=None):
        box.current = value
        box._set(box.value, timestamp)


class ListOfNodes(hashtypes.Descriptor):
    @classmethod
    def parse(cls, key, hash, attrs, parent=None):
        return ListOfNodes()


    def setDefault(self, box):
        return


    def toHash(self, box):
        return [ ]


    def item(self, treeWidget, parent, box, isClass):
        item = PropertyTreeWidgetItem(box, treeWidget, parent)
        try:
            item.displayText = self.displayedName
        except AttributeError:
            item.displayText = box.path[-1]


class SchemaReader(object):
    def setDeviceType(self, deviceType):
        self.deviceType = deviceType


    def readSchema(self, schema):
        if schema is None:
            return
        ret = Schema.parse(schema.name, schema.hash, {})
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

        if self.deviceType == NavigationItemTypes.DEVICE:
            item.enabled = True

        item.displayText = attrs.get('displayedName', key.split('.')[-1])
        self.copyAttr(item, attrs, 'allowedStates')
        return item


    def _createImageItem(self, key, hash, attrs, parent):
        item = ImageTreeWidgetItem(key, self.treeWidget, parent)

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

                if item.classAlias == "Integer Field":
                    aitem.classAlias = "Integer Field"
                    aitem.setIcon(0, QIcon(":int-attribute"))
                else:
                    aitem.classAlias = "Float Field"
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
            if attrs['accessMode'] & (
                    AccessMode.INITONLY | AccessMode.RECONFIGURABLE):
                component = EditableNoApplyComponent
        else:
            if False: #attrs['accessMode'] & AccessMode.RECONFIGURABLE:
                component = EditableApplyLaterComponent
            else:
                component = ChoiceComponent
        if component is not None:
            item.editableComponent = component(
                item.classAlias, key=item.internalKey, value=item.defaultValue)
        if component is EditableApplyLaterComponent:
            item.editableComponent.signalApplyChanged.connect(
                self.treeWidget.onApplyChanged)

        for i, k in enumerate(hash):
            childItem = self.parse(key + "." + k, hash[k], hash[k, ...], item)

            if item.defaultValue is not None:
                if k != item.defaultValue:
                    childItem.setHidden(True)
            else:
                if i > 0:
                    childItem.setHidden(True)

            if item.editableComponent is not None:
                item.editableComponent.addParameters(itemToBeAdded=childItem)
        # Set default value in choice combobox
        item.onSetToDefault()
        
        return item


    def parseListOfNodes(self, key, hash, attrs, parent):
        item = self._createPropertyItem(key, hash, attrs, parent)
        for k in hash:
            self.parse(key + "." + k, hash[k], hash[k, ...], item)
