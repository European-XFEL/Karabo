from functools import partial

from PyQt5.QtCore import pyqtSignal, pyqtSlot, Qt
from PyQt5.QtWidgets import QAction, QGridLayout, QSizePolicy, QWidget
from pyqtgraph import GraphicsView, mkPen, PlotItem

from karabogui.actions import build_qaction, KaraboAction
from karabogui import icons

from karabogui.graph.common.api import (
    AxesLabelsDialog, AxisType, BaseROIController, create_axis_items,
    ExportTool, ExportToolset, get_default_brush, get_default_pen, make_pen,
    MouseMode, KaraboLegend, KaraboToolBar, KaraboViewBox, PlotDataExporter,
    PointCanvas, ROITool, ROIToolset)
from karabogui.graph.common.const import (
    AXIS_ITEMS, ACTION_ITEMS, CHECK_ACTIONS, DEFAULT_BAR_WIDTH,
    EMPTY_SYMBOL_OPTIONS, DEFAULT_SYMBOL, SYMBOL_SIZE, WIDGET_MIN_HEIGHT,
    WIDGET_MIN_WIDTH)

from karabogui.graph.plots.dialogs import RangeDialog
from karabogui.graph.plots.items import (
    ScatterGraphPlot, VectorBarGraphPlot, VectorFillGraphPlot)
from karabogui.graph.plots.tools import CrossTargetController

ALPHA_GRID = 30 / 255
TICK_AXES = ["bottom", "left"]


