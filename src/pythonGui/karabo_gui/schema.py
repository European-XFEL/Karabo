#############################################################################
# Author: martin.teichmann@xfel.eu>
# Created on April 8, 2014
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

"""
.. autoclass:: Box
"""

from collections import OrderedDict
from functools import partial

from PyQt4.QtCore import QObject, pyqtSignal, pyqtSlot

from karabo.middlelayer import (
    AccessMode, AccessLevel, Assignment, NodeType, Hash, Timestamp)
import karabo.middlelayer_api.hash as hashmod
from karabo_gui.attributeediting.api import (
    EDITABLE_ATTRIBUTE_NAMES, ATTRIBUTE_EDITOR_FACTORIES,
    EditAttributeComponent
)
from karabo_gui.components import (
    ChoiceComponent, EditableApplyLaterComponent, EditableNoApplyComponent)
import karabo_gui.globals as krb_globals
import karabo_gui.icons as icons
from karabo_gui.registry import Monkey
from karabo_gui.singletons.api import get_network
from karabo_gui.treewidgetitems import (
    CommandTreeWidgetItem, ImageTreeWidgetItem, PropertyTreeWidgetItem,
    TableTreeWidgetItem)
from karabo_gui.widget import EditableWidget

# MOST of the attribute names from Schemas that we care about.
# Schema.parseAttrs() contains a few others as well.
# This MUST correspond to the C++ class util::Schema, where they are defined.
SCHEMA_ATTRIBUTE_NAMES = (
    'description', 'defaultValue', 'displayType', 'assignment',
    'alias', 'allowedStates', 'tags', 'options', 'minInc', 'maxInc',
    'minExc', 'maxExc', 'minSize', 'maxSize', 'warnLow',
    'warnHigh', 'alarmLow', 'alarmHigh', 'archivePolicy',
    'relativeError', 'absoluteError', 'rowSchema'
)


class Box(QObject):
    """This class represents one value of a device or a device class.
    It has signals that are emitted whenever the value changes.

    Those signals keep the GUI consistent: all widgets connect to them
    and are updated.

    Note that the network is *not* connected to those signals, as this
    would end up in an endless loop as every change coming from the
    network is returned to it.
    """
    # signalNewDescriptor(box)
    signalNewDescriptor = pyqtSignal(object)
    # signalUpdateComponent(box, value, timestamp)
    signalUpdateComponent = pyqtSignal(object, object, object)
    # signalUserChanged(box, value, timestamp)
    signalUserChanged = pyqtSignal(object, object, object)
    # the user changed the value, but it is not yet applied, so the value
    # in the box has not yet changed!
    signalHistoricData = pyqtSignal(object, object)
    visibilityChanged = pyqtSignal(bool)

    def __init__(self, path, descriptor, parent):
        super().__init__(parent)
        # Path as tuple
        self.path = path
        if parent is not None:
            self.configuration = parent.configuration
        else:
            self.configuration = self
        self.timestamp = None
        self._value = Dummy()
        self.initialized = False
        self.descriptor = descriptor
        self.current = None  # Support for choice of nodes
        self.visible = 0

    def key(self):
        return self.configuration.id + '.' + '.'.join(self.path)

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
        if self.descriptor is None:
            msg = "Box.{} needs descriptor to work".format(attr)
            raise AttributeError(msg)
        return partial(getattr(self.descriptor, attr), self)

    def _set(self, value, timestamp):
        """This is the internal method that sets the value and notifies
        listeners. The public set method is in the descriptors, so they can
        take care that the values actually make sense
        """
        self._value = self.descriptor.cast(value)
        self.initialized = True
        self.update(timestamp)

    def update(self, timestamp=None):
        """Call this method if you changed the value of this box without
        setting it, like changing elements of a list.
        """
        self.timestamp = timestamp
        self.configuration.boxChanged(self, self._value, timestamp)

    @pyqtSlot(object, object)
    def slotSet(self, box, value):
        if box.current is not None:
            value = box.current
        self.set(value)

    def hasValue(self):
        return self.initialized

    def isAllowed(self):
        """Return whether the user may change the value, based on
        device's state
        """
        if not self.configuration.isOnline():
            return False
        return (self.descriptor is None or
                self.descriptor.allowedStates is None or
                self.configuration.value.state
                in self.descriptor.allowedStates)

    def isAccessible(self):
        """Return whether the user may change the value, based on
        the current access level
        """
        global_level = krb_globals.GLOBAL_ACCESS_LEVEL
        return (self.descriptor is None or
                global_level >= self.descriptor.requiredAccessLevel)

    def getPropertyHistory(self, t0, t1, maxNumData):
        get_network().onGetPropertyHistory(self, t0, t1, maxNumData)

    def __str__(self):
        return "<{} {}>".format(type(self).__name__, self.key())

    @property
    def boxvalue(self):
        """Don't get the actual value of a box, but a proxy to get the
        sub-boxes of a value
        """
        r = _BoxValue()
        r.__dict__["__box__"] = self
        return r

    def addVisible(self):
        self.visible += 1
        self.parent().addVisible()
        if self.visible == 1:
            # This needs to be done to subscribe for output channels
            self.visibilityChanged.emit(True)

    def removeVisible(self):
        self.visible -= 1
        self.parent().removeVisible()
        if self.visible == 0:
            # This needs to be done to unsubscribe from output channels
            self.visibilityChanged.emit(False)

    def unitLabel(self):
        """The unit strings are only available, if the descriptor is properly
        set, otherwise nothing is returned.
        """
        descr = self.descriptor
        if descr is not None:
            return descr.metricPrefixSymbol + descr.unitSymbol

    def axisLabel(self):
        """This function returns the axis label string.
        """
        unit = self.unitLabel()
        descr = self.descriptor
        name = descr.displayedName if descr is not None else ''
        return "{} [{}]".format(name, unit) if unit else name


