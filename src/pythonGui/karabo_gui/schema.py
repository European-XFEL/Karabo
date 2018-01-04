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
import numbers
import weakref

import numpy as np
from PyQt4.QtCore import QObject, pyqtSignal, pyqtSlot

from karabo.middlelayer import (
    AccessMode, AccessLevel, NodeType, Hash, Timestamp)
import karabo.middlelayer_api.hash as hashmod
import karabo_gui.globals as krb_globals
from karabo_gui.registry import Monkey
from karabo_gui.singletons.api import get_network

# MOST of the attribute names from Schemas that we care about.
# Schema.parseAttrs() contains a few others as well.
# This MUST correspond to the C++ class util::Schema, where they are defined.
SCHEMA_ATTRIBUTE_NAMES = (
    'description', 'defaultValue', 'displayType', 'assignment',
    'alias', 'allowedStates', 'tags', 'options', 'minInc', 'maxInc',
    'minExc', 'maxExc', 'minSize', 'maxSize', 'warnLow',
    'warnHigh', 'alarmLow', 'alarmHigh', 'archivePolicy',
    'relativeError', 'absoluteError', 'rowSchema', 'daqPolicy'
)
# The names of attributes which can be edited prior to class instantiation
EDITABLE_ATTRIBUTE_NAMES = (
    'minExc', 'maxExc', 'minInc', 'maxInc', 'absoluteError', 'relativeError',
    'warnLow', 'warnHigh', 'alarmLow', 'alarmHigh', 'metricPrefixSymbol',
    'unitSymbol', 'daqPolicy'
)


def box_has_changes(descriptor, old_value, new_value):
    """Check whether the given `old_value` differs from the given `new_value`
    """
    def _build_array_cmp(dtype):
        """Builds a comparison function for numpy arrays"""
        coerce = dtype.type
        if coerce is np.bool_:
            coerce = int

        def _array_cmp(a, b):
            try:
                return coerce(a) == coerce(b)
            except ValueError:
                return False

        return _array_cmp

    def _cmp(a, b):
        return a == b

    # Check if changes were made
    if old_value is None:
        has_changes = True
    elif (isinstance(old_value, (numbers.Complex, np.inexact))
            and not isinstance(old_value, numbers.Integral)):
        diff = abs(old_value - new_value)
        absErr = descriptor.absoluteError
        relErr = descriptor.relativeError
        if absErr is not None:
            has_changes = diff >= absErr
        elif relErr is not None:
            has_changes = diff >= abs(old_value * relErr)
        else:
            has_changes = diff >= 1e-4
    elif isinstance(old_value, (list, np.ndarray)):
        if len(old_value) != len(new_value):
            has_changes = True
        else:
            has_changes = False
            cmp = _cmp
            if isinstance(old_value, np.ndarray):
                cmp = _build_array_cmp(old_value.dtype)
            for i in range(len(old_value)):
                if not cmp(old_value[i], new_value[i]):
                    has_changes = True
                    break
    else:
        has_changes = (str(old_value) != str(new_value))
    return has_changes


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
        self.rowsInfo = []
        self.has_conflict = False

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
        """The unit strings are only available, if the attributeInfo is
        properly set, otherwise nothing is returned.
        """
        if not hasattr(self, 'attributeInfo'):
            return ''

        attrs = self.attributeInfo.attrs
        prefix_symbol = attrs.get('metricPrefixSymbol', '')
        return prefix_symbol + attrs.get('unitSymbol', '')

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

    def set(self, box, value, timestamp=None):
        if not hasattr(box, 'attributeInfo'):
            box.attributeInfo = EditableAttributeInfo(box, box.descriptor)
        box._set(value, timestamp)

    def dispatchUserChanges(self, box, value, attrs=None):
        attrs = {} if attrs is None else attrs
        box.attributeInfo.update(attrs)
        value = box.descriptor.cast(value)
        if box_has_changes(box.descriptor, box.value, value):
            box.configuration.setUserValue(box, value)
        box.signalUserChanged.emit(box, value, None)

    def setDefault(self, box):
        if self.defaultValue is not None:
            self.set(box, self.defaultValue)
        else:
            if not hasattr(box, 'attributeInfo'):
                box.attributeInfo = EditableAttributeInfo(box, box.descriptor)

    def toHash(self, box):
        attr_info = getattr(box, 'attributeInfo', None)
        attrs = {} if attr_info is None else dict(attr_info.attrs)
        return box.value, attrs

    def fromHash(self, box, data, attrs=None, timestamp=None):
        if not hasattr(box, 'attributeInfo'):
            box.attributeInfo = EditableAttributeInfo(box, box.descriptor,
                                                      attrs)
        else:
            box.attributeInfo.update(attrs)
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
                try:
                    s(v, attrs=a, timestamp=ts)
                except (AttributeError, ValueError, TypeError):
                    print('Ignoring bad configuration value for', vv.key())
        box._set(box._value, timestamp)

    def dispatchUserChanges(self, box, value, attrs=None):
        for k, v, a in value.iterall():
            getattr(box.boxvalue, k).dispatchUserChanges(v, attrs=a)
        box.signalUserChanged.emit(box, value, None)

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
    # Subclassed to simplify isinstance() checks later
    pass


