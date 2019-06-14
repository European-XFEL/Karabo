from xml.etree.ElementTree import SubElement

from traits.api import Float, Int

from karabo.common.savable import BaseSavableModel
from karabo.common.scenemodel.const import NS_KARABO
from karabo.common.scenemodel.io_utils import read_base_widget_data


KARABO_NoROI = 0
KARABO_RECT = 1
KARABO_CROSSHAIR = 2

KARABO_BASE_SAVABLE = ['x', 'y', 'width', 'height', 'keys', 'parent_component']

AXES_SET = ['x_grid', 'y_grid', 'x_log', 'y_log', 'x_invert', 'y_invert']
LABEL_UNITS = ['x_label', 'y_label', 'x_units', 'y_units']
RANGE_SET = ['x_min', 'x_max', 'y_min', 'y_max']
TRANSFORM_SET = ['x_scale', 'y_scale', 'x_translate', 'y_translate']

KARABO_ROI_ITEMS = 'roi_items'
KARABO_ROI_TYPE = 'roi_type'


class BaseROIData(BaseSavableModel):
    """The BaseROI Data Model being ``Savable`` and conering the ``type``"""
    roi_type = Int()


class CrossROIData(BaseROIData):
    """The CrossROIData Model for CrossHair markers"""
    x = Float()
    y = Float()

    @property
    def coords(self):
        return self.x, self.y


class RectROIData(BaseROIData):
    """The RectROIData Model for rectangular regions"""
    x = Float()
    y = Float()
    w = Float()
    h = Float()

    @property
    def coords(self):
        return self.x, self.y, self.w, self.h


def read_base_karabo_image_model(element):
    traits = read_base_widget_data(element)

    traits['aux_plots'] = int(element.get(NS_KARABO + 'aux_plots', '0'))
    traits['colormap'] = element.get(NS_KARABO + 'colormap', "viridis")
    traits['roi_tool'] = int(element.get(NS_KARABO + 'roi_tool', 0))
    traits['roi_items'] = read_roi_info(element)
    traits['aspect_ratio'] = int(element.get(NS_KARABO + 'aspect_ratio', 1))

    show_scale = element.get(NS_KARABO + 'show_scale', '1')
    traits['show_scale'] = bool(int(show_scale))

    traits.update(read_transforms(element))
    traits.update(read_basic_label(element))

    return traits


def write_base_karabo_image_model(model, element):
    element.set(NS_KARABO + 'colormap', model.colormap)
    element.set(NS_KARABO + 'aux_plots', str(model.aux_plots))
    element.set(NS_KARABO + 'roi_tool', str(model.roi_tool))
    element.set(NS_KARABO + 'aspect_ratio', str(model.aspect_ratio))

    show_scale = str(int(model.show_scale))
    element.set(NS_KARABO + 'show_scale', show_scale)

    # Save ROI configuration
    write_roi_info(model, element)
    write_transforms(model, element)
    write_basic_label(model, element)


def read_roi_info(element):
    """Extracts the ROIData list from the element XML"""
    roi_data = []

    for child_elem in element:
        if child_elem.tag != NS_KARABO + 'roi':
            continue

        roi_type = int(child_elem.get('roi_type', 0))
        traits = {'roi_type': roi_type}
        if roi_type == 1:  # Rect:
            traits['x'] = float(child_elem.get('x'))
            traits['y'] = float(child_elem.get('y'))
            traits['w'] = float(child_elem.get('w'))
            traits['h'] = float(child_elem.get('h'))
            roi_data.append(RectROIData(**traits))
        elif roi_type == 2:  # Crosshair:
            traits['x'] = float(child_elem.get('x'))
            traits['y'] = float(child_elem.get('y'))
            roi_data.append(CrossROIData(**traits))

    return roi_data


def write_roi_info(model, element):
    """Method responsible for serializing the ROIs information.

    Each model has an ROIModel, which contains the models for each of the
    available ROI classes. For each of these models, we have have a list of
    ROI items that were drawn.
    """
    for roi in model.roi_items:
        roi_element = SubElement(element, NS_KARABO + 'roi')
        for attribute in roi.class_visible_traits():
            roi_element.set(attribute, str(getattr(roi, attribute)))


def read_transforms(element):
    traits = {}
    for name in TRANSFORM_SET:
        value = element.get(NS_KARABO + name)
        traits[name] = float(value)

    return traits


def write_transforms(model, element):
    for name in TRANSFORM_SET:
        element.set(NS_KARABO + name, str(getattr(model, name)))


def read_axes_set(element):
    return {name: element.get(NS_KARABO + name, 'false').lower() == 'true'
            for name in AXES_SET}


def write_axes_set(model, element):
    for name in AXES_SET:
        element.set(NS_KARABO + name, str(getattr(model, name)))


def read_basic_label(element):
    return {name: element.get(NS_KARABO + name, '') for name in LABEL_UNITS}


def write_basic_label(model, element):
    for name in LABEL_UNITS:
        element.set(NS_KARABO + name, getattr(model, name))


def read_range_set(element):
    traits = {name: float(element.get(NS_KARABO + name, '0.0'))
              for name in RANGE_SET}
    traits['autorange'] = element.get(
        NS_KARABO + 'autorange', 'true').lower() == 'true'
    return traits


def write_range_set(model, element):
    for name in RANGE_SET:
        element.set(NS_KARABO + name, str(getattr(model, name)))
    element.set(NS_KARABO + 'autorange', str(model.autorange))


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
            if roi_type == KARABO_RECT:
                roi_items.append(RectROIData(**roi))
            elif roi_type == KARABO_CROSSHAIR:
                roi_items.append(CrossROIData(**roi))
        config[KARABO_ROI_ITEMS] = roi_items

    return config
