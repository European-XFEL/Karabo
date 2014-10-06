#############################################################################
# Author: martin.teichmann@xfel.eu>
# Created on April 8, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from karabo.hash import Hash
from karabo import hashtypes
from enums import AccessMode
from registry import Monkey
from network import Network
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
    signalUpdateComponent = pyqtSignal(object, object, object) # box, value, timestamp
    signalUserChanged = pyqtSignal(object, object) # box, value
    # the user changed the value, but it is not yet applied, so the value
    # in the box has not yet changed!
    signalHistoricData = pyqtSignal(object, object)

    def __init__(self, path, descriptor, configuration):
        QObject.__init__(self)
        # Path as tuple
        self.path = path
        self._descriptor = descriptor
        self.configuration = configuration
        self.timestamp = None
        self._value = Dummy()
        self.initialized = False
        self.current = None # Support for choice of nodes


    def key(self):
        return self.configuration.id + '.' + '.'.join(self.path)


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
        if d is not None:
            self._value = self.dummyCast()
            self.signalNewDescriptor.emit(self)


    def fillWidget(self, parameterEditor, isClass):
        if self._descriptor is not None:
            self._descriptor.fillWidget(parameterEditor, self, isClass)


    def __getattr__(self, attr):
        assert self.descriptor is not None, \
            "Box.{} needs descriptor to work".format(attr)
        return partial(getattr(self.descriptor, attr), self)


    def _set(self, value, timestamp):
        """ this is the internal method that sets the value and notifies
        listeners. The public set method is in the descriptors, so
        they can take care that the values actually make sense """
        self._value = self.descriptor.cast(value)
        self.initialized = True
        self.timestamp = timestamp
        self.signalUpdateComponent.emit(self, self._value, timestamp)


    def onUpdateValue(self, box, value, timestamp):
        """
        This slot updates not only the components but also the
        value of the box.
        """
        if self.descriptor is None:
            return
        
        if box.current is not None:
            value = box.current
        
        self.set(value, timestamp)


    def hasValue(self):
        return self.initialized


    def isAllowed(self):
        """return whether the value is allowed in the current state """

        return (self.descriptor is None or
                self.descriptor.allowedStates is None or
                self.configuration.value.state in
                    self.descriptor.allowedStates)


    def getPropertyHistory(self, t0, t1, maxNumData):
        Network().onGetPropertyHistory(self, t0, t1, maxNumData)


    def __str__(self):
        return "<{} {}>".format(type(self).__name__, self.key())


    @property
    def boxvalue(self):
        """ don't get the actual value of a box, but a proxy to get the
        sub-boxes of a value """
        r = _BoxValue()
        r.__dict__["__box__"] = self
        return r


class _BoxValue(object):
    def __getattr__(self, attr):
        try:
            return self.__box__.value.__dict__[attr]
        except KeyError:
            if isinstance(self.__box__.value, Dummy):
                r = Box(self.__box__.path + (unicode(attr),), None,
                        self.__box__.configuration)
                self.__box__.value.__dict__[attr] = r
                return r
            else:
                raise AttributeError(attr)


    def __setattr__(self, attr, value):
        self.value.__dict__[attr] = value


class Descriptor(hashtypes.Descriptor):
    # Means that parent class is overwritten/updated
    __metaclass__ = Monkey

    def completeItem(self, treeWidget, item, box, isClass):
        if self.assignment == 1: # Mandatory
            f = item.font(0)
            f.setBold(True)
            item.setFont(0, f)
        item.requiredAccessLevel = self.requiredAccessLevel
        item.displayText = self.displayedName
        item.allowedStates = self.allowedStates


    def __get__(self, instance, owner):
        return instance.__dict__[self.key].value


    def __set__(self, instance, value):
        instance.__dict__[self.key].set(value)


class Type(hashtypes.Type):
    # Means that parent class is overwritten/updated
    __metaclass__ = Monkey
    icon = icons.undefined


    def set(self, box, value, timestamp=None):
        box._set(value, timestamp)


    def dispatchUserChanges(self, box, hash):
        box.signalUserChanged.emit(box, box.descriptor.cast(hash))


    def setDefault(self, box):
        if self.defaultValue is not None:
            self.set(box, self.defaultValue)


    def item(self, treeWidget, parent, box, isClass):
        item = PropertyTreeWidgetItem(box, treeWidget, parent)

        item.setIcon(0, self.icon if self.options is None else icons.enum)
        item.enumeration = self.options
        item.classAlias = self.classAlias
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
        self.completeItem(treeWidget, item, box, isClass)
        return item


    def redummy(self, box):
        """ remove all values from box """
        box._value = Dummy()
        box.initialized = False
        box.descriptor = None


    def dummyCast(self, box):
        """dummy-aware casting of box

        if the box has a value, cast it to our type, if it has not,
        just leave it as is."""
        if box.hasValue():
            return self.cast(box.value)
        else:
            return box.value