class KaraboPlotView(QWidget):
    """ The main KaraboPlotView widget for ImagePlots

    This widget houses the plotItem and the toolbar for other tooling features
    """
    stateChanged = pyqtSignal(object)

    def __init__(self, axis=AxisType.Classic, actions=None, parent=None):
        super(KaraboPlotView, self).__init__(parent)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setMinimumSize(WIDGET_MIN_WIDTH, WIDGET_MIN_HEIGHT)

        # Main layout to organize
        layout = QGridLayout()
        self.setLayout(layout)

        # Initialize axis items
        axis_items = create_axis_items(axis, axes_with_ticks=TICK_AXES)

        # Our viewbox with reset range addition!
        viewbox = KaraboViewBox()
        viewbox.menu.addAction("Reset Range", self.reset_range)

        # Add our basic plotItem to this widget
        self.plotItem = PlotItem(viewBox=viewbox, axisItems=axis_items)

        # Improve axes rendering
        for axis in AXIS_ITEMS:
            axis_item = self.plotItem.getAxis(axis)
            axis_item.setZValue(0)

        self.graph_view = GraphicsView()
        self.graph_view.setAntialiasing(False)
        self.graph_view.enableMouse(False)

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

        for check_action in (x_grid, y_grid, x_log, y_log, x_invert, y_invert):
            if check_action.name in actions:
                q_ac = build_qaction(check_action, self)
                q_ac.triggered.connect(partial(self.check_action_callback,
                                               callback=check_action.triggered,
                                               name=check_action.name))
                self.qactions[check_action.name] = q_ac
                self.addAction(q_ac)

        for k_action in (axes, x_range, y_range):
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

    @pyqtSlot(object, object, object)
    def check_action_callback(self, state, callback=None, name=None):
        if callback is not None:
            callback(state)
        self.qactions[name].setChecked(state)
        self.configuration[name] = state
        self.stateChanged.emit({name: state})

    def set_grid_x(self, state):
        alpha_value = ALPHA_GRID if state else False
        self.plotItem.showGrid(x=state, alpha=alpha_value)

    def set_grid_y(self, state):
        alpha_value = ALPHA_GRID if state else False
        self.plotItem.showGrid(y=state, alpha=alpha_value)

    def set_log_x(self, state):
        self.plotItem.setLogMode(x=state)

    def set_log_y(self, state):
        self.plotItem.setLogMode(y=state)

    def set_invert_x(self, state):
        self.plotItem.invertX(state)

    def set_invert_y(self, state):
        self.plotItem.invertY(state)

    @pyqtSlot()
    def configure_axes(self):
        config, ok = AxesLabelsDialog.get(self.configuration, parent=self)
        if ok:
            self.set_label(axis=0, text=config['x_label'],
                           units=config['x_units'])
            self.set_label(axis=1, text=config['y_label'],
                           units=config['y_units'])
            self.configuration.update(**config)
            self.stateChanged.emit(config)

    @pyqtSlot()
    def configure_range_x(self):
        view_range = self.plotItem.vb.state['viewRange']
        actual = {
            'x_min': view_range[0][0],
            'x_max': view_range[0][1]}

        config, ok = RangeDialog.get(self.configuration, actual, axis=0,
                                     parent=self)
        if ok:
            self.configuration.update(**config)
            x_autorange = config['x_autorange']
            if not x_autorange:
                self.set_range_x(config['x_min'], config['x_max'])
            else:
                self.set_autorange_x(x_autorange)
            self.stateChanged.emit(config)

    @pyqtSlot()
    def configure_range_y(self):
        view_range = self.plotItem.vb.state['viewRange']
        actual = {
            'y_min': view_range[1][0],
            'y_max': view_range[1][1]}

        config, ok = RangeDialog.get(self.configuration, actual, axis=1,
                                     parent=self)
        if ok:
            self.configuration.update(**config)
            autorange = config['y_autorange']
            if not autorange:
                self.set_range_y(config['y_min'], config['y_max'])
            else:
                self.set_autorange_y(autorange)
            self.stateChanged.emit(config)

    @pyqtSlot()
    def reset_range(self):
        self.reset_range_x(config=self.configuration)
        self.reset_range_y(config=self.configuration)

    @pyqtSlot(bool)
    def toggle_data_symbols(self, show):
        """Toggle the data points on the plotItems"""
        self._show_symbols = show
        for item in self.plotItem.dataItems[:]:
            if show:
                options = {'symbol': DEFAULT_SYMBOL,
                           'symbolSize': SYMBOL_SIZE,
                           'symbolPen': item.opts['pen'],
                           'symbolBrush': None}
            else:
                options = EMPTY_SYMBOL_OPTIONS
            # NOTE: We are directly setting the options dict to avoid
            # multiple updates!
            item.opts.update(options)
            item.updateItems()

    # ----------------------------------------------------------------
    # Base Functions

    def reset_range_x(self, config):
        x_autorange = config['x_autorange']
        if not x_autorange:
            self.set_range_x(config['x_min'], config['x_max'])
        else:
            self.set_autorange_x(x_autorange)

    def reset_range_y(self, config):
        y_autorange = config['y_autorange']
        if not y_autorange:
            self.set_range_y(config['y_min'], config['y_max'])
        else:
            self.set_autorange_y(y_autorange)

    def enable_data_toggle(self, activate=False):
        """Optionally enable the showing of data points"""
        toggle_action = QAction("Show data points", self)
        toggle_action.setCheckable(True)
        toggle_action.setChecked(False)
        toggle_action.toggled.connect(self.toggle_data_symbols)
        viewbox = self.plotItem.vb
        viewbox.add_action(toggle_action)
        if activate:
            toggle_action.setChecked(True)

    def enable_export(self):
        """Optionally enable the showing of data points"""
        # Add Export toolset
        if self._toolbar is not None:
            self._toolbar.addSeparator()
            export_toolset = self._toolbar.add_toolset(ExportToolset,
                                                       tools=[ExportTool.Data])
            export_toolset.clicked.connect(self.export)

    def restore(self, config):
        """Restore the widget configuration with a config dictionary"""
        self.configuration.update(**config)
        actions = [name for name in CHECK_ACTIONS if name in self.qactions]
        for name in actions:
            self.qactions[name].setChecked(self.configuration[name])

        self.set_grid_x(config['x_grid'])
        self.set_grid_y(config['y_grid'])
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
                if roi_type == ROITool.Crosshair:
                    pos = (roi_data['x'], roi_data['y'])
                    size = None
                else:
                    continue

                self._roi.add(roi_type, pos, size=size)

            self._roi.show(roi_tool)

            # Make sure that we checked the tool!
            if self._toolbar is not None:
                if roi_tool != ROITool.NoROI:
                    toolset = self._toolbar.toolset[ROITool]
                    toolset.buttons[roi_tool].setChecked(True)

        self.reset_range_x(config=config)
        self.reset_range_y(config=config)

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

    @pyqtSlot()
    def _set_roi_configuration(self):
        """Set ROI information to the model"""
        items = []
        for tool, roi_items in self._roi.roi_items.items():
            # Each tool has multiple roi objects
            for roi in roi_items:
                traits = {'roi_type': tool}
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
        self._toolbar = tb = KaraboToolBar(orientation=Qt.Vertical,
                                           parent=self)
        tb.toolset[MouseMode].clicked.connect(self.plotItem.vb.set_mouse_mode)

        # Add ROI toolset
        if self._roi is not None:
            roi_toolset = tb.add_toolset(ROIToolset, tools=[ROITool.Crosshair])
            roi_toolset.clicked.connect(self._activate_roi_tool)
            # Connect signals from the items
            self._roi.selected.connect(roi_toolset.check_button)
            self._roi.removed.connect(roi_toolset.uncheck_all)

        # Add Crosshair toolset
        if self._cross_target is not None:
            tb.add_button("cross_target", self._cross_target.action_button)

        # row, col, row_span, col_span
        self.layout().addWidget(tb, 0, 1, 1, 1)

        return self._toolbar

    @pyqtSlot(object)
    def export(self, export_type=ExportTool.Data):
        """Exports the image according to the desired format"""
        if export_type != ExportTool.Data:
            raise LookupError("No exporter found for {}".format(export_type))

        if len(self.plotItem.dataItems) == 0:
            return

        PlotDataExporter(self.plotItem.dataItems).export()

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

    def add_curve_fill(self, pen=get_default_pen(), brush=get_default_brush()):
        """Adds two curves which are filled inbetween"""
        item = VectorFillGraphPlot(
            viewbox=self.plotItem.vb, pen=pen, brush=brush)

        self.plotItem.addItem(item)

        return item

    def add_curve_item(self, name=None, pen=get_default_pen(),
                       brush=get_default_brush(), **options):
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
                            'symbolPen': pen, 'symbolBrush': None})

        return self.plotItem.plot(name=name, pen=pen, brush=brush, **options)

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
        self.plotItem.legend = self._legend
        self._legend.setParentItem(self.plotItem.vb)
        if not visible:
            self.plotItem.legend.setVisible(False)

        return self._legend

    def set_legend(self, visible):
        """Set the legend item visible or invisible"""
        if self._legend is None:
            return

        self.plotItem.legend.setVisible(visible)

    # ----------------------------------------------------------------
    # Axis and Title methods

    def set_label(self, axis, text=None, units=None):
        """Set the label of an axis of the built-in plotItem

        :param axis: The axis, e.g. ``0`` for x-axis, ``1`` for y-axis
        :param text: The text used for the label (None)
        :param units: The declared unit string for the axis label (None)
        """
        self.plotItem.setLabel(TICK_AXES[axis], text, units)

    def set_title(self, title=None, **kwargs):
        """Set the title of the built-in plotItem"""
        # XXX: Size
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
        self.plotItem.vb.setRange(xRange=[min_value, max_value])

    def set_range_y(self, min_value, max_value):
        """Set the Y Range of the view box with min and max value"""
        self.plotItem.vb.setRange(yRange=[min_value, max_value])

    # -----------------------------------------------------------------------
    # ROI methods

    @pyqtSlot(object)
    def _activate_roi_tool(self, roi_tool):
        if self._canvas is not None:
            self._deactivate_canvas()

        if roi_tool in [ROITool.NoROI, ROITool.Crosshair]:
            self._roi.show(roi_tool)
        elif roi_tool is ROITool.DrawCrosshair:
            # Hide shown ROIs to give way to the canvas
            self._roi.show(ROITool.NoROI)
            self._activate_canvas(roi_tool)

    def _activate_canvas(self, draw_tool):
        roi_canvas_map = {
            ROITool.DrawCrosshair: (PointCanvas, ROITool.Crosshair)}

        if draw_tool not in roi_canvas_map:
            return

        canvas_class, roi_tool = roi_canvas_map[draw_tool]

        viewbox_rect = self.plotItem.vb.viewRect()
        self._canvas = canvas_class(viewbox_rect)
        self.plotItem.vb.addItem(self._canvas, ignoreBounds=True)
        self._canvas.editingFinished.connect(partial(self._draw_roi, roi_tool))

    @pyqtSlot(object)
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
