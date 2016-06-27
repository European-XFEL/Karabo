from karabo_gui.widget import Widget
from .base import BaseWidgetContainer

_VACUUM_CLASS_NAMES = (
    'CryoCoolerWidget', 'HydraulicValveWidget', 'MaxiGaugeWidget',
    'MembranePumpWidget', 'MotorWidget', 'PressureGaugeWidget',
    'PressureSwitchWidget', 'RightAngleValveWidget', 'ShutOffValveWidget',
    'TemperatureProbeWidget', 'TurboPumpWidget', 'ValveWidget')
_VACUUM_WIDGETS = {n: Widget.widgets[n] for n in _VACUUM_CLASS_NAMES}


class VacuumWidgetContainer(BaseWidgetContainer):
    """ A container for VacuumWidgets
    """
    def _create_widget(self, boxes):
        factory = _VACUUM_WIDGETS[self.model.klass]
        display_widget = factory(boxes[0], self)
        return display_widget.widget