class Char(hashtypes.Char):
    # Means that parent class is overwritten/updated
    __metaclass__ = Monkey

    classAlias = "Text Field"
    icon = icons.string


class String(hashtypes.String):
    # Means that parent class is overwritten/updated
    __metaclass__ = Monkey

    classAlias = "Text Field"
    icon = icons.string

    def item(self, treeWidget, parent, box, isClass):
        try:
            self.classAlias = dict(directory='Directory', fileIn='File In',
                                   fileOut='File Out')[self.displayType]
        except (AttributeError, KeyError):
            pass
        else:
            self.icon = icons.path
        item = super(String, self).item(treeWidget, parent, box, isClass)
        self.completeItem(treeWidget, item, box, isClass)
        return item


class Integer(hashtypes.Integer):
    # Means that parent class is overwritten/updated
    __metaclass__ = Monkey
    classAlias = 'Integer Field'
    icon = icons.int


class Number(hashtypes.Number):
    # Means that parent class is overwritten/updated
    __metaclass__ = Monkey
    classAlias = "Float Field"
    icon = icons.float


class Bool(hashtypes.Bool):
    # Means that parent class is overwritten/updated
    __metaclass__ = Monkey
    classAlias = "Toggle Field"
    icon = icons.boolean


class Vector(hashtypes.Vector):
    # Means that parent class is overwritten/updated
    __metaclass__ = Monkey
    classAlias = 'List'


class Object(object):
    def __init__(self, box):
        for k, v in type(self).__dict__.iteritems():
            if isinstance(v, hashtypes.Descriptor):
                b = getattr(box.boxvalue, k, None)
                if b is None:
                    b = Box(box.path + (k,), v, box.configuration)
                else:
                    b.descriptor = v
                self.__dict__[k] = b


class Dummy(object):
    """this class represents a not-yet-loaded value.
    it seems to contain all possible attributes, as we don't know yet
    which attributes might come...

    All the actual functionality is done in Box."""


class Schema(hashtypes.Descriptor):
    classAlias = "Value Field"

    def __init__(self, name='DUNNO'):
        self.dict = OrderedDict()
        self.cls = None
        self.name = name


    @classmethod
    def parse(cls, key, hash, attrs, parent=None):
        nodes = (Schema.parseLeaf, Schema.parse, ChoiceOfNodes.parse,
                 ListOfNodes.parse)
        self = dict(Image=Image, Slot=Slot).get(attrs.get('displayType', None),
                                                cls)(key)
        self.displayedName = key
        self.parseAttrs(self, attrs, parent)
        for k, h, a in hash.iterall():
            self.dict[k] = nodes[a['nodeType']](k, h, a, self)
        self.key = key
        return self


    @staticmethod
    def parseAttrs(self, attrs, parent):
        """parse the attributes from attrs. This should correspond to
        the C++ class util::Schema, where they are defined."""
        copy = ['description', 'defaultValue', 'displayType', 'assignment',
                'alias', 'allowedStates', 'tags', 'options', 'minInc', 'maxInc',
                'minExc', 'maxExc', 'minSize', 'maxSize', 'warnLow',
                'warnHigh', 'alarmLow', 'alarmHigh', 'archivePolicy']
        for a in copy:
            setattr(self, a, attrs.get(a))
        self.displayedName = attrs.get('displayedName', self.displayedName)
        self.accessMode = attrs.get('accessMode', 0)
        self.metricPrefixSymbol = attrs.get('metricPrefixSymbol', '')
        self.unitSymbol = attrs.get('unitSymbol', '')
        ral = 0 if parent is None else parent.requiredAccessLevel
        self.requiredAccessLevel = max(attrs.get('requiredAccessLevel', 0), ral)


    @staticmethod
    def parseLeaf(key, hash, attrs, parent):
        ret = Type.fromname[attrs['valueType']]()
        ret.displayedName = key
        ret.key = key
        Schema.parseAttrs(ret, attrs, parent)
        if ret.options is not None:
            ret.classAlias = "Selection Field"
        return ret


    def item(self, treeWidget, parent, box, isClass):
        item = PropertyTreeWidgetItem(box, treeWidget, parent)
        self.completeItem(treeWidget, item, box, isClass)
        return item


    def completeItem(self, treeWidget, item, box, isClass):
        self._item(treeWidget, item, box, isClass)
        super(Schema, self).completeItem(treeWidget, item, box, isClass)


    def _item(self, treeWidget, parent, box, isClass):
        for k, v in self.dict.iteritems():
            if isinstance(v, hashtypes.Descriptor):
                try:
                    c = getattr(box.boxvalue, k)
                except AttributeError:
                    print 'missing {} in {}'.format(k, box.value)
                else:
                    item = v.item(treeWidget, parent, c, isClass)


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


    def dummyCast(self, box):
        if box.hasValue():
            return self.cast(box.value)
        else:
            return self.getClass()(box)


    def toHash(self, box):
        ret = Hash()
        for k in self.dict:
            v = getattr(box.boxvalue, k, None)
            if v is None:
                continue
            if v.hasValue():
                ret[k] = v.toHash()
        return ret


    def fromHash(self, box, value, timestamp=None):
        for k, v, a in value.iterall():
            try:
                vv = getattr(box.boxvalue, k)
            except AttributeError:
                continue
            try:
                ts = Timestamp.fromHashAttributes(a)
            except KeyError:
                ts = None
            try:
                s = vv.fromHash
            except AttributeError:
                pass
                #print 'bullshit in', k, vv, vv.descriptor
            else:
                s(v, ts)
        box._set(box._value, timestamp)


    def dispatchUserChanges(self, box, hash):
        for k, v in hash.iteritems():
            getattr(box.boxvalue, k).dispatchUserChanges(v)


    def setDefault(self, box):
        box._value = self.getClass()(box)
        for k, v in self.dict.iteritems():
            getattr(box.boxvalue, k).setDefault()
        box._set(box._value, None)


    def redummy(self, box):
        d = Dummy()
        for k, v in self.dict.iteritems():
            b = getattr(box.boxvalue, k)
            if b is not None and b.descriptor is not None:
                b.redummy()
                setattr(d, k, b)
        box._value = d
        box.initialized = False
        box.descriptor = None


