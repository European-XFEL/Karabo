from graph.controllers.plot_models import BasePlotModel
from graph.controllers.model_tools import CrossROIData, RectROIData
from .enums import ROITool

KARABO_BASE_SAVABLE = ['x', 'y', 'width', 'height',
                       'keys', 'parent_component']


def build_model_config(model):
    """Build a configuration dictionary from a model

    This function removes the base savable trait settings!
    """
    config = {}
    for name in model.copyable_trait_names():
        if name in KARABO_BASE_SAVABLE:
            continue
        config[name] = getattr(model, name)
    return config


KARABO_ROI_ITEMS = 'roi_items'
KARABO_ROI_TYPE = 'roi_type'


def build_graph_config(model):
    """Build a graph configuration dictionary from a model

    This function removes the base savable trait settings!
    """
    config = {}
    for name in model.copyable_trait_names():
        if name in KARABO_BASE_SAVABLE:
            continue
        elif name == KARABO_ROI_ITEMS:
            roi_items = []
            roi_data = getattr(model, KARABO_ROI_ITEMS)
            for roi in roi_data:
                traits = {}
                for subname in roi.copyable_trait_names():
                    traits[subname] = getattr(roi, subname)
                roi_items.append(traits)
            config[KARABO_ROI_ITEMS] = roi_items
        else:
            # No special case, just write!
            config[name] = getattr(model, name)

    return config


def restore_graph_config(config):
    """Restore a graph configuration from a dictionary

    This function makes sure that certain data can be written on the
    traits model!
    """
    roi_data = config.get(KARABO_ROI_ITEMS, None)
    if roi_data is not None and roi_data:
        roi_items = []
        for roi in roi_data:
            roi_type = roi[KARABO_ROI_TYPE]
            if roi_type == ROITool.Crosshair:
                roi_items.append(CrossROIData(**roi))
            elif roi_type == ROITool.Rect:
                roi_items.append(RectROIData(**roi))
        config[KARABO_ROI_ITEMS] = roi_items

    return config


def get_configuration():
    return build_model_config(BasePlotModel())
