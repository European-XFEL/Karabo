from karabo.middlelayer import AccessMode
from karabo_gui.schema import Dummy, Schema, VectorHash


def get_attribute_data(attr_info, row):
    """Return the `name`, the `descriptor` and the actual value of the given
    `attr_info`
    """
    name = attr_info.names[row]
    box = attr_info.parent()
    if box is None:
        return name, None, ''
    descriptor = box.descriptor
    return name, descriptor, getattr(descriptor, name)


def get_box_value(box, is_edit_col=False):
    """Return the actual value of the given `box`, depending on whether this is
    requested for an editable column
    """
    descriptor = box.descriptor
    if isinstance(descriptor, (Schema, VectorHash)):
        return ''

    value = box.value
    if isinstance(value, (Dummy, bytes, bytearray)):
        return ''
    if is_edit_col:
        is_editable = (descriptor.accessMode in
                       (AccessMode.INITONLY, AccessMode.RECONFIGURABLE))
        if not is_editable:
            return ''

    return value


def get_vector_col_value(cell_info, is_edit_col=False):
    """Return the actual value of the given `cell_info`, depending on whether
    this is requested for an editable column
    """
    parent_row = cell_info.parent()
    if parent_row is None:
        return ''
    parent_box = parent_row.parent()
    if parent_box is None:
        return ''
    descriptor = parent_box.descriptor
    if is_edit_col:
        is_editable = (descriptor.accessMode in
                       (AccessMode.INITONLY, AccessMode.RECONFIGURABLE))
        if not is_editable:
            return ''

    row = parent_box.value[descriptor.rowsInfo.index(parent_row)]
    return row[cell_info.name]
