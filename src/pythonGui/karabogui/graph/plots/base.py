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

from pyqtgraph import GraphicsObject, GraphicsView, PlotItem, mkBrush, mkPen
from qtpy.QtCore import QSize, Signal, Slot
from qtpy.QtGui import QColor, QPalette
from qtpy.QtWidgets import QAction, QGridLayout, QSizePolicy, QWidget

from karabogui import icons
from karabogui.actions import KaraboAction, build_qaction
from karabogui.graph.common.api import (
    AxesLabelsDialog, AxisType, BaseROIController, ExportTool, ImageExporter,
    KaraboLegend, KaraboViewBox, MouseTool, PlotDataExporter, PointCanvas,
    ROITool, ToolbarController, create_axis_items, get_default_brush,
    get_default_pen, make_pen, safe_log10)
from karabogui.graph.common.const import (
    ACTION_ITEMS, AXIS_ITEMS, CHECK_ACTIONS, DEFAULT_BAR_WIDTH, DEFAULT_SYMBOL,
    EMPTY_SYMBOL_OPTIONS, SYMBOL_SIZE, WIDGET_HEIGHT_HINT, WIDGET_MIN_HEIGHT,
    WIDGET_MIN_WIDTH, WIDGET_WIDTH_HINT)
from karabogui.graph.plots.dialogs import GraphViewDialog, RangeDialog
from karabogui.graph.plots.items import ScatterGraphPlot, VectorBarGraphPlot
from karabogui.graph.plots.tools import CrossTargetController

ALPHA_GRID = 30 / 255
TICK_AXES = ["bottom", "left"]
ROI_CANVAS_MAP = {
    ROITool.DrawCrosshair: (PointCanvas, ROITool.Crosshair)}


