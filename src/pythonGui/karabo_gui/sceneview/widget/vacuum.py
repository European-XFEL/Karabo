from karabo_gui.vacuumwidgets.cryocoolerwidget import CryoCoolerWidget
from karabo_gui.vacuumwidgets.hydraulicvalvewidget import HydraulicValveWidget
from karabo_gui.vacuumwidgets.maxigaugewidget import MaxiGaugeWidget
from karabo_gui.vacuumwidgets.membranepumpwidget import MembranePumpWidget
from karabo_gui.vacuumwidgets.motorwidget import MotorWidget
from karabo_gui.vacuumwidgets.pressuregaugewidget import PressureGaugeWidget
from karabo_gui.vacuumwidgets.pressureswitchwidget import PressureSwitchWidget
from karabo_gui.vacuumwidgets.rightanglevalvewidget import RightAngleValveWidget  # noqa
from karabo_gui.vacuumwidgets.shutoffvalvewidget import ShutOffValveWidget
from karabo_gui.vacuumwidgets.temperatureprobeswidget import TemperatureProbeWidget  # noqa
from karabo_gui.vacuumwidgets.turbopumpwidget import TurboPumpWidget
from karabo_gui.vacuumwidgets.valvewidget import ValveWidget
from .base import BaseWidgetContainer

_VACUUM_CLASSES = (
    CryoCoolerWidget, HydraulicValveWidget, MaxiGaugeWidget,
    MembranePumpWidget, MotorWidget, PressureGaugeWidget, PressureSwitchWidget,
    RightAngleValveWidget, ShutOffValveWidget, TemperatureProbeWidget,
    TurboPumpWidget, ValveWidget)
_VACUUM_WIDGETS = {c.__class__.__name__: c for c in _VACUUM_CLASSES}


class VacuumWidgetContainer(BaseWidgetContainer):
    """ A container for VacuumWidgets
    """
    def _create_widget(self, boxes):
        factory = _VACUUM_WIDGETS[self.model.klass]
        display_widget = factory(boxes[0], self)
        return display_widget.widget
