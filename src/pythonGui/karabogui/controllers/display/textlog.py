#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on October 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from datetime import datetime

from PyQt4.QtCore import Qt
from PyQt4.QtGui import (
    QFrame, QHBoxLayout, QPushButton, QVBoxLayout, QTextEdit)
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import DisplayTextLogModel
from karabogui import icons
from karabogui.binding.api import (
    BaseBindingController, StringBinding, register_binding_controller
)
from karabogui.const import FINE_COLOR
from karabogui.util import generateObjectName

W_SIZE = 32


@register_binding_controller(ui_name='Text Log', read_only=True,
                             binding_type=StringBinding)
class DisplayTextLog(BaseBindingController):
    # The scene model class for this controller
    model = Instance(DisplayTextLogModel)
    # Internal traits
    log_widget = Instance(QTextEdit)

    def create_widget(self, parent):
        widget = QFrame(parent)
        ver_layout = QVBoxLayout(widget)
        self.log_widget = QTextEdit(widget)

        # no focus for this widget!
        self.log_widget.setFocusPolicy(Qt.NoFocus)
        ver_layout.addWidget(self.log_widget)

        # start horizontal layout with button and spacer
        hor_layout = QHBoxLayout()

        # strech is required to move the button to the right
        hor_layout.addStretch(1)

        button = QPushButton()
        button.setFixedSize(W_SIZE, W_SIZE)
        button.setFocusPolicy(Qt.NoFocus)
        button.setToolTip('Clear log')
        button.setIcon(icons.editClear)
        button.clicked.connect(self.log_widget.clear)
        hor_layout.addWidget(button)

        # spacer item to align button to the right!

        ver_layout.addLayout(hor_layout)

        # nice color background
        objectName = generateObjectName(widget)
        sheet = ('QWidget#{} {{ background-color : rgba{}; }}'
                 ''.format(objectName, FINE_COLOR))
        self.log_widget.setObjectName(objectName)
        self.log_widget.setStyleSheet(sheet)
        return widget

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        # catch both None and empty strings
        if value:
            self._write_log(value, self.proxy.binding.timestamp)

    def _write_log(self, text, timestamp):
        dt = datetime.fromtimestamp(timestamp.toTimestamp())
        stamp = dt.strftime('[%H:%M:%S]')
        item = '{}: {}'.format(stamp, text)
        self.log_widget.append(item)

        # update our scroll bar
        bar = self.log_widget.verticalScrollBar()
        bar.setValue(bar.maximum())
