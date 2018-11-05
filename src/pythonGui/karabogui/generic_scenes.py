from karabo.common.scenemodel.api import get_trendline_scene, get_image_scene
from karabogui.binding.types import (
    BoolBinding, FloatBinding, ImageBinding, IntBinding, NodeBinding,
    PipelineOutputBinding)


def get_generic_scene(proxy):
    """Return a generic scene for a proxy binding

    :param proxy: property proxy
    """
    binding = getattr(proxy, 'binding', None)
    if binding is None:
        return

    instance_id = proxy.root_proxy.device_id

    if isinstance(binding, (BoolBinding, FloatBinding, IntBinding)):
        path = proxy.path
        return get_trendline_scene(instance_id, path)

    elif isinstance(binding, PipelineOutputBinding):

        def _iter_binding(node, base=''):
            namespace = node.value
            base = base + '.' if base else ''
            for name in namespace:
                subname = base + name
                subnode = getattr(namespace, name)
                if isinstance(subnode, NodeBinding):
                    yield from _iter_binding(subnode, base=subname)
                yield subname, subnode

        for path, node in _iter_binding(binding, base=proxy.path):
            if isinstance(node, ImageBinding):
                return get_image_scene(instance_id, path)

    elif isinstance(binding, ImageBinding):
        return get_image_scene(instance_id, proxy.path)

    return None