class _BoxValue(object):
    def __getattr__(self, attr):
        try:
            return self.__box__.value.__dict__[attr]
        except KeyError:
            if isinstance(self.__box__.value, Dummy):
                r = Box(self.__box__.path + (str(attr),), None, self.__box__)
                self.__box__.value.__dict__[attr] = r
                return r
            else:
                raise AttributeError(attr)

    def __setattr__(self, attr, value):
        self.value.__dict__[attr] = value


class Descriptor(hashmod.Descriptor, metaclass=Monkey):
    # Means that parent class is overwritten/updated

    def completeItem(self, treeWidget, item, box, isClass):
        if self.assignment == Assignment.MANDATORY.value:
            f = item.font(0)
            f.setBold(True)
            item.setFont(0, f)
        item.requiredAccessLevel = self.requiredAccessLevel
        item.displayText = self.displayedName
        item.allowedStates = self.allowedStates

    def __get__(self, instance, owner):
        if instance is None:
            return self
        else:
            if self.key not in instance.__dict__:
                raise AttributeError
            return instance.__dict__[self.key].value

    def __set__(self, instance, value):
        if instance.__box__.configuration.type == "device":
            get_network().onReconfigure([(getattr(instance.__box__.boxvalue,
                                                  self.key), value)])
        else:
            instance.__dict__[self.key]._set(self.cast(value), None)


