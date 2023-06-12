# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from collections import defaultdict

from pyqtgraph import Point
from qtpy.QtCore import QObject, Signal, Slot

from karabogui.graph.common.enums import ROITool
from karabogui.graph.common.roi.crosshair import CrosshairROI
from karabogui.graph.common.roi.rect import RectROI


class BaseROIController(QObject):
    """
    This class is responsible for managing all the ROIs of a plotItem.
    Therefore, all ROIs should be added/removed from here. It offers 4
    signals for communicating to the outside:
        - updated: when the ROI information is refreshed
        - removed: when an ROI is removed
        - selected: when one of the ROIs is selected
    """
    updated = Signal(object)
    removed = Signal()
    selected = Signal(object)

    TOOL_MAP = {
        ROITool.Rect: RectROI,
        ROITool.Crosshair: CrosshairROI
    }

    def __init__(self, plotItem):
        super().__init__()
        self._rois = defaultdict(list)

        self.plotItem = plotItem

        self._current_tool = ROITool.NoROI
        self._current_item = defaultdict(lambda: None)

        self._updates_enabled = False

        # Position snapping is disabled by default.
        # This is usually utilized by image ROIs
        self.scaleSnap = False
        self.translateSnap = False

        self._pen = None

    # ---------------------------------------------------------------------
    # Public methods

    @property
    def roi_items(self):
        return dict(self._rois)

    @property
    def current_tool(self):
        return self._current_tool

    @property
    def current_tool_item(self):
        return self._current_item[self._current_tool]

    def show(self, roi):
        """
        Enables the given ROITool.
        """
        self._current_tool = roi
        if roi == ROITool.NoROI:
            self.hide_all()
        else:
            for class_ in self._rois.keys():
                if roi != class_:
                    self._hide_roi_class(class_)

            self._show_roi_class(roi)

    def add(self, tool, pos, size=None, name='', ignore_bounds=False,
            current_item=True):
        # Get the ROI class
        roi_class = self.TOOL_MAP[tool]
        pos = Point(pos)

        if size is not None:
            roi_item = roi_class(pos=pos, size=size, name=name,
                                 scaleSnap=self.scaleSnap,
                                 translateSnap=self.translateSnap,
                                 pen=self._pen)
        else:
            roi_item = roi_class(pos=pos, name=name,
                                 scaleSnap=self.scaleSnap,
                                 translateSnap=self.translateSnap,
                                 pen=self._pen)

        # Connect some signals
        roi_item.sigRegionChangeStarted.connect(self._set_current_item)
        roi_item.sigRegionChanged.connect(self.update)
        roi_item.sigRemoveRequested.connect(self._remove_roi_item)
        roi_item.sigClicked.connect(self._set_current_item)

        # Bookkeeping. Add the items to the list of existing ROI tools
        self._rois[tool].append(roi_item)

        # Add the ROI item to the plot
        self._add_to_plot(roi_item, ignore_bounds)

        if current_item:
            # Set as current item, in which affects the aux plots.
            self._current_tool = tool
            self._set_current_item(roi_item, update=False)

        return roi_item

    def set_pen(self, pen):
        self._pen = pen

    def _add_to_plot(self, roi_item, ignore_bounds=True):
        self.plotItem.vb.addItem(roi_item, ignoreBounds=ignore_bounds)
        if roi_item.textItem:
            self.plotItem.vb.addItem(roi_item.textItem,
                                     ignoreBounds=ignore_bounds)

    def _remove_from_plot(self, roi_item):
        roi_item.sigRegionChanged.disconnect()
        roi_item.sigRegionChangeStarted.disconnect()
        roi_item.sigClicked.disconnect()
        roi_item.set_visible(False)

        if roi_item.textItem:
            self.plotItem.vb.removeItem(roi_item.textItem)
            roi_item.textItem.deleteLater()

        self.plotItem.vb.removeItem(roi_item)
        roi_item.deleteLater()

    def hide_all(self):
        """
        Hide all added ROI classes.
        """
        for roi_class in self._rois.keys():
            self._hide_roi_class(roi_class)

    def remove_all(self):
        for roi_items in self._rois.values():
            for item in roi_items:
                self._remove_roi_item(item)

        self._rois.clear()

    def get_coords(self):
        """Retrieves the absolute coords of all existing ROIs and returns dict:
           {ROITool.Crosshair: [(0,0), (1,1), ..], ROITool.Rect: [(0,0,0,0)]}
        """
        roi_coords = {}
        for roi, items in self._rois.items():
            roi_coords[roi] = [item.coords for item in items]
        return roi_coords

    def destroy(self):
        self.updated.disconnect()
        self.deleteLater()

    def enable_updates(self, enable=True):
        self._updates_enabled = enable
        self.update()

    @Slot()
    def update(self):
        """Subclass for ROI region change interactions"""

    # ---------------------------------------------------------------------
    # Private methods

    def _hide_roi_class(self, roi_class):
        """
        Hides the ROI belonging to the given class.
        """
        roi_items = self._rois[roi_class]
        if not roi_items:
            return

        for item in roi_items:
            item.set_visible(False)

    @Slot(object)
    def _remove_roi_item(self, roi_item):
        """
        Deletes the given ROI and removes it from the list.
        """
        # Delete the item from the list
        roi_items = self._rois[self._current_tool]
        roi_items.remove(roi_item)

        # Check first if current item
        if roi_item is self._current_item[self._current_tool]:
            # Give the honor to others if available.
            if roi_items:
                self._set_current_item(roi_items[-1])
            else:
                self._set_current_item(None)
                self.removed.emit()

        # Then delete the item
        self._remove_from_plot(roi_item)

    def _show_roi_class(self, roi_class):
        """
        Show a ROI item for a given class, creating a new one if it's not
        available.
        """
        # Use already created ROI if available
        roi_items = self._rois[roi_class]

        # Do nothing if no recorded items
        if not roi_items:
            return

        for item in roi_items:
            self._show_roi_item(item)

        # Set last item as current if None
        if self._current_item[roi_class] is None:
            self._set_current_item(item)

    def _show_roi_item(self, roi_item):
        """ Shows an ROI item """
        roi_item.set_visible(True)

    @Slot(object)
    def _set_current_item(self, roi_item, update=True):
        # Check if item is the same as current
        if roi_item is self._current_item[self._current_tool]:
            return

        # Deselect current ROI
        current_item = self._current_item[self._current_tool]
        if current_item is not None:
            current_item.selected = False

        # Put new ROI as selected
        self._current_item[self._current_tool] = roi_item
        if roi_item is None:
            self._current_tool = ROITool.NoROI
        else:
            roi_item.selected = True