class KaraboPlotView(QWidget):
    """ The main KaraboPlotView widget for ImagePlots

    This widget houses the plotItem and the toolbar for other tooling features
    """
    stateChanged = Signal(object)

    def __init__(self, axis=AxisType.Classic, actions=None, parent=None):
        super().__init__(parent)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setMinimumSize(WIDGET_MIN_WIDTH, WIDGET_MIN_HEIGHT)

        # Main layout to organize
        layout = QGridLayout(self)
        layout.setSpacing(0)
        layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(layout)

        # Initialize axis items
        axis_items = create_axis_items(axis, axes_with_ticks=TICK_AXES)

        # Our viewbox with reset range addition!
        viewbox = KaraboViewBox(parent=None)
        viewbox.menu.addAction("Reset Range", self.reset_range)

        # Add our basic plotItem to this widget
        self.plotItem = PlotItem(viewBox=viewbox, axisItems=axis_items,
                                 enableMenu=False, parent=None)

        # Improve axes rendering
        for axis in AXIS_ITEMS:
            axis_item = self.plotItem.getAxis(axis)
            axis_item.setZValue(0)

        self.graph_view = GraphicsView(parent=self)
        self.graph_view.setAntialiasing(False)
        self.graph_view.enableMouse(False)
        # Erase all transparent palettes (Performance fix: PyQtGraph == 0.11.1)
        self.graph_view.setPalette(QPalette())

        # row, col, row_span, col_span
        self.layout().addWidget(self.graph_view, 0, 0, 1, 1)
        # PlotItem gets added automatically!
        self.graph_view.setCentralItem(self.plotItem)

        self.show_all_axis()

        self.configuration = {}
        self.qactions = {}
        actions = actions or ACTION_ITEMS
        self.setup_qactions(actions)

        # Our tooling instances
        self._toolbar = None
        self._legend = None
        self._cross_target = None
        self._roi = None
        self._canvas = None

        self._show_symbols = False

        self._data_point_toggle_enabled = False
        self._show_legend_action = None

    def sizeHint(self):
        """Provide a size hint slightly larger than the minimum size"""
        return QSize(WIDGET_WIDTH_HINT, WIDGET_HEIGHT_HINT)

    def setup_qactions(self, actions):
        """The setup of the basic actions is gathered here!"""

        x_grid = KaraboAction(
            text="Grid X",
            tooltip="Toggle the grid of the X-Axis",
            triggered=self.set_grid_x,
            checkable=True,
            name='x_grid')

        y_grid = KaraboAction(
            text="Grid Y",
            tooltip="Toggle the grid of the Y-Axis",
            triggered=self.set_grid_y,
            checkable=True,
            name='y_grid')

        x_log = KaraboAction(
            text="Log X",
            tooltip="Toggle the logarithmic scale of the X-Axis",
            triggered=self.set_log_x,
            checkable=True,
            name='x_log')

        y_log = KaraboAction(
            text="Log Y",
            tooltip="Toggle the logarithmic scale of the Y-Axis",
            triggered=self.set_log_y,
            checkable=True,
            name='y_log')

        x_invert = KaraboAction(
            text="Invert X",
            tooltip="Invert the scale of the X-Axis",
            triggered=self.set_invert_x,
            checkable=True,
            name='x_invert')

        y_invert = KaraboAction(
            text="Invert Y",
            tooltip="Invert the scale of the Y-Axis",
            triggered=self.set_invert_y,
            checkable=True,
            name='y_invert')

        x_range = KaraboAction(
            text="Range X",
            tooltip="Configure the range of the X-Axis",
            triggered=self.configure_range_x,
            name='x_range')

        y_range = KaraboAction(
            text="Range Y",
            tooltip="Configure the range of the Y-Axis",
            triggered=self.configure_range_y,
            name='y_range')

        axes = KaraboAction(
            text="Axes",
            tooltip="Configure the axes labels and units",
            triggered=self.configure_axes,
            name='axes')

        view = KaraboAction(
            text="View",
            tooltip="Configure the background and title of the graph view",
            triggered=self.configure_view,
            name='view')

        for check_action in (x_grid, y_grid, x_log, y_log, x_invert, y_invert):
            if check_action.name in actions:
                q_ac = build_qaction(check_action, self)
                q_ac.triggered.connect(partial(self.check_action_callback,
                                               callback=check_action.triggered,
                                               name=check_action.name))
                self.qactions[check_action.name] = q_ac
                self.addAction(q_ac)

        viewbox = self.plotItem.vb
        for action in (x_log, y_log):
            if action.name in self.qactions:
                viewbox.add_action(self.qactions[action.name], separator=False)

        for k_action in (axes, x_range, y_range, view):
            if k_action.name in actions:
                q_ac = build_qaction(k_action, self)
                q_ac.triggered.connect(k_action.triggered)
                self.addAction(q_ac)

        if x_range.name in actions:
            axis_item = self.plotItem.getAxis('bottom')
            axis_item.axisDoubleClicked.connect(self.configure_range_x)

        if y_range.name in actions:
            axis_item = self.plotItem.getAxis('left')
            axis_item.axisDoubleClicked.connect(self.configure_range_y)

    # ----------------------------------------------------------------
    # Base Action Slots

    @Slot(object, object, object)
    def check_action_callback(self, state, callback=None, name=None):
        if callback is not None:
            callback(state)

        self.qactions[name].setChecked(state)
        self.configuration[name] = state
        self.stateChanged.emit({name: state})

    def set_grid_x(self, state):
        alpha_value = ALPHA_GRID if state else None
        self.plotItem.showGrid(x=state, alpha=alpha_value)

    def set_grid_y(self, state):
        alpha_value = ALPHA_GRID if state else None
        self.plotItem.showGrid(y=state, alpha=alpha_value)

    def set_log_x(self, state):
        self.plotItem.setLogMode(x=state)

    def set_log_y(self, state):
        self.plotItem.setLogMode(y=state)

    def set_invert_x(self, state):
        self.plotItem.invertX(state)

    def set_invert_y(self, state):
        self.plotItem.invertY(state)

    @Slot()
    def configure_axes(self):
        config, ok = AxesLabelsDialog.get(self.configuration, parent=self)
        if ok:
            self.set_label(axis=0, text=config['x_label'],
                           units=config['x_units'])
            self.set_label(axis=1, text=config['y_label'],
                           units=config['y_units'])
            self.configuration.update(**config)
            self.stateChanged.emit(config)

    @Slot()
    def configure_range_x(self):
        x_min, x_max = self.get_view_range_x()
        actual = {'x_min': x_min, 'x_max': x_max}
        config, ok = RangeDialog.get(self.configuration, actual, axis=0,
                                     parent=self)
        if ok:
            self.configuration.update(**config)
            x_autorange = config['x_autorange']
            if not x_autorange:
                self.set_range_x(*self.get_config_range_x())
            else:
                self.set_autorange_x(x_autorange)
            self.stateChanged.emit(config)

    @Slot()
    def configure_range_y(self):
        y_min, y_max = self.get_view_range_y()
        actual = {'y_min': y_min, 'y_max': y_max}
        config, ok = RangeDialog.get(self.configuration, actual, axis=1,
                                     parent=self)
        if ok:
            self.configuration.update(**config)
            autorange = config['y_autorange']
            if not autorange:
                self.set_range_y(*self.get_config_range_y())
            else:
                self.set_autorange_y(autorange)
            self.stateChanged.emit(config)

    @Slot()
    def configure_view(self):
        config, ok = GraphViewDialog.get(self.configuration, parent=self)
        if ok:
            self.set_title(config['title'])
            self.set_background(config['background'])
            self.configuration.update(**config)
            self.stateChanged.emit(config)

    @Slot()
    def reset_range(self):
        self.reset_range_x()
        self.reset_range_y()

    @Slot(bool)
    def toggle_data_symbols(self, show):
        """Toggle the data points on the plotItems"""
        self._show_symbols = show
        for item in self.plotItem.dataItems[:]:
            if show:
                pen = item.opts['pen']
                options = {'symbol': DEFAULT_SYMBOL,
                           'symbolSize': SYMBOL_SIZE,
                           'symbolPen': pen,
                           'symbolBrush': mkBrush(pen.color())}
            else:
                options = EMPTY_SYMBOL_OPTIONS
            # NOTE: We are directly setting the options dict to avoid
            # multiple updates!
            item.opts.update(options)
            item.updateItems()

    # ----------------------------------------------------------------
    # Base Functions

    def set_background(self, color):
        """Set the background color of the overall base plot widget"""
        self.graph_view.setBackground(QColor(color))
        if self._toolbar is not None:
            self._toolbar.set_background(QColor(color))

    def reset_range_x(self):
        config = self.configuration
        x_autorange = config['x_autorange']
        if not x_autorange:
            self.set_range_x(*self.get_config_range_x())
        else:
            self.set_autorange_x(x_autorange)

    def reset_range_y(self):
        config = self.configuration
        y_autorange = config['y_autorange']
        if not y_autorange:
            self.set_range_y(*self.get_config_range_y())
        else:
            self.set_autorange_y(y_autorange)

    def enable_data_toggle(self, activate=False):
        """Optionally enable the showing of data points"""
        toggle_action = QAction("Show data points", self)
        toggle_action.setCheckable(True)
        toggle_action.setChecked(False)
        toggle_action.toggled.connect(self.toggle_data_symbols)
        viewbox = self.plotItem.vb
        seperator = self._legend is None
        viewbox.add_action(toggle_action, separator=seperator)
        if activate:
            toggle_action.setChecked(True)
        self._data_point_toggle_enabled = True

    def enable_export(self):
        """Optionally enable the showing of data points"""
        # Add Export toolset
        if self._toolbar is not None:
            export_toolset = self._toolbar.add_toolset(ExportTool)
            export_toolset.on_trait_change(self.export, "current_tool")

    def restore(self, config):
        """Restore the widget configuration with a config dictionary"""
        self.configuration.update(**config)
        actions = [name for name in CHECK_ACTIONS if name in self.qactions]
        for name in actions:
            self.qactions[name].setChecked(self.configuration[name])

        # Set x and y grid at the same time to avoid overwritten settings
        self.plotItem.showGrid(config['x_grid'], config['y_grid'],
                               alpha=ALPHA_GRID)

        self.set_log_x(config['x_log'])
        self.set_log_y(config['y_log'])
        self.set_label(axis=0, text=config['x_label'], units=config['x_units'])
        self.set_label(axis=1, text=config['y_label'], units=config['y_units'])
        self.set_invert_x(config['x_invert'])
        self.set_invert_y(config['y_invert'])

        # Restore ROIs
        if self._roi is not None:
            self._roi.remove_all()
            roi_tool = config['roi_tool']
            roi_items = config.get('roi_items', [])
            for roi_data in roi_items:
                roi_type = roi_data['roi_type']
                name = roi_data['name']
                if roi_type == ROITool.Crosshair:
                    pos = (roi_data['x'], roi_data['y'])
                    size = None
                else:
                    continue

                self._roi.add(roi_type, pos, size=size, name=name)

            self._roi.show(roi_tool)

            # Make sure that we checked the tool!
            if self._toolbar is not None:
                if roi_tool != ROITool.NoROI:
                    tool = ROITool(roi_tool).name
                    self._toolbar.buttons[tool].setChecked(True)

        self.reset_range()

        # Set the view configuration! Some widgets, e.g. external extensions
        # might not define `background` and `title`.
        # Note: This was added in karabo 2.11!
        if 'background' in config:
            self.set_background(config['background'])
        if 'title' in config:
            self.set_title(config['title'])

    def apply_curve_options(self, options: dict):
        """Apply the plotting options to the corresponding curves"""
        for plot_item in self.plotItem.listDataItems():
            curve_opts = options.get(plot_item.name(), None)
            if curve_opts is None:
                continue
            # Safety copy
            curve_opts = curve_opts.copy()
            legend_name = curve_opts.get("legend_name", plot_item.name())
            pen_color = curve_opts.get("pen_color")
            pen = mkPen(pen_color)
            curve_opts["pen"] = pen
            opts = plot_item.opts
            opts.update(curve_opts)

            for sample, label in self._legend.items:
                if sample.item is plot_item:
                    sample.setBrush(mkBrush(pen.color()))
                    sample.setPen(pen)
                    label.setText(legend_name)
                    break

            self._legend.updateSize()
            plot_item.updateItems()

    # ----------------------------------------------------------------
    # Toolbar functions Events

    def add_cross_target(self):
        if self._cross_target is None:
            self._cross_target = CrossTargetController(self.plotItem)

        return self._cross_target

    def add_roi(self, enable=True):
        if enable and self._roi is None:
            # Initialize ROI controller
            self._roi = BaseROIController(self.plotItem)
            self._roi.set_pen(make_pen('k'))
            set_roi = KaraboAction(
                text="Set ROI",
                icon=icons.apply,
                tooltip="Set the ROI information",
                triggered=self._set_roi_configuration)
            q_ac = build_qaction(set_roi, self)
            q_ac.triggered.connect(set_roi.triggered)
            self.addAction(q_ac)

        elif not enable and self._roi is not None:
            # Remove ROI items and destroy controller
            self._roi.remove_all()
            self._roi.destroy()
            self._roi = None

        return self._roi

    @Slot()
    def _set_roi_configuration(self):
        """Set ROI information to the model"""
        items = []
        for tool, roi_items in self._roi.roi_items.items():
            # Each tool has multiple roi objects
            for roi in roi_items:
                traits = {'roi_type': tool, 'name': roi.name}
                if tool == ROITool.Crosshair:
                    x, y = roi.coords
                    traits.update({'x': x, 'y': y})
                    items.append(traits)
        config = {}
        config['roi_tool'] = self._roi.current_tool
        config['roi_items'] = items
        self.configuration.update(**config)
        self.stateChanged.emit(config)

    def add_toolbar(self):
        """Enable the standard toolbar on the widget

        This is the finalizer of the `KaraboPlotView` widget. All signals
        should be connected here!
        """
        if self._toolbar is not None:
            raise ValueError('Toolbar already added')

        # Instantiate toolbar, has a default mouse mode toolset
        self._toolbar = tb = ToolbarController(parent=self)
        tb.toolsets[MouseTool].on_trait_change(
            self.plotItem.vb.set_mouse_tool,
            "current_tool")

        # Add ROI toolset
        if self._roi is not None:
            crosshair_button = tb.add_tool(ROITool.Crosshair)
            crosshair_button.triggered.connect(self._activate_roi_tool)

            # Connect signals from the items
            self._roi.selected.connect(crosshair_button.check)
            uncheck_button = partial(crosshair_button.check, None)
            self._roi.removed.connect(uncheck_button)

        # Add Crosshair toolset
        if self._cross_target is not None:
            tb.add_button(self._cross_target.action_button)

        # row, col, row_span, col_span
        self.layout().addWidget(tb.widget, 0, 1, 1, 1)

        return self._toolbar

    @Slot(object)
    def export(self, export_type=ExportTool.Data):
        """Exports the data or image according to the desired format"""
        if export_type == ExportTool.Image:
            scene = self.plotItem.scene()
            exporter = ImageExporter(scene)
        elif export_type == ExportTool.Data:
            if not len(self.plotItem.dataItems):
                return
            exporter = PlotDataExporter(self.plotItem.dataItems)

        exporter.export()

    # ----------------------------------------------------------------
    # PlotItem methods

    def clear(self):
        """Remove all the items in the PlotItem object"""
        plot_item = self.plotItem
        for item in plot_item.items[:]:
            plot_item.removeItem(item)

    def clearData(self):
        plot_item = self.plotItem
        for item in plot_item.dataItems[:]:
            item.clear()

    def add_curve_fill(self, name=None, pen=get_default_pen(),
                       brush=get_default_brush()):
        item = self.plotItem.plot(name=name, pen=pen, fillLevel=0,
                                  fillBrush=brush)
        if hasattr(item, 'setDynamicRangeLimit'):
            item.setDynamicRangeLimit(None)
        return item

    def add_curve_item(self, name=None, pen=get_default_pen(), **options):
        """Add a plot to the built-in plotItem from PyQtGraph

        :param name: Set a name to automatically provide a legend

        :param pen: Pen to use for drawing line between points.
                    Default is solid grey, 1px width. Use None to disable line
                    drawing.
        """
        if self._show_symbols:
            # If a curve is dynamically added, we still have to show points
            # when desired!
            options.update({'symbol': DEFAULT_SYMBOL,
                            'symbolSize': SYMBOL_SIZE,
                            'symbolPen': pen,
                            'symbolBrush': mkBrush(pen.color())})

        item = self.plotItem.plot(name=name, pen=pen, **options)
        if hasattr(item, 'setDynamicRangeLimit'):
            # DynamicRangeLimit was introduced in pyqtgraph 0.11.1 and
            # contains a lot of bugs, e.g. curve won't show initial values
            # until pyqtgraph >= 0.12.1
            # Setting it to `None` disables it!
            item.setDynamicRangeLimit(None)
        return item

    def add_scatter_item(self, pen=mkPen(None), cycle=True, **options):
        """Add a scatter item to the built-in plotItem

        :param pen: pen used for the scatter plot (default: None)
        :param cycle: Defines whether the latest brush color is different

        Optionally as keyword arguments:

        :parem size: The size (or list of sizes) of spots.
        """
        item = ScatterGraphPlot(pen=pen, cycle=cycle, **options)
        self.plotItem.addItem(item)

        return item

    def add_bar_item(self, width=DEFAULT_BAR_WIDTH, brush=get_default_brush()):
        """Add a bar item to the built-in plotItem

        :param width: width used for the bar plot
        :param brush: brush to be used when plotting (default: fancy blue)
        """
        item = VectorBarGraphPlot(width, brush)
        self.plotItem.addItem(item)

        return item

    def remove_item(self, item):
        """Remove any plot item to the built-in plotItem"""
        self.plotItem.removeItem(item)

    def set_aspect_locked(self, enable, ratio=1):
        self.plotItem.setAspectLocked(enable, ratio)

    # ----------------------------------------------------------------
    # Legend methods

    def add_legend(self, size=None, offset=(5, 5), visible=True):
        """Create a new LegendItem and anchor it over the internal ViewBox.

        Plots will be automatically displayed in the legend if they
        are created with the 'name' argument.

        :param size: the size of the legend (default: None)
        :param offset: the offset with default (30, 30)
        :param visible: Boolean to set whether this Legend should be visible
        """
        if self._legend is not None:
            return

        self._legend = KaraboLegend(size, offset)
        self._legend.setBrush(None)

        self.plotItem.legend = self._legend
        self._legend.setParentItem(self.plotItem.vb)
        if not visible:
            self.plotItem.legend.setVisible(False)

        # Add a menu-item to show/hide the legend
        show_legend_action = QAction("Show Legend", parent=self)
        show_legend_action.setCheckable(True)
        show_legend_action.setChecked(False)
        show_legend_action.triggered.connect(self._set_legend)
        seperator = not self._data_point_toggle_enabled
        self.plotItem.vb.add_action(show_legend_action, separator=seperator)
        self._show_legend_action = show_legend_action
        return self._legend

    def set_legend(self, visible: bool):
        """Set the legend item visible or invisible"""
        self._set_legend(visible)
        self._toggle_legend_menu(visible)

    @Slot(bool)
    def _set_legend(self, visible: bool):
        if self._legend is None:
            return
        self.plotItem.legend.setVisible(visible)

    def _toggle_legend_menu(self, toggled: bool):
        self._show_legend_action.setChecked(toggled)

    # ----------------------------------------------------------------
    # Axis and Title methods

    def set_label(self, axis, text="", units=""):
        """Set the label of an axis of the built-in plotItem

        :param axis: The axis, e.g. ``0`` for x-axis, ``1`` for y-axis
        :param text: The text used for the label (None)
        :param units: The declared unit string for the axis label (None)
        """
        self.plotItem.setLabel(TICK_AXES[axis], text, units)
        show = True if (text != "" or units != "") else False
        # XXX: We circumvent a PyQtGraph bug here and set the visibility
        # ourselves! Setting a label and removing it, gives PyQtGraph problems.
        self.plotItem.showLabel(TICK_AXES[axis], show=show)

    def set_title(self, title=None, **kwargs):
        """Set the title of the built-in plotItem"""
        title = f"<b>{title}</b>" if title else None
        self.plotItem.setTitle(title, **kwargs)

    def hide_all_axis(self):
        """Hide all axis of the widget"""
        for axis in AXIS_ITEMS:
            self.plotItem.hideAxis(axis)

    def show_all_axis(self):
        """Show all axis of the widget"""
        for axis in AXIS_ITEMS:
            self.plotItem.showAxis(axis)

    # ----------------------------------------------------------------
    # Range methods

    def set_autorange(self, enable):
        """Enable or disable the auto range for a specific axis"""
        if enable:
            self.plotItem.vb.enableAutoRange()
        else:
            self.plotItem.vb.disableAutoRange()

    def set_autorange_x(self, enable):
        """Enable or disable the auto range for a specific axis"""
        self.plotItem.vb.enableAutoRange(axis=0, enable=enable)

    def set_autorange_y(self, enable):
        """Enable or disable the auto range for a specific axis"""
        self.plotItem.vb.enableAutoRange(axis=1, enable=enable)

    def set_range_x(self, min_value, max_value):
        """Set the X Range of the view box with min and max value"""
        self.plotItem.vb.setRange(xRange=[min_value, max_value],
                                  disableAutoRange=True)

    def set_range_y(self, min_value, max_value):
        """Set the Y Range of the view box with min and max value"""
        self.plotItem.vb.setRange(yRange=[min_value, max_value],
                                  disableAutoRange=True)

    def get_view_range_x(self):
        view_range = self.plotItem.vb.viewRange()
        x_min, x_max = view_range[0]

        # Revert viewbox values (log scale) to linear scale
        if self.configuration['x_log']:
            x_min, x_max = 10 ** x_min, 10 ** x_max
        return x_min, x_max

    def get_view_range_y(self):
        view_range = self.plotItem.vb.viewRange()
        y_min, y_max = view_range[1]

        # Revert viewbox values (log scale) to linear scale
        if self.configuration['y_log']:
            y_min, y_max = 10 ** y_min, 10 ** y_max
        return y_min, y_max

    def get_config_range_x(self):
        """Returns the x-range from the configuration in viewbox coordinates
           (linear or log scale)"""
        config = self.configuration
        x_min, x_max = config['x_min'], config['x_max']
        # Revert input values (linear scale) to log scale if enabled
        if config['x_log']:
            x_min, x_max = safe_log10(x_min), safe_log10(x_max)
        return x_min, x_max

    def get_config_range_y(self):
        """Returns the y-range from the configuration in viewbox coordinates
           (linear or log scale)"""
        config = self.configuration
        y_min, y_max = config['y_min'], config['y_max']
        # Revert input values (linear scale) to log scale if enabled
        if config['y_log']:
            y_min, y_max = safe_log10(y_min), safe_log10(y_max)
        return y_min, y_max

    def update_legend_text(self, plot_item: GraphicsObject, text: str):
        """Set the label text for the  plot item's legend"""
        label = self._legend.getLabel(plot_item)
        if label is not None:
            label.setText(text)
            self._legend.updateSize()

    # -----------------------------------------------------------------------
    # ROI methods

    @Slot(object)
    def _activate_roi_tool(self, tool):
        if tool is None:
            tool = ROITool.NoROI

        if self._canvas is not None:
            self._deactivate_canvas()

        if tool in [ROITool.NoROI, ROITool.Crosshair]:
            self._roi.show(tool)
        elif tool is ROITool.DrawCrosshair:
            # Hide shown ROIs to give way to the canvas
            self._roi.show(ROITool.NoROI)
            self._activate_canvas(tool)

    def _activate_canvas(self, draw_tool):
        # Deactivate the canvas if a previous is applied
        if self._canvas is not None:
            self._deactivate_canvas()

        # Check if draw tool has an canvas counterpart
        if draw_tool not in ROI_CANVAS_MAP:
            return

        canvas_class, roi_tool = ROI_CANVAS_MAP[draw_tool]
        viewbox_rect = self.plotItem.vb.viewRect()

        # Create and add canvas
        self._canvas = canvas_class(viewbox_rect)
        self.plotItem.vb.addItem(self._canvas, ignoreBounds=True)
        self._canvas.editingFinished.connect(partial(self._draw_roi, roi_tool))

    @Slot(object)
    def _draw_roi(self, roi_tool, rect):
        self._deactivate_canvas()
        if rect.isValid():
            # Draw the ROI and show corresponding ROI tool
            self._roi.add(roi_tool, pos=rect.topLeft())
            self._roi.show(roi_tool)
            self._roi.selected.emit(roi_tool)
        else:
            # Do nothing and unselect the button
            self._roi.selected.emit(ROITool.NoROI)

    def _deactivate_canvas(self):
        self.plotItem.vb.removeItem(self._canvas)
        self._canvas.destroy()
        self._canvas = None
