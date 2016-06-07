#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtGui import QLabel


class LabelWidget(QLabel):
    """ A label which can appear in a scene
    """

    def __init__(self, model, parent=None):
        super(LabelWidget, self).__init__(model.text, parent)

        self.setGeometry(model.x, model.y, model.width, model.height)
        self.setLineWidth(model.frame_width)

        styleSheet = []
        styleSheet.append('qproperty-font: "{}";'.format(model.font))
        styleSheet.append('color: "{}";'.format(model.foreground))
        styleSheet.append('background-color: "{}";'.format(model.background))
        self.setStyleSheet("".join(styleSheet))