class Type(hashmod.Type, metaclass=Monkey):
    # Means that parent class is overwritten/updated
    icon = icons.undefined

    def set(self, box, value, timestamp=None):
        box._set(value, timestamp)

    def dispatchUserChanges(self, box, hash, attrs=None):
        self._copyAttrs(box, attrs)
        box.signalUserChanged.emit(box, box.descriptor.cast(hash), None)

    def setDefault(self, box):
        if self.defaultValue is not None:
            self.set(box, self.defaultValue)

    def item(self, treeWidget, parentItem, box, isClass):
        item = PropertyTreeWidgetItem(box, treeWidget, parentItem)

        item.setIcon(0, self.icon if self.options is None else icons.enum)
        item.enumeration = self.options
        component = None
        item.editableComponent = None
        if isClass:
            if self.accessMode in (AccessMode.INITONLY,
                                   AccessMode.RECONFIGURABLE):
                component = EditableNoApplyComponent
        else:
            if self.accessMode is AccessMode.RECONFIGURABLE:
                component = EditableApplyLaterComponent
        if component is not None:
            factory = EditableWidget.getClass(box)(box, treeWidget)
            item.editableComponent = component(factory, box, treeWidget)
        if component is EditableApplyLaterComponent:
            item.editableComponent.signalApplyChanged.connect(
                treeWidget.onApplyChanged)
        self.completeItem(treeWidget, item, box, isClass)
        return item

    def completeItem(self, treeWidget, item, box, isClass):
        """Add tree widget items for editable attributes.

        Editable attributes only show up BEFORE a class is instantiated.
        """
        super(Type, self).completeItem(treeWidget, item, box, isClass)

        # We're not interested unless this is an uninstantiated class.
        if not isClass:
            return

        desc = box.descriptor
        for name in EDITABLE_ATTRIBUTE_NAMES:
            if hasattr(desc, name):
                value = getattr(desc, name)
                # Skip empty values
                if value is None:
                    continue
                # Skip blank units
                if name == 'unitSymbol' and value == '':
                    continue
                # Skip metric prefixes with no associated units
                if (name == 'metricPrefixSymbol' and value == ''
                        and getattr(desc, 'unitSymbol') == ''):
                    continue
                self._attributeItem(treeWidget, item, box, name)

    def _attributeItem(self, treeWidget, parentItem, box, attributeName):
        """ Build a single tree widget item for an attribute.
        """
        item = PropertyTreeWidgetItem(box, treeWidget, parentItem)

        item.setIcon(0, self.icon if self.options is None else icons.enum)
        item.enumeration = self.options
        factory = ATTRIBUTE_EDITOR_FACTORIES[attributeName]
        item.editableComponent = EditAttributeComponent(
            factory, box, attributeName, treeWidget)

        item.requiredAccessLevel = self.requiredAccessLevel
        item.displayText = attributeName
        item.allowedStates = self.allowedStates

    def toHash(self, box):
        desc = box.descriptor
        attributes = {key: getattr(desc, key)
                      for key in EDITABLE_ATTRIBUTE_NAMES
                      if hasattr(desc, key) and getattr(desc, key) is not None}
        return box.value, attributes

    def fromHash(self, box, data, attrs=None, timestamp=None):
        self._copyAttrs(box, attrs)
        box._set(data, timestamp)

    def redummy(self, box):
        """Remove all values from the box
        """
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

    def _copyAttrs(self, box, attrs):
        attrs = attrs if attrs is not None else {}
        desc = box.descriptor
        for name, value in attrs.items():
            setattr(desc, name, value)


class Char(hashmod.Char, metaclass=Monkey):
    # Means that parent class is overwritten/updated
    icon = icons.string


class String(hashmod.String, metaclass=Monkey):
    # Means that parent class is overwritten/updated
    icon = icons.string

    def item(self, treeWidget, parentItem, box, isClass):
        if self.displayType in ("directory", "fileIn", "fileOut"):
            self.icon = icons.path
        item = super(String, self).item(treeWidget, parentItem, box, isClass)
        self.completeItem(treeWidget, item, box, isClass)
        return item


class Integer(hashmod.Integer, metaclass=Monkey):
    # Means that parent class is overwritten/updated
    icon = icons.int


class Number(hashmod.Number, metaclass=Monkey):
    # Means that parent class is overwritten/updated
    icon = icons.float


class Bool(hashmod.Bool, metaclass=Monkey):
    # Means that parent class is overwritten/updated
    icon = icons.boolean


class Vector(hashmod.Vector, metaclass=Monkey):
    pass


class Object(object):
    def __init__(self, box):
        self.__box__ = box
        sdict = type(self).__dict__
        if isinstance(box.value, Dummy):
            self.__dummies__ = {k: v for k, v in box.value.__dict__.items()
                                if isinstance(v, Box) and k not in sdict}
        else:
            self.__dummies__ = {}
        for k, v in sdict.items():
            if isinstance(v, hashmod.Descriptor):
                b = getattr(box.boxvalue, k, None)
                if b is None:
                    b = Box(box.path + (k,), v, box)
                else:
                    b.descriptor = v
                self.__dict__[k] = b

    def __enter__(self):
        self.__box__.addVisible()
        return self

    def __exit__(self, a, b, c):
        self.__box__.removeVisible()