class TableNode(Schema):
    # Subclassed to simplify isinstance() checks later
    pass


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
        device_id = box.configuration.id
        slot_name = '.'.join(box.path)
        get_network().onExecute(device_id, slot_name)

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

    def cast(self, other):
        if isinstance(other, (str, self.getClass())):
            return other
        return Schema.cast(self, other)

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
        key = self.getCurrent(box)
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

    def getCurrent(self, box):
        current = box.current
        if current is None:
            current = self.defaultValue
        if current is None:
            current = next(k for k in self.dict)
        return current


class ListOfNodes(Type):
    """A ListOfNodes is a list type whose schema contains a number of child
    node schemas which describe the values which can be added to the list.

    This doesn't really fit the whole Box/Configuration view of the world very
    well, so we resort to a very ugly hack here to get basic value storage
    and default values correct.

    This hack is being put in place to aid debris removal which will allow
    easier extraction of Box and it's associated black magic.
    """
    _hashname = 'LIST'

    @classmethod
    def parse(cls, key, hash, attrs, parent=None):
        # XXX: This. This should not be allowed to last long.
        from .configuration import Configuration

        self = cls(strict=False, **attrs)
        node_schemata = {}
        for k, h, a in hash.iterall():
            node_schemata[k] = Schema.parse(k, h, a)

        if not (self.defaultValue is None or
                all(name in node_schemata for name in self.defaultValue)):
            msg = 'the default value "{}" is not in {} for node {}'
            raise AssertionError(msg.format(self.defaultValue,
                                            list(hash.keys()), key))
        self.objs = {k: Configuration(k, 'class', descriptor=v)
                     for k, v in node_schemata.items()}
        self.metricPrefixSymbol = ''
        self.unitSymbol = ''
        return self

    def cast(self, other):
        assert_msg = '"{}" is not in the allowed classes ({})'

        if isinstance(other, Hash):
            # XXX: We're talking to a middlelayer device. The ML implementation
            # of ListOfNodes might as well be a different class, because it
            # doesn't have the same type for property values as Bound/C++ APIs
            return []  # No value for you!

        retval = []
        for val in other:
            if isinstance(val, str):
                assert val in self.objs, assert_msg.format(val, self.objs)
                hsh = Hash(val, self.objs[val].toHash()[0])
                retval.append(hsh)
            else:
                assert isinstance(val, Hash)
                name = next(iter(val))
                assert name in self.objs, assert_msg.format(name, self.objs)
                # XXX: Use a stored Configuration object to validate the
                # contents of the hash before adding it
                validator = self.objs[name]
                validator.fromHash(val[name])
                retval.append(val)
        return retval

    def toHash(self, box):
        # box.value is already a list of Hashes
        return box.value, {}

# ----------------------------------------------------------------------------
# Configuration editing extensions
# These are used by `karabo_gui.configurator.qt_item_model`


def get_editable_attributes(descriptor):
    """Return the editable attribute names of a descriptor
    """
    names = []
    for name in EDITABLE_ATTRIBUTE_NAMES:
        # `descriptor` can be None!
        value = getattr(descriptor, name, None)
        if value is not None:
            # Skip blank units
            if name == 'unitSymbol' and value == '':
                continue
            # Skip metric prefixes with no associated units
            if (name == 'metricPrefixSymbol' and value == ''
                    and getattr(descriptor, 'unitSymbol', '') == ''):
                continue
            names.append(name)
    return names


class EditableAttributeInfo(object):
    """This class records the editable attributes of a box. Each time
    the descriptor of a box is changed, a new instance of this class should
    be generated.
    """
    def __init__(self, box, descriptor, attrs=None):
        attrs = attrs or {}
        names = get_editable_attributes(descriptor)
        self.attrs = OrderedDict([(name,
                                   attrs.get(name, getattr(descriptor, name)))
                                  for name in names])
        self.parent = weakref.ref(box)

    def update(self, attrs):
        """Update the data with the given `attrs`
        """
        self.attrs.update(attrs)

    def get_data_by_index(self, index):
        """Return a tuple with the `name` and the `value` behind the given
        `index`
        """
        tuple_list = list(self.attrs.items())
        try:
            name, value = tuple_list[index]
        except IndexError:
            return '', None
        return name, value


class VectorHash(hashmod.VectorHash, metaclass=Monkey):
    def set(self, box, value, timestamp=None):
        box.rowsInfo = [VectorHashRowInfo(box) for row in value]
        box._set(value, timestamp)

    def fromHash(self, box, value, attrs=None, timestamp=None):
        box.rowsInfo = [VectorHashRowInfo(box) for row in value]
        Type.fromHash(self, box, value, attrs=attrs, timestamp=timestamp)

    def setDefault(self, box):
        if self.defaultValue is not None:
            self.set(box, self.defaultValue)


class VectorHashCellInfo(object):
    """A proxy for VectorHash cells (row & column) which is used by the
    configurator item model
    """
    def __init__(self, row, name, obj):
        self.name = name
        self.obj = obj
        self.parent = weakref.ref(row)


class VectorHashRowInfo(object):
    """A proxy for VectorHash rows which is used by the configurator item
    model
    """
    def __init__(self, box):
        self.columns = [VectorHashCellInfo(
            self, k, hashmod.Type.fromname[a["valueType"]](strict=False, **a))
            for k, v, a in box.descriptor.rowSchema.hash.iterall()]
        self.parent = weakref.ref(box)
