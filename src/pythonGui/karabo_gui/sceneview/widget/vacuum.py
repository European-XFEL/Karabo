from karabo_gui.widget import Widget
from .base import BaseWidgetContainer

_VACUUM_CLASS_NAMES = (
    'AgilentIonWidget', 'CryoCoolerWidget', 'HydraulicValveWidget',
    'LampSmallWidget', 'LampWidget', 'MaxiGaugeWidget', 'MembranePumpWidget',
    'MotorWidget', 'PressureGaugeWidget', 'PressureSwitchWidget',
    'RightAngleValveWidget', 'ShutOffValveWidget', 'TemperatureProbeWidget',
    'TurboPumpWidget', 'ValveWidget'
    )

_VACUUM_WIDGETS = {n: Widget.widgets[n] for n in _VACUUM_CLASS_NAMES}


class VacuumWidgetContainer(BaseWidgetContainer):
    """ A container for VacuumWidgets
    """
    def _create_widget(self, boxes):
        factory = _VACUUM_WIDGETS[self.model.klass]
        return factory(boxes[0], self)