class NetworkObject(Object, QObject):
    """An object that gets its data via a network output
    """
    def __init__(self, box):
        QObject.__init__(self)
        Object.__init__(self, box)
        box.visibilityChanged.connect(self.onVisibilityChanged)
        self.onVisibilityChanged(box.visible > 0)

    @pyqtSlot(bool)
    def onVisibilityChanged(self, visible):
        get_network().onSubscribeToOutput(self.__box__, visible)


class Dummy(object):
    """This class represents a not-yet-loaded value.
    It seems to contain all possible attributes, as we don't know yet which
    attributes might come...

    All the actual functionality is done in Box.
    """


class Schema(hashmod.Descriptor):
    def __init__(self, name='DUNNO'):
        self.dict = OrderedDict()
        self.cls = None
        self.name = name

    @classmethod
    def parse(cls, key, hash, attrs, parent=None):
        nodes = {
            NodeType.Leaf: Schema.parseLeaf,
            NodeType.Node: Schema.parse,
            NodeType.ChoiceOfNodes: ChoiceOfNodes.parse,
            NodeType.ListOfNodes: ListOfNodes.parse
        }
        displaytype_map = {
            'ImageData': ImageNode,
            'Image': ImageNode,
            'Slot': SlotNode,
            'OutputChannel': OutputNode,
            'Table': TableNode,
        }
        klass = displaytype_map.get(attrs.get('displayType', None), cls)
        self = klass(key)
        self.displayedName = key
        self.parseAttrs(self, attrs, parent)
        for k, h, a in hash.iterall():
            self.dict[k] = nodes[NodeType(a['nodeType'])](k, h, a, self)
        self.key = key
        return self

    @staticmethod
    def parseAttrs(self, attrs, parent):
        """Parse the attributes from attrs.
        """
        for a in SCHEMA_ATTRIBUTE_NAMES:
            setattr(self, a, attrs.get(a))
        self.displayedName = attrs.get('displayedName', self.displayedName)
        self.accessMode = AccessMode(attrs.get('accessMode',
                                               AccessMode.INITONLY))
        self.metricPrefixSymbol = attrs.get('metricPrefixSymbol', '')
        self.unitSymbol = attrs.get('unitSymbol', '')

        if parent is None:
            ral = AccessLevel.OBSERVER
        else:
            ral = parent.requiredAccessLevel
        self.requiredAccessLevel = max(AccessLevel(
            attrs.get('requiredAccessLevel', AccessLevel.OBSERVER)), ral)

    @staticmethod
    def parseLeaf(key, hash, attrs, parent):
        ret = Type.fromname[attrs['valueType']]()
        ret.displayedName = key
        ret.key = key
        Schema.parseAttrs(ret, attrs, parent)
        return ret

    def item(self, treeWidget, parentItem, box, isClass):
        item = PropertyTreeWidgetItem(box, treeWidget, parentItem)
        self.completeItem(treeWidget, item, box, isClass)
        return item

    def completeItem(self, treeWidget, item, box, isClass):
        self._item(treeWidget, item, box, isClass)
        super(Schema, self).completeItem(treeWidget, item, box, isClass)

    def _item(self, treeWidget, parentItem, box, isClass):
        for k, v in self.dict.items():
            if isinstance(v, hashmod.Descriptor):
                try:
                    c = getattr(box.boxvalue, k)
                except AttributeError:
                    print('missing {} in {}'.format(k, box.value))
                else:
                    v.item(treeWidget, parentItem, c, isClass)

    def fillWidget(self, treeWidget, configuration, isClass):
        self._item(treeWidget, treeWidget.invisibleRootItem(), configuration,
                   isClass)
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
                value, attrs = v.toHash()
                ret[k] = value
                ret[k, ...] = attrs
        return ret, {}

    def fromHash(self, box, value, attrs=None, timestamp=None):
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
                # print 'bullshit in', k, vv, vv.descriptor
            else:
                s(v, attrs=a, timestamp=ts)
        box._set(box._value, timestamp)

    def dispatchUserChanges(self, box, hash, attrs=None):
        for k, v, a in hash.iterall():
            getattr(box.boxvalue, k).dispatchUserChanges(v, attrs=a)

    def setDefault(self, box):
        box._value = self.getClass()(box)
        for k, v in self.dict.items():
            getattr(box.boxvalue, k).setDefault()
        box._set(box._value, None)

    def redummy(self, box):
        d = Dummy()
        for k, v in self.dict.items():
            b = getattr(box.boxvalue, k)
            if b is not None and b.descriptor is not None:
                b.redummy()
                setattr(d, k, b)
        for k, v in box.value.__dummies__.items():
            setattr(d, k, v)
        box._value = d
        box.initialized = False
        box.descriptor = None

    def _recurseGetDictPaths(self, parent_key, parent_value, path_list,
                             accessMode=None):
        """This is a private methods which goes recursively through the `dict`
        of a `Schema` and returns a list of either all paths or all
        read-only paths.
        """
        if isinstance(parent_value, Schema):
            for key, value in parent_value.dict.items():
                new_key = "{}.{}".format(parent_key, key)
                self._recurseGetDictPaths(
                    new_key, value, path_list, accessMode)
        if accessMode is None:
            return path_list.append(parent_key)
        elif parent_value.accessMode is accessMode:
            return path_list.append(parent_key)

    def getAllPaths(self):
        """This method returns a string list of all paths for this context.
        """
        all_paths = []
        for key, value in self.dict.items():
            self._recurseGetDictPaths(key, value, all_paths)
        return all_paths

    def getReadOnlyPaths(self):
        """This recursive method returns a string list of all paths with
        read-only access.
        """
        read_only_paths = []
        for key, value in self.dict.items():
            self._recurseGetDictPaths(key, value, read_only_paths,
                                      accessMode=AccessMode.READONLY)
        return read_only_paths

    def getObsoletePaths(self, config):
        """This recursive method checks whether the paths in the `config`
        hash still exist in this context.

        A string list of obsolete paths is returned.
        """
        all_paths = self.getAllPaths()

        def recurse(parent_key, parent_value, obsolete_paths):
            if isinstance(parent_value, Hash):
                for key, value in parent_value.items():
                    new_key = "{}.{}".format(parent_key, key)
                    recurse(new_key, value, obsolete_paths)

            if parent_key not in all_paths:
                return obsolete_paths.append(parent_key)

        obsolete_paths = []
        for key, value in config.items():
            recurse(key, value, obsolete_paths)
        return obsolete_paths