class Image(Schema):
    classAlias = "Image View"

    def item(self, treeWidget, parent, box, isClass):
        item = ImageTreeWidgetItem(box, treeWidget, parent)
        item.enabled = not isClass
        self.completeItem(treeWidget, item, box, isClass)


class Slot(Schema):
    classAlias = "Command"


    def execute(self, box):
        Network().onExecute(box)


    def item(self, treeWidget, parent, box, isClass):
        item = CommandTreeWidgetItem(self.key, box, treeWidget, parent)
        item.enabled = not isClass
        self.completeItem(treeWidget, item, box, isClass)


class ChoiceOfNodes(Schema):
    @classmethod
    def parse(cls, key, hash, attrs, parent=None):
        self = super(ChoiceOfNodes, cls).parse(key, hash, attrs, parent)
        self.classAlias = 'Choice Element'
        assert self.defaultValue is None or self.defaultValue in self.dict, \
            'the default value "{}" is not in {} for node {}'.format(
                self.defaultValue, hash.keys(), key)
        return self


    def item(self, treeWidget, parent, box, isClass):
        item = PropertyTreeWidgetItem(box, treeWidget, parent)
        item.defaultValue = self.defaultValue

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

        self.completeItem(treeWidget, item, box, isClass)

        for i in range(item.childCount()):
            child = item.child(i)

            if item.defaultValue is None:
                if i > 0:
                    child.setHidden(True)
            else:
                if child.text(0) != item.defaultValue:
                    child.setHidden(True)

            if item.editableComponent is not None:
                item.editableComponent.widgetFactory.addItem(child)

        # Trigger change of combobox
        item.editableComponent.widgetFactory.valueChanged(box, box.current)
        return item


    def fromHash(self, box, value, timestamp=None):
        for k in value:
            box.current = k
            break # there should be only one entry in the hash
        Schema.fromHash(self, box, value, timestamp)


    def dispatchUserChanges(self, box, hash):
        for k in hash:
            box.signalUserChanged.emit(box, k)
            break
        Schema.dispatchUserChanges(self, box, hash)


    def setDefault(self, box):
        if self.defaultValue is not None:
            box.current = self.defaultValue
        Schema.setDefault(self, box)


    def toHash(self, box):
        ret = Schema.toHash(self, box)
        if box.current is None:
            return Hash()
        else:
            return Hash(box.current, ret[box.current])


    def set(self, box, value, timestamp=None):
        print "#### CHOICE.set", box.key(), value, box.current
        box.current = value
        box._set(box.value, timestamp)


class ListOfNodes(hashtypes.Descriptor):
    @classmethod
    def parse(cls, key, hash, attrs, parent=None):
        return ListOfNodes()


    def setDefault(self, box):
        return


    def setAssignment(self, item):
        return


    def toHash(self, box):
        return [ ]


    def item(self, treeWidget, parent, box, isClass):
        item = PropertyTreeWidgetItem(box, treeWidget, parent)
        item.displayText = box.path[-1]
        item.requiredAccessLevel = 100


    def dummyCast(self, box):
        return box.value


    def redummy(self, box):
        box._value = Dummy()
        box.initialized = False
        box.descriptor = None
