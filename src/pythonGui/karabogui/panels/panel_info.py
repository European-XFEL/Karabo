from qtpy.QtCore import QModelIndex
from traits.api import Undefined

from karabo.native import Hash
from karabogui.binding.api import (
    ChoiceOfNodesBinding, ListOfNodesBinding, NodeBinding)

_RECURSIVE_BINDINGS = (ListOfNodesBinding, ChoiceOfNodesBinding)

HASH_TYPE = "hash"
LOGBOOK_KEY_DATA_DTYPE = "dataType"
LOGBOOK_KEY_DATA = "data"


def _friendly_repr(binding, value):
    """Return a user-friendly value"""
    if binding is None or value is Undefined:
        return str(value)

    converters = {"hex": hex, "oct": oct, "bin": bin}
    base = binding.display_type
    if base in converters:
        try:
            return converters[base](value)
        except TypeError:
            # value could be an empty string
            return value
    return value


def create_configurator_info(panel):
    """Create the logbook info for the configuration panel"""

    model = panel.model()

    def _get_row_data(parent_index):
        """Recursively yield the row data for a QModelIndex"""
        for row in range(model.rowCount(parent_index)):
            proxy_index = model.index(row, 1, parent_index)
            proxy = model.index_ref(proxy_index)
            # No pipeline data!
            if proxy is None or proxy.pipeline_parent_path:
                continue
            elif isinstance(proxy.binding, _RECURSIVE_BINDINGS):
                continue
            elif isinstance(proxy.binding, NodeBinding):
                yield from _get_row_data(model.index(row, 0, parent_index))
            else:
                yield proxy.path, _friendly_repr(proxy.binding, proxy.value)

    data = Hash({key: value for key, value
                 in _get_row_data(QModelIndex())})

    ret = {}
    ret[LOGBOOK_KEY_DATA] = data
    ret[LOGBOOK_KEY_DATA_DTYPE] = HASH_TYPE

    return ret


def create_scene_info(panel):
    """Create the logbook info for the scene panel"""
    scene_view = panel.scene_view
    proxies = [proxy for proxy in scene_view.cached_proxies()
               if not isinstance(proxy.binding, NodeBinding)]
    data = Hash({proxy.key: _friendly_repr(proxy.binding, proxy.value)
                 for proxy in proxies})

    ret = {}
    ret[LOGBOOK_KEY_DATA] = data
    ret[LOGBOOK_KEY_DATA_DTYPE] = HASH_TYPE

    return ret