class ImageNode(Schema):
    def item(self, treeWidget, parentItem, box, isClass):
        item = ImageTreeWidgetItem(box, treeWidget, parentItem)
        item.enabled = not isClass
        self.completeItem(treeWidget, item, box, isClass)


class TableNode(Schema):
    def item(self, treeWidget, parentItem, box, isClass):
        item = TableTreeWidgetItem(box, treeWidget, parentItem)
        self.completeItem(treeWidget, item, box, isClass)


class OutputNode(Schema):
    def getClass(self):
        if self.cls is None:
            self.cls = type(str(self.name), (NetworkObject,), self.dict)
        return self.cls


class Slot(Object):
    def __init__(self, box):
        Object.__init__(self, box)
        self.box = box

    def __call__(self):
        self.box.execute()


class SlotNode(Schema):
    def execute(self, box):
        get_network().onExecute(box)

    def item(self, treeWidget, parentItem, box, isClass):
        item = CommandTreeWidgetItem(self.key, box, treeWidget, parentItem)
        item.enabled = not isClass
        self.completeItem(treeWidget, item, box, isClass)

    def getClass(self):
        if self.cls is None:
            self.cls = type(str(self.name), (Slot,), self.dict)
        return self.cls


class ChoiceOfNodes(Schema):
    @classmethod
    def parse(cls, key, hash, attrs, parent=None):
        self = super(ChoiceOfNodes, cls).parse(key, hash, attrs, parent)
        assert self.defaultValue is None or self.defaultValue in self.dict, \
            'the default value "{}" is not in {} for node {}'.format(
                self.defaultValue, list(hash.keys()), key)
        self.metricPrefixSymbol = ''
        self.unitSymbol = ''
        return self

    def item(self, treeWidget, parentItem, box, isClass):
        item = PropertyTreeWidgetItem(box, treeWidget, parentItem)
        item.defaultValue = self.defaultValue

        item.isChoiceElement = True
        item.editableComponent = None
        component = None

        if isClass:
            component = EditableNoApplyComponent
        else:
            component = ChoiceComponent

        factory = EditableWidget.getClass(box)(box, treeWidget)
        item.editableComponent = component(factory, box, treeWidget)
        self.completeItem(treeWidget, item, box, isClass)

        children = []
        for i in range(item.childCount()):
            child = item.child(i)
            childKey = child.box.path[-1]

            if item.defaultValue is None:
                if i > 0:
                    child.setHidden(True)
                else:
                    if box.current is None:
                        # Set current choice elements via key
                        box.current = childKey
            else:
                if childKey != item.defaultValue:
                    child.setHidden(True)

            if item.editableComponent is not None:
                children.append(child)
        item.editableComponent.widgetFactory.childItemList = children

        # Trigger change of combobox
        item.editableComponent.widgetFactory.valueChanged(box, box.current)
        return item

    def fromHash(self, box, value, attrs=None, timestamp=None):
        for k in value:
            box.current = k
            break  # there should be only one entry in the hash
        Schema.fromHash(self, box, value, attrs=attrs, timestamp=timestamp)

    def dispatchUserChanges(self, box, hash, attrs=None):
        for k in hash:
            box.signalUserChanged.emit(box, k, None)
            break
        Schema.dispatchUserChanges(self, box, hash, attrs=attrs)

    def setDefault(self, box):
        if self.defaultValue is not None:
            box.current = self.defaultValue
        Schema.setDefault(self, box)

    def toHash(self, box):
        ret, attrs = super(ChoiceOfNodes, self).toHash(box)
        if box.current is None:
            return Hash()
        else:
            key = box.current
            h = Hash(key, ret[key])
            h[key, ...] = attrs
            return h, {}

    def set(self, box, value, timestamp=None):
        """The value of this ChoiceElement is set.

        ``value`` is a string or a Schema with the selected choice
        """
        if isinstance(value, str):
            box.current = value
        else:
            box.current = value.current
        # Go on recursively
        box._set(box.value, timestamp)

    def slotSet(self, box, otherbox, value):
        """Value is another choice element to copy from
        """
        self.set(box, otherbox.current)


