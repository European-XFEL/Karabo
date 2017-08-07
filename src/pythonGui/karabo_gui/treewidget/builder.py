from karabo.middlelayer import (
    AccessLevel, AccessMode, Assignment, Descriptor,
    Bool, Char, ComplexDouble, ComplexFloat, Double, Float,
    Int16, Int32, Int64, Int8, String, UInt16, UInt32, UInt64, UInt8,
    VectorBool, VectorChar, VectorComplexDouble, VectorComplexFloat,
    VectorDouble, VectorFloat, VectorHash, VectorInt16, VectorInt32,
    VectorInt64, VectorInt8, VectorString, VectorUInt16, VectorUInt32,
    VectorUInt64, VectorUInt8
)
from karabo.middlelayer_api.hash import ByteArray
from karabo_gui.attributeediting.api import (
    EDITABLE_ATTRIBUTE_NAMES, ATTRIBUTE_EDITOR_FACTORIES
)
from karabo_gui.configurator.api import ConfigurationTreeView
from karabo_gui.schema import (
    ChoiceOfNodes, ImageNode, ListOfNodes, OutputNode, Schema, SlotNode,
    TableNode
)
from karabo_gui.widget import EditableWidget
from .attribute_item import AttributeTreeWidgetItem
from .command_item import CommandTreeWidgetItem
from .image_item import ImageTreeWidgetItem
from .property_item import PropertyTreeWidgetItem
from .table_item import TableTreeWidgetItem
from .utils import get_icon


def fill_parameter_tree_widget(tree_widget, configuration):
    """Fill a `ParameterTreeWidget` with items which match up with the parts
    of `configuration`.
    """
    # XXX: Handle the new-style configuration tree widget
    if isinstance(tree_widget, ConfigurationTreeView):
        tree_widget.model().configuration = configuration
        configuration.parameterEditor = tree_widget
        tree_widget.resizeColumnToContents(0)
        return

    class_types = ('class', 'projectClass')

    configuration.parameterEditor = tree_widget
    descriptor = configuration.descriptor
    if descriptor is not None:
        assert isinstance(descriptor, Schema)
        is_class = configuration.type in class_types
        root_item = tree_widget.invisibleRootItem()
        _build_recursive(descriptor, tree_widget, root_item, configuration,
                         is_class)
        tree_widget.resizeColumnToContents(0)

    tree_widget.globalAccessLevelChanged()
    tree_widget.ensureMiddleColumnWidth()


def _build_recursive(descriptor, tree_widget, parent_item, box, is_class):
    """Recursively build the children of a tree widget item
    """
    for key, value in descriptor.dict.items():
        if isinstance(value, Descriptor):
            try:
                child = getattr(box.boxvalue, key)
            except AttributeError:
                print('missing {} in {}'.format(key, box.value))
            else:
                factory = _ITEM_FACTORIES[type(value)]
                factory(value, tree_widget, parent_item, child, is_class)


def _attribute_item_leaf(descriptor, tree_widget, parent_item, box, attr_name):
    """Build a single tree widget item for an attribute.
    """
    item = AttributeTreeWidgetItem(attr_name, box, tree_widget, parent_item)
    item.setIcon(0, get_icon(descriptor))

    factory = ATTRIBUTE_EDITOR_FACTORIES[attr_name]
    item.create_editable_widget(factory, box)
    item.make_class_connections(box)

    item.requiredAccessLevel = descriptor.requiredAccessLevel
    item.displayText = attr_name
    item.allowedStates = descriptor.allowedStates


def _finalize_build_base(descriptor, tree_widget, item, box, is_class):
    if descriptor.assignment == Assignment.MANDATORY.value:
        font = item.font(0)
        font.setBold(True)
        item.setFont(0, font)
    item.requiredAccessLevel = descriptor.requiredAccessLevel
    item.displayText = descriptor.displayedName
    item.allowedStates = descriptor.allowedStates


def _finalize_build_node(descriptor, tree_widget, item, box, is_class):
    _build_recursive(descriptor, tree_widget, item, box, is_class)
    _finalize_build_base(descriptor, tree_widget, item, box, is_class)


def _finalize_build_leaf(descriptor, tree_widget, item, box, is_class):
    _finalize_build_base(descriptor, tree_widget, item, box, is_class)
    # We only show attribute items for non-device instance tree widgets
    if not is_class:
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
            _attribute_item_leaf(descriptor, tree_widget, item, box, name)


