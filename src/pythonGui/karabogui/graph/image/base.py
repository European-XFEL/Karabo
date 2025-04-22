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
from functools import partial

from pyqtgraph import GraphicsLayoutWidget
from qtpy.QtCore import QEvent, QSize, Qt, Signal, Slot
from qtpy.QtGui import QPalette
from qtpy.QtWidgets import QAction, QGridLayout, QWidget

from karabogui import icons
from karabogui.graph.common.api import (
    COLORMAPS, AuxPlots, AxesLabelsDialog, ExportTool, ImageROIController,
    MouseTool, PointCanvas, RectCanvas, ROITool, ToolbarController)
from karabogui.graph.common.const import TF_SCALING, X_AXIS_HEIGHT

from .aux_plots.controller import AuxPlotsController
from .colorbar import ColorBarWidget
from .dialogs.transforms import ImageTransformsDialog
from .legends.scale import ScaleLegend
from .plot import KaraboImagePlot
from .tools.picker import PickerController
from .tools.toolbar import AuxPlotsToolset
from .utils import (
    create_colormap_menu, create_icon_from_colormap, levels_almost_equal)

ROI_CANVAS_MAP = {
    ROITool.DrawRect: (RectCanvas, ROITool.Rect),
    ROITool.DrawCrosshair: (PointCanvas, ROITool.Crosshair)}