class ListOfNodes(hashmod.Descriptor):
    @classmethod
    def parse(cls, key, hash, attrs, parent=None):
        descr = ListOfNodes()
        descr.metricPrefixSymbol = ''
        descr.unitSymbol = ''
        return descr

    def setDefault(self, box):
        return

    def setAssignment(self, item):
        return

    def toHash(self, box):
        return [], {}

    def item(self, treeWidget, parentItem, box, isClass):
        item = PropertyTreeWidgetItem(box, treeWidget, parentItem)
        item.displayText = box.path[-1]
        item.requiredAccessLevel = AccessLevel.GOD

    def dummyCast(self, box):
        return box.value

    def redummy(self, box):
        box._value = Dummy()
        box.initialized = False
        box.descriptor = None


class VectorHash(hashmod.VectorHash, metaclass=Monkey):
    # Means that parent class is overwritten/updated

    def item(self, treeWidget, parentItem, box, isClass):
        item = TableTreeWidgetItem(box, treeWidget, parentItem)
        item.setIcon(0, self.icon if self.options is None else icons.enum)
        item.enumeration = self.options
        component = None
        item.editableComponent = None
        if isClass:
            if self.accessMode in (AccessMode.INITONLY,
                                   AccessMode.RECONFIGURABLE):
                component = EditableNoApplyComponent
        else:
            if self.accessMode is AccessMode.RECONFIGURABLE:
                component = EditableApplyLaterComponent

        if component is not None:
            factory = EditableWidget.getClass(box)(box, treeWidget)
            item.editableComponent = component(factory, box, treeWidget)
        if component is EditableApplyLaterComponent:
            item.editableComponent.signalApplyChanged.connect(
                treeWidget.onApplyChanged)
        self.completeItem(treeWidget, item, box, isClass)
        return item
