from karabo.middlelayer import SchemaHashType
from karabo_gui.displaywidgets.displaytableelement import DisplayTableElement
from karabo_gui.editablewidgets.editabletableelement import EditableTableElement  # noqa
from .base import BaseWidgetContainer


class _TableElementWrapperMixin(object):
    def __init__(self, model, box, parent):
        super(_TableElementWrapperMixin, self).__init__(box, parent)
        self.model = model

        # Initialize the widget
        schema = SchemaHashType.fromstring(self.model.column_schema)
        super(_TableElementWrapperMixin, self)._setColumnSchema(schema)

    def _setColumnSchema(self, schema):
        super(_TableElementWrapperMixin, self)._setColumnSchema(schema)
        self.model.column_schema = SchemaHashType.toString(schema)


class _DisplayTableWrapper(_TableElementWrapperMixin, DisplayTableElement):
    """ A wrapper around DisplayTableElement
    """


class _EditableTableWrapper(_TableElementWrapperMixin, EditableTableElement):
    """ A wrapper around EditableTableElement
    """


class TableElementContainer(BaseWidgetContainer):
    def _create_widget(self, boxes):
        factories = {
            'DisplayTableElement': _DisplayTableWrapper,
            'EditableTableElement': _EditableTableWrapper,
        }
        factory = factories[self.model.klass]
        return factory(self.model, boxes[0], self)