def _item_choice_of_nodes(descriptor, tree_widget, parent_item, box, is_class):
    item = PropertyTreeWidgetItem(box, tree_widget, parent_item)
    item.defaultValue = descriptor.defaultValue
    item.isChoiceElement = True

    item.create_editable_widget(EditableWidget.getClass(box), box)
    if is_class:
        # XXX: Check for INITONLY and RECONFIGURABLE?
        item.make_class_connections(box)
    else:
        # XXX: Check for RECONFIGURABLE?
        item.make_device_connections(box)
    _finalize_build_node(descriptor, tree_widget, item, box, is_class)

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

        children.append(child)
    item.editable_widget.childItemList = children

    # Trigger change of combobox
    item.editable_widget.valueChanged(box, box.current)
    return item


def _item_image_node(descriptor, tree_widget, parent_item, box, is_class):
    item = ImageTreeWidgetItem(box, tree_widget, parent_item)
    item.enabled = not is_class
    _finalize_build_node(descriptor, tree_widget, item, box, is_class)


def _item_list_of_nodes(descriptor, tree_widget, parent_item, box, is_class):
    # XXX: This item has obviously rotted!
    item = PropertyTreeWidgetItem(box, tree_widget, parent_item)
    item.displayText = box.path[-1]
    item.requiredAccessLevel = AccessLevel.GOD


def _item_slot_node(descriptor, tree_widget, parent_item, box, is_class):
    item = CommandTreeWidgetItem(descriptor.key, box, tree_widget, parent_item)
    item.enabled = not is_class
    _finalize_build_node(descriptor, tree_widget, item, box, is_class)


def _item_table_node(descriptor, tree_widget, parent_item, box, is_class):
    item = TableTreeWidgetItem(box, tree_widget, parent_item)
    _finalize_build_node(descriptor, tree_widget, item, box, is_class)


def _item_leaf(descriptor, tree_widget, parent_item, box, is_class,
               item_factory=PropertyTreeWidgetItem):

    item = item_factory(box, tree_widget, parent_item)
    item.setIcon(0, get_icon(descriptor))

    if is_class:
        if descriptor.accessMode in (AccessMode.INITONLY,
                                     AccessMode.RECONFIGURABLE):
            item.create_editable_widget(EditableWidget.getClass(box), box)
            item.make_class_connections(box)
    else:
        if descriptor.accessMode is AccessMode.RECONFIGURABLE:
            item.create_editable_widget(EditableWidget.getClass(box), box)
            item.make_device_connections(box)
    _finalize_build_leaf(descriptor, tree_widget, item, box, is_class)
    return item


def _item_node(descriptor, tree_widget, parent_item, box, is_class):
    item = PropertyTreeWidgetItem(box, tree_widget, parent_item)
    _finalize_build_node(descriptor, tree_widget, item, box, is_class)
    return item


def _item_vector_hash(descriptor, tree_widget, parent_item, box, is_class):
    item_factory = TableTreeWidgetItem
    return _item_leaf(descriptor, tree_widget, parent_item, box, is_class,
                      item_factory=item_factory)


_ITEM_FACTORIES = {
    Bool: _item_leaf,
    ByteArray: _item_leaf,
    Char: _item_leaf,
    ChoiceOfNodes: _item_choice_of_nodes,
    ComplexDouble: _item_leaf,
    ComplexFloat: _item_leaf,
    Double: _item_leaf,
    Float: _item_leaf,
    ImageNode: _item_image_node,
    Int16: _item_leaf,
    Int32: _item_leaf,
    Int64: _item_leaf,
    Int8: _item_leaf,
    ListOfNodes: _item_list_of_nodes,
    OutputNode: _item_node,
    Schema: _item_node,
    SlotNode: _item_slot_node,
    String: _item_leaf,
    TableNode: _item_table_node,
    UInt16: _item_leaf,
    UInt32: _item_leaf,
    UInt64: _item_leaf,
    UInt8: _item_leaf,
    VectorBool: _item_leaf,
    VectorChar: _item_leaf,
    VectorComplexDouble: _item_leaf,
    VectorComplexFloat: _item_leaf,
    VectorDouble: _item_leaf,
    VectorFloat: _item_leaf,
    VectorHash: _item_vector_hash,
    VectorInt16: _item_leaf,
    VectorInt32: _item_leaf,
    VectorInt64: _item_leaf,
    VectorInt8: _item_leaf,
    VectorString: _item_leaf,
    VectorUInt16: _item_leaf,
    VectorUInt32: _item_leaf,
    VectorUInt64: _item_leaf,
    VectorUInt8: _item_leaf,
}