class KaraboImageView(QWidget):
    stateChanged = Signal(object)
    toolTipChanged = Signal()

    def __init__(self, parent=None):
        """ The main image view widget for a single ImagePlot

        Acts as a container of the graphics layout, the image plotitem,
        the toolbar and other tooling features.
        """
        super().__init__(parent)

        # Main layout to organize
        layout = QGridLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(layout)

        self.image_layout = GraphicsLayoutWidget()
        self.layout().addWidget(self.image_layout, 0, 0, 1, 1)

        # Image can have transparent background!
        palette = self.image_layout.palette()
        palette.setColor(QPalette.Background, Qt.transparent)
        self.image_layout.setPalette(palette)

        # Add our basic plotItem to this widget
        self.plotItem = KaraboImagePlot(parent=self.image_layout.ci)
        self.plotItem.imageLevelsChanged.connect(self._image_levels_changed)
        self.plotItem.imageItem.sigImageChanged.connect(self._image_changed)

        # row, col, row_span, col_span
        self.image_layout.addItem(self.plotItem, 1, 1, 1, 1)
        self.image_layout.ci.layout.setRowStretchFactor(1, 3)
        self.image_layout.ci.layout.setColumnStretchFactor(1, 3)
        # Note: Leave a little bottom margin for the color bar
        self.image_layout.ci.layout.setContentsMargins(0, 0, 0, 5)

        # Our tooling instances
        self._aux_plots = None
        self._colorbar = None
        self._canvas = None
        self._picker = None

        self.toolbar = None
        self.roi = None

        self._colormap_action = None
        self._apply_action = None
        self._scale_legend = None
        self._save_levels_action = None

        self.configuration = {}

    # -----------------------------------------------------------------------
    # Public tool methods

    def add_toolbar(self):
        """Enable the standard toolbar on the widget

        This is the finalizer of the `KaraboImageView` widget. All signals
        should be connected here!
        """
        if self.toolbar is not None:
            raise ValueError('Toolbar already added')

        # Instantiate toolbar, has a default mouse mode toolset
        self.toolbar = tb = ToolbarController(parent=self)
        tb.toolsets[MouseTool].on_trait_change(
            self.set_mouse_mode, "current_tool")

        # Add the optional mouse mode: picker tool
        if self._picker is not None:
            tb.add_tool(MouseTool.Picker)

        # Add Aux Plots toolset
        if self._aux_plots is not None:
            tb.register_toolset(toolset=AuxPlots, klass=AuxPlotsToolset)
            aux_plots_toolset = tb.add_toolset(AuxPlots)
            aux_plots_toolset.on_trait_change(self.show_aux_plots,
                                              "current_tool")

        # Add ROI toolset
        if self.roi is not None:
            roi_toolset = tb.add_toolset(ROITool)
            roi_toolset.on_trait_change(self._activate_roi_tool,
                                        "current_tool")
            # Connect signals from the items
            self.roi.selected.connect(roi_toolset.check)
            self.roi.removed.connect(roi_toolset.uncheck_all)

        # Add Export toolset
        export_toolset = tb.add_toolset(ExportTool)
        export_toolset.on_trait_change(self.plotItem.export, "current_tool")

        # row, col, row_span, col_span
        self.layout().addWidget(tb.widget, 0, 1, 1, 1)

        return self.toolbar

    def add_colorbar(self):
        """Enable the standard colorbar of this widget"""
        if self._colorbar is None:
            plotItem = self.plotItem

            image = plotItem.imageItem
            self._colorbar = ColorBarWidget(image, parent=plotItem)

            top_axis_checked = plotItem.getAxis("top").style["showValues"]
            top_margin = X_AXIS_HEIGHT * top_axis_checked

            bot_axis_checked = plotItem.getAxis("bottom").style["showValues"]
            bottom_margin = X_AXIS_HEIGHT * bot_axis_checked

            self._colorbar.set_margins(top=top_margin, bottom=bottom_margin)
            self._colorbar.levelsChanged.connect(
                plotItem.set_image_levels)

            self.image_layout.addItem(self._colorbar, row=1, col=2)
            self.image_layout.ci.layout.setColumnStretchFactor(2, 1)

            self.add_colormap_action()

            if self._save_levels_action is None:
                self._save_levels_action = QAction(self)
                self._save_levels_action.setIcon(icons.apply)
                self._save_levels_action.setIconText("Save Color Levels")
                self._save_levels_action.triggered.connect(
                    self._save_color_levels)
                self.addAction(self._save_levels_action)

        return self._colorbar

    def remove_colorbar(self):
        if self._colorbar is not None:
            self.image_layout.removeItem(self._colorbar)
            self.image_layout.ci.layout.setColumnStretchFactor(1, 3)
            self._colorbar.deleteLater()
            self._colorbar = None

            self.remove_colormap_action()

    def add_roi(self, enable=True):
        if enable and self.roi is None:
            # Initialize ROI controller
            self.roi = ImageROIController(self.plotItem)
            if self._apply_action is None:
                self._apply_action = QAction(self)
                self._apply_action.setIcon(icons.apply)
                self._apply_action.setIconText('Set ROI and Aux')
                self._apply_action.triggered.connect(
                    self._set_roi_configuration)
                self.addAction(self._apply_action)

            # Set the minimum size of the plot without rescaling problems
            # from the ROI items (e.g., zooming out of infinity when it is
            # collapsed), which is 300 x 300.
            self.plotItem.setMinimumWidth(300)
            self.plotItem.setMinimumHeight(300)

        elif not enable and self.roi is not None:
            # Remove ROI items and destroy controller
            self.roi.remove_all()
            self.roi.destroy()
            self.roi = None
            self._apply_action.disconnect()
            self.removeAction(self._apply_action)
            self._apply_action = None

        return self.roi

    def add_picker(self, enable=True):
        if enable and self._picker is None:
            self._picker = PickerController(self.plotItem, parent=self)
        elif not enable and self._picker is not None:
            self._picker.destroy()
            self._picker = None

        return self._picker

    def add_aux(self, plot=None, **config):
        """Add a auxiliary plots to the ImageView"""
        if plot is not None:
            if self._aux_plots is None:
                # Create an instance of the aux plots controller with the klass
                self._aux_plots = AuxPlotsController(
                    image_layout=self.image_layout)  # noqa

                self.plotItem.imageAxesChanged.connect(self._set_axes_to_aux)

                # If ROI exists
                if self.roi is not None:
                    self.roi.updated.connect(self._aux_plots.process)

            controller = self._aux_plots.add(plot, **config)
            controller.link_view(self.plotItem.vb)

        else:
            if plot is AuxPlots.NoPlot:
                self._aux_plots.current_plot.link_view(None)
                self._aux_plots.clear()

                # If ROI exists
                if self.roi is None:
                    self.roi.updated.disconnect(self._aux_plots.analyze)

                self.plotItem.imageAxesChanged.disconnect(
                    self._set_axes_to_aux)

        return self._aux_plots

    def enable_aux(self):
        if self.toolbar is None:
            return

        # Enable toolbar buttons
        toolset = self.toolbar.toolsets.get(AuxPlots)
        if toolset is not None:
            for button in toolset.buttons.values():
                button.setEnabled(True)

    def disable_aux(self):
        if self._aux_plots is None:
            return

        # Hide existing aux plot if showing
        self.restore({"aux_plots": AuxPlots.NoPlot})

        # Disable toolbar buttons
        if self.toolbar is not None:
            toolset = self.toolbar.toolsets.get(AuxPlots)
            if toolset is not None:
                for aux_plot, button in toolset.buttons.items():
                    if aux_plot == self._aux_plots.current_plot:
                        button.setChecked(False)
                    button.setEnabled(False)

    def add_colormap_action(self, cmap="none"):
        if self._colormap_action is None:
            menu = create_colormap_menu(COLORMAPS, cmap, self.set_colormap)
            self._colormap_action = QAction(self)
            self._colormap_action.setIconText("Colormap")
            self._colormap_action.setMenu(menu)
            self.addAction(self._colormap_action)

        return self._colormap_action

    def remove_colormap_action(self):
        if self._colormap_action is not None:
            self._colormap_action.deleteLater()
            self.removeAction(self._colormap_action)
            self._colormap_action = None

    def add_axes_labels_dialog(self):
        axes_action = QAction("Axes Labels", self)
        axes_action.triggered.connect(self._show_labels_dialog)
        self.addAction(axes_action)

        return axes_action

    def add_transforms_dialog(self):
        transforms_action = QAction("Transformations", self)
        transforms_action.triggered.connect(self._show_transforms_dialog)
        self.addAction(transforms_action)

        return transforms_action

    # -----------------------------------------------------------------------
    # Public Interface

    def plot(self):
        return self.plotItem

    def set_colormap(self, colormap, update=True):
        self.plotItem.set_colormap(colormap)
        if self._colorbar is not None:
            self._colorbar.set_colormap(colormap)
        if self._aux_plots is not None:
            self._aux_plots.set_config(AuxPlots.Histogram, colormap=colormap)

        # NOTE: We might have a colormap action without colorbar!
        if self._colormap_action is not None:
            self._colormap_action.setIcon(create_icon_from_colormap(colormap))

        if self._picker is not None:
            self._picker.update()

        if update:
            config = {'colormap': colormap}
            self.configuration.update(**config)
            self.stateChanged.emit(config)

    def add_widget(self, widget, row, col, row_span=1, col_span=1):
        self.layout().addWidget(widget, row, col, row_span, col_span)

    def add_layout(self, layout, row, col, row_span=1, col_span=1):
        self.layout().addLayout(layout, row, col, row_span, col_span)

    def restore(self, configuration):
        """This method is responsible for restoring the state of all widgets.

        Ideally this should be called at the end of the widget setup.
        """
        # Update internal configuration
        self.configuration.update(**configuration)

        # Restore colormap
        colormap = configuration.get('colormap')
        if colormap is not None:
            self.set_colormap(colormap, update=False)
            # We check the action as well.
            if self._colormap_action is not None:
                for cmap_action in self._colormap_action.menu().actions():
                    if cmap_action.text() == colormap:
                        cmap_action.setChecked(True)

        # Restore labels
        x_label = configuration.get('x_label')
        x_units = configuration.get('x_units')
        if x_label is not None or x_units is not None:
            self.set_label(axis=0,
                           text=x_label or '',
                           units=x_units or '')
        y_label = configuration.get('y_label')
        y_units = configuration.get('y_units')
        if y_label is not None or y_units is not None:
            self.set_label(axis=1,
                           text=y_label or '',
                           units=y_units or '')

        # Restore transforms
        transforms = {k: v for k, v in configuration.items()
                      if k in ['x_scale', 'y_scale',
                               'x_translate', 'y_translate',
                               'aspect_ratio']}
        if transforms:
            kwargs = transforms
            kwargs.update({'default': True})
            self.plotItem.set_transform(**kwargs)
            # Restore scale legend
            show_legend = configuration.get('show_scale')
            self._show_scale_legend(show_legend)
            self._update_scale_legend()

        # Restore ROIs
        current_roi_tool = configuration.get('roi_tool')
        roi_items = configuration.get('roi_items')
        if self.roi is not None:
            # Restore ROI items
            if roi_items is not None:
                self.roi.remove_all()
                for roi_data in roi_items:
                    roi_type = roi_data['roi_type']
                    name = roi_data['name']
                    if roi_type == ROITool.Rect:
                        x, y, w, h = (roi_data['x'], roi_data['y'],
                                      roi_data['w'], roi_data['h'])
                        pos = x, y
                        size = w, h
                    elif roi_type == ROITool.Crosshair:
                        pos = (roi_data['x'], roi_data['y'])
                        size = None
                    else:
                        continue

                    self.roi.add(roi_type, pos, size=size, name=name)

            # Show ROI tool
            if current_roi_tool is not None:
                self.roi.show(current_roi_tool)

        # Restore auxiliary plots
        aux_plots_class = configuration.get('aux_plots')
        if self._aux_plots is not None and aux_plots_class is not None:
            self.show_aux_plots(aux_plots_class)

        # Restore toolbar state
        if self.toolbar is not None:
            # Restore ROI state
            toolset = self.toolbar.toolsets.get(ROITool)
            if toolset is not None and current_roi_tool is not None:
                toolset.check(current_roi_tool)

            # Restore aux plots state
            toolset = self.toolbar.toolsets.get(AuxPlots)
            if toolset is not None and aux_plots_class is not None:
                toolset.check(aux_plots_class)

        # Restore color levels if available (None, [])
        color_levels = configuration.get('color_levels')
        if not color_levels:
            color_levels = None
        self.plotItem.set_image_levels(color_levels)

    def deactivate_roi(self):
        """Deactivate the region of interest"""
        if self.roi is not None:
            for name, roi_type in self.roi.roi_items.items():
                for roi in roi_type:
                    roi.translatable = False

            if self.toolbar is not None:
                for roi_type in [ROITool.Rect, ROITool.Crosshair]:
                    buttons = self.toolbar.buttons[roi_type.name]
                    menu = buttons.menu()
                    action = menu.actions()[1]
                    menu.removeAction(action)

    # -----------------------------------------------------------------------
    # Qt Slots

    @Slot()
    def _image_changed(self):
        """Unified handle of image changes. This is to avoid race conditions.
           Will move the picker and ROI slots here as well"""

        # Check effective colorbar levels. In the ideal world, the effective
        # levels should not be bounded to the colorbar.
        if self._colorbar is not None:
            image_levels = self.plotItem.imageItem.levels
            if not levels_almost_equal(self._colorbar.levels, image_levels):
                self._colorbar.set_levels(image_levels)
                if self._aux_plots is not None:
                    self._aux_plots.set_config(plot=AuxPlots.Histogram,
                                               levels=image_levels)

        if self.roi is not None:
            self.roi.update()

    @Slot(object)
    def _image_levels_changed(self, levels):
        """Unified handle of image level changes."""

        if self._picker is not None:
            self._picker.update()

        if self._aux_plots is not None and self._colorbar is not None:
            levels = levels if levels is not None else self._colorbar.levels
            self._aux_plots.set_config(plot=AuxPlots.Histogram, levels=levels)

    @Slot(MouseTool)
    def set_mouse_mode(self, mode):
        self.plotItem.vb.set_mouse_tool(mode)
        if self._picker is not None:
            self._picker.activate(mode is MouseTool.Picker)

    @Slot(AuxPlots)
    def show_aux_plots(self, plot_type):
        """Hides/shows the auxiliar plots set for this controller"""
        if self._aux_plots is not None:
            self._aux_plots.current_plot = plot_type

        if self.roi is not None:
            self.roi.enable_updates(plot_type != AuxPlots.NoPlot)

    @Slot()
    def _show_labels_dialog(self):
        config, result = AxesLabelsDialog.get(self.configuration,
                                              parent=self)

        if not result:
            return

        self.set_label(axis=0,
                       text=config["x_label"],
                       units=config["x_units"])
        self.set_label(axis=1,
                       text=config["y_label"],
                       units=config["y_units"])

        self._update_scale_legend()

        self.configuration.update(**config)
        self.stateChanged.emit(config)

    def set_label(self, axis, text='', units=''):
        """Public interface similar with to KaraboPlotView"""
        self.plotItem.set_label(axis, text, units)

        # Modify colorbar top margin whether labels are present/absent
        if axis == 0 and self._colorbar is not None:
            margin = X_AXIS_HEIGHT
            if text == '' and units == '':
                margin -= 20
            self._colorbar.set_margins(top=margin)

    @Slot()
    def _show_transforms_dialog(self):
        transform = self.plotItem.axes_transform
        aspect_ratio = self.plotItem.aspect_ratio
        show_legend = self._scale_legend is not None

        config, result = ImageTransformsDialog.get(
            transform, aspect_ratio, show_legend, parent=self)

        if not result:
            return

        self.plotItem.set_transform(x_scale=config["x_scale"],
                                    y_scale=config["y_scale"],
                                    x_translate=config["x_translate"],
                                    y_translate=config["y_translate"],
                                    aspect_ratio=config["aspect_ratio"])

        self._show_scale_legend(show=config["show_scale"])

        self._update_scale_legend()

        self.configuration.update(**config)
        self.stateChanged.emit(config)

    @Slot()
    def _set_roi_configuration(self):
        config = {}
        aux_plots_tool = self.toolbar.toolsets[AuxPlots].current_tool
        if aux_plots_tool is not None:
            config['aux_plots'] = aux_plots_tool

        config['roi_tool'] = self.roi.current_tool
        items = []
        # Save ROI information
        for tool, roi_items in self.roi.roi_items.items():
            # Each tool has multiple roi objects
            for roi in roi_items:
                traits = {'roi_type': tool, "name": roi.name}
                if tool == ROITool.Crosshair:
                    x, y = roi.coords
                    traits.update({'x': x, 'y': y})
                else:
                    x, y, w, h = roi.coords
                    traits.update({'x': x, 'y': y, 'w': w, 'h': h})
                items.append(traits)

        config['roi_items'] = items
        self.configuration.update(**config)
        self.stateChanged.emit(config)

    @Slot()
    def _set_axes_to_aux(self):
        self._aux_plots.set_axes(*self.plotItem.transformed_axes)

    @Slot()
    def _save_color_levels(self):
        image_item = self.plotItem.imageItem
        levels = []
        if not image_item.auto_levels:
            levels = image_item.levels.tolist()
        config = {"color_levels": levels}
        self.stateChanged.emit(config)

    # -----------------------------------------------------------------------
    # ROI methods

    @Slot(object)
    def _activate_roi_tool(self, roi_tool):
        if self._canvas is not None:
            self._deactivate_canvas()

        if roi_tool in [ROITool.NoROI, ROITool.Rect, ROITool.Crosshair]:
            self.roi.show(roi_tool)
        elif roi_tool in [ROITool.DrawRect, ROITool.DrawCrosshair]:
            # Hide shown ROIs to give way to the canvas
            self.roi.show(ROITool.NoROI)
            self._activate_canvas(roi_tool)

    def _activate_canvas(self, draw_tool):
        # Deactivate the canvas if a previous is applied
        if self._canvas is not None:
            self._deactivate_canvas()

        # Check if draw tool has an canvas counterpart
        if draw_tool not in ROI_CANVAS_MAP:
            return

        canvas_class, roi_tool = ROI_CANVAS_MAP[draw_tool]
        rect = self.plotItem.vb.viewRect()

        # Create and add canvas
        self._canvas = canvas_class(rect)
        self.plotItem.vb.addItem(self._canvas)
        self._canvas.editingFinished.connect(partial(self._draw_roi, roi_tool))

    @Slot(object)
    def _draw_roi(self, roi_tool, rect):
        self._deactivate_canvas()
        if rect.isValid():
            # Draw the ROI and show corresponding ROI tool
            new_rect = self.plotItem.mapRectFromTransform(rect)
            self.roi.add(roi_tool, new_rect.topLeft(), size=new_rect.size())
            self.roi.show(roi_tool)
            self.roi.selected.emit(roi_tool)
        else:
            # Do nothing and unselect the button
            self.roi.selected.emit(ROITool.NoROI)

    def _deactivate_canvas(self):
        self.plotItem.vb.removeItem(self._canvas)
        self._canvas.destroy()
        self._canvas = None

    # -----------------------------------------------------------------------
    # Scale legend

    def _show_scale_legend(self, show=True):
        if show:
            # Create the legend object
            if self._scale_legend is None:
                self._scale_legend = ScaleLegend()
                self._scale_legend.setParentItem(self.plotItem.vb)
                self._scale_legend.anchor(itemPos=(0, 1),
                                          parentPos=(0, 1),
                                          offset=(5, -5))
        else:
            # Destroy object
            if self._scale_legend is not None:
                self._scale_legend.setParentItem(None)
                self._scale_legend.deleteLater()
                self._scale_legend = None

    def _update_scale_legend(self):
        if self._scale_legend is not None:
            x_scale, y_scale = self.plotItem.axes_transform[TF_SCALING]
            x_units, y_units = (labels["units"] for labels in
                                self.plotItem.axes_labels)
            self._scale_legend.set_value(x_scale, y_scale, x_units, y_units)

    # -----------------------------------------------------------------------

    def event(self, event):
        if event.type() == QEvent.ToolTip:
            self.toolTipChanged.emit()
        return super().event(event)

    def sizeHint(self):
        """ The optimal size when all aux plots and labels are activated."""
        return QSize(610, 460)
