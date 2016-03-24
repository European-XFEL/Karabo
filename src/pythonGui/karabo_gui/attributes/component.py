from PyQt4.QtGui import QHBoxLayout, QWidget

from karabo_gui.components import BaseComponent


class EditAttributeComponent(BaseComponent):
    """ This component is used for the attributes of a property.
    """
    def __init__(self, widgetClass, box, attributeName, parent):
        super(EditAttributeComponent, self).__init__(parent)

        self.__compositeWidget = QWidget(parent)
        hLayout = QHBoxLayout(self.__compositeWidget)
        hLayout.setContentsMargins(0, 0, 0, 0)

        self.widgetFactory = widgetClass(box, attributeName,
                                         parent=self.__compositeWidget)

        self._attributeName = attributeName
        self.connectWidget(box)
        hLayout.addWidget(self.widgetFactory.widget)

    def connectWidget(self, box):
        self.widgetFactory.setParent(self)

        if box.hasValue():
            value = getattr(box.descriptor, self._attributeName)
            self.widgetFactory.attributeValueChanged(value)

        self.widgetFactory.signalEditingFinished.connect(self.onEditingFinished)

    def onEditingFinished(self, box, value):
        setattr(box.descriptor, self._attributeName, value)

        # Configuration changed - so project needs to be informed to show it
        if box.configuration.type in ('projectClass', 'deviceGroupClass'):
            box.configuration.signalConfigurationModified.emit(True)

    def setEnabled(self, enable):
        self.widget.setEnabled(enable)

    @property
    def value(self):
        return self.widgetFactory.value

    @property
    def widget(self):
        return self.__compositeWidget


