from .enumeditwidget import MetricPrefixAttributeEditor, UnitAttributeEditor
from .numbereditwidget import NumberAttributeEditor


EDITABLE_ATTRIBUTE_NAMES = (
    'minExc', 'maxExc', 'minInc', 'maxInc', 'absoluteError', 'relativeError',
    'warnLow', 'warnHigh', 'alarmLow', 'alarmHigh', 'metricPrefixSymbol',
    'unitSymbol'
)

ATTRIBUTE_EDITOR_FACTORIES = {name: NumberAttributeEditor
                              for name in EDITABLE_ATTRIBUTE_NAMES}
ATTRIBUTE_EDITOR_FACTORIES['metricPrefixSymbol'] = MetricPrefixAttributeEditor
ATTRIBUTE_EDITOR_FACTORIES['unitSymbol'] = UnitAttributeEditor
