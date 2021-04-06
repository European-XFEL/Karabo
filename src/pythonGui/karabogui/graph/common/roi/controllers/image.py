from qtpy.QtCore import Slot

from karabogui.graph.common.const import SCALING, TRANSLATION
from karabogui.graph.common.enums import ROITool
from karabogui.graph.common.roi.utils import ImageRegion

from .base import BaseROIController


class ImageROIController(BaseROIController):

    def __init__(self, plotItem):
        super(ImageROIController, self).__init__(plotItem)
        plotItem.imageTransformed.connect(self._update_transform)

        # Enable movement wrt image pixels
        self._scale_snap = True
        self._translate_snap = True

    def show(self, roi):
        super(ImageROIController, self).show(roi)
        if self.plotItem.image_set:
            # Create a first ROI read
            self.update()

    def _show_roi_item(self, roi_item):
        super(ImageROIController, self)._show_roi_item(roi_item)
        self._update_geometry(roi_item)

    def destroy(self):
        self.plotItem.imageTransformed.disconnect(self._update_transform)
        super(ImageROIController, self).destroy()

    @Slot()
    def update(self):
        """Emits the ROI information, which can be either from the whole plot
           or the ROI rectangle."""
        if not self.plotItem.image_set or not self._updates_enabled:
            return

        image = self.plotItem.image

        # Get current ROI item
        if self._current_tool is ROITool.NoROI:
            current_item = None
        else:
            current_item = self._current_item[self._current_tool]

        # Get data of the region. If ROI is none, get the whole image.
        if current_item is None:
            region = ImageRegion(image, ImageRegion.Area,
                                 x_slice=slice(image.shape[1]),
                                 y_slice=slice(image.shape[0]))
        else:
            region = current_item.get_region(self.plotItem.imageItem)

        self.updated.emit(region)

    @Slot()
    def _update_geometry(self, roi=None):
        """Adjusts ROI item geometry with the current image transform"""
        if self._current_tool == ROITool.NoROI:
            return

        # If roi is None, transform all visible ROI items
        if roi is None:
            roi_items = self._rois[self._current_tool]
        else:
            roi_items = [roi]

        for item in roi_items:
            item.update_geometry_from_transform(
                self.plotItem.axes_transform[SCALING],
                self.plotItem.axes_transform[TRANSLATION], update=False)

    @Slot(object)
    def _set_current_item(self, roi_item, update=True):
        super(ImageROIController, self)._set_current_item(roi_item, update)
        if update and self.plotItem.image_set:
            self.update()

    @Slot()
    def _update_transform(self):
        self._update_geometry()
        self.update()
