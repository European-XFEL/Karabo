from karabo.hash import Schema, Hash
from karabo import hashtypes
from enums import AccessMode
from registry import Monkey
import manager
import icons
from timestamp import Timestamp
from util import Weak

from components import (ChoiceComponent, EditableApplyLaterComponent,
                        EditableNoApplyComponent)

from treewidgetitems.commandtreewidgetitem import CommandTreeWidgetItem
from treewidgetitems.imagetreewidgetitem import ImageTreeWidgetItem
from treewidgetitems.propertytreewidgetitem import PropertyTreeWidgetItem

from PyQt4.QtCore import QObject, pyqtSignal

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
        # Path as tuple
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
        if self.hasValue():
            self._value = d.cast(self._value)
        self.signalNewDescriptor.emit(self)


    def fillWidget(self, parameterEditor, isClass):
        if self._descriptor is not None:
            self._descriptor.fillWidget(parameterEditor, self, isClass)


    def __getattr__(self, attr):
        return partial(getattr(self.descriptor, attr), self)


    def _set(self, value, timestamp):
        self._value = self.descriptor.cast(value)
        self.timestamp = timestamp
        self.signalUpdateComponent.emit(self, self._value, timestamp)


    def hasValue(self):
        return not isinstance(self._value, Dummy)


    def isAllowed(self):
        """return whether the value is allowed in the current state """

        return (self.descriptor is None or
                self.descriptor.allowedStates is None or
                self.configuration.configuration.state.value in
                    self.descriptor.allowedStates)


    def getFromPast(self, t0, t1, maxNumData):
        manager.Manager().signalGetFromPast.emit(self, t0, t1, maxNumData)


class Type(hashtypes.Type):
    __metaclass__ = Monkey
    icon = icons.undefined


    def set(self, box, value, timestamp=None):
        box._set(value, timestamp)


    def setDefault(self, box):
        if self.defaultValue is not None:
            self.set(box, self.defaultValue)


    def item(self, treeWidget, parent, box, isClass):
        item = PropertyTreeWidgetItem(box, treeWidget, parent)
        try:
            item.displayText = self.displayedName
        except AttributeError:
            item.displayText = box.path[-1]
        item.allowedStates = self.allowedStates

        if self.options is not None:
            item.enumeration = self.options
            item.classAlias = "Selection Field"
            item.setIcon(0, icons.enum)
        else:
            item.enumeration = None
            item.classAlias = self.classAlias
            item.setIcon(0, self.icon)

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
        item.requiredAccessLevel = self.requiredAccessLevel
        return item


class Char(hashtypes.Char):
    __metaclass__ = Monkey

    classAlias = "Text Field"
    icon = icons.string


class String(hashtypes.String):
    __metaclass__ = Monkey

    classAlias = "Text Field"
    icon = icons.string

    def item(self, treeWidget, parent, box, isClass):
        item = super(String, self).item(treeWidget, parent, box, isClass)

        try:
            ca = dict(directory='Directory', fileIn='File In',
                      fileOut='File Out')[self.displayType]
            item.classAlias = ca
            item.setIcon(0, icons.path)
        except (AttributeError, KeyError):
            pass
        return item


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


class Vector(hashtypes.Vector):
    __metaclass__ = Monkey
    classAlias = 'Plot'


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
        self.displayedName = key
        self.parseAttrs(self, attrs, parent)
        for k, h, a in hash.iterall():
            self.dict[k] = nodes[a['nodeType']](k, h, a, self)
        self.classAlias = dict(Image="Image View", Slot="Command").get(
            attrs.get('displayType', None), "Value Field")
        self.key = key
        return self


    @staticmethod
    def parseAttrs(self, attrs, parent):
        """parse the attributes from attrs. This should correspond to
        the C++ class util::Schema, where they are defined."""
        copy = ['description', 'defaultValue', 'displayType', 'alias',
                'allowedStates', 'tags', 'options', 'minInc', 'maxInc',
                'minExc', 'maxExc', 'minSize', 'maxSize', 'warnLow',
                'warnHigh', 'alarmLow', 'alarmHigh', 'archivePolicy']
        for a in copy:
            setattr(self, a, attrs.get(a))
        self.displayedName = attrs.get('displayedName', self.displayedName)
        self.accessMode = attrs.get('accessMode', 0)
        self.metricPrefixSymbol = attrs.get('metrixPrefixSymbol', '')
        self.unitSymbol = attrs.get('unitSymbol', '')
        ral = 0 if parent is None else parent.requiredAccessLevel
        self.requiredAccessLevel = max(attrs.get('requiredAccessLevel', 0), ral)


    @staticmethod
    def parseLeaf(key, hash, attrs, parent):
        ret = Type.fromname[attrs['valueType']]()
        ret.displayedName = key
        Schema.parseAttrs(ret, attrs, parent)
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
        item.allowedStates = self.allowedStates
        item.requiredAccessLevel = self.requiredAccessLevel
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
        self._item(treeWidget, treeWidget.invisibleRootItem(), configuration, isClass)
        treeWidget.resizeColumnToContents(0)


    def getClass(self):
        if self.cls is None:
            self.cls = type(str(self.name), (Object,), self.dict)
        return self.cls


    def cast(self, other):
        if isinstance(other, self.getClass()):
            return other
        else:
            raise TypeError('cannot cast to {}, (was {})'.format(
                self.name, other))


    def toHash(self, box):
        ret = Hash()
        for k, v in box.value.__dict__.iteritems():
            try:
                if v.hasValue():
                    ret[k] = v.toHash()
            except AttributeError as e:
                pass
        return ret


    def fromHash(self, box, value, timestamp=None):
        if isinstance(box._value, Dummy):
            box._value = self.getClass()(box)
        for k, v, a in value.iterall():
            try:
                vv = getattr(box._value, k)
            except AttributeError:
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


class ChoiceOfNodes(Schema):
    @classmethod
    def parse(cls, key, hash, attrs, parent=None):
        self = super(ChoiceOfNodes, cls).parse(key, hash, attrs, parent)
        self.assignment = attrs['assignment']
        self.classAlias = 'Choice Element'
        return self


    def item(self, treeWidget, parent, box, isClass):
        item = PropertyTreeWidgetItem(box, treeWidget, parent)
        try:
            item.displayText = self.displayedName
        except AttributeError:
            item.displayText = box.path[-1]
        item.allowedStates = self.allowedStates
        if self.assignment == 1: # Mandatory
            f = item.font(0)
            f.setBold(True)
            item.setFont(0, f)
        item.defaultValue = self.defaultValue
        item.requiredAccessLevel = self.requiredAccessLevel

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
        if self.defaultValue is not None:
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
        item.requiredAccessLevel = 100
