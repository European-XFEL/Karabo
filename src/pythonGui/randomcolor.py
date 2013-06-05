#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on August 16, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which offers the management of a randomized QColor.
"""

__all__ = ["RandomColor"]


from random import randint


from PyQt4.QtGui import QColor


class RandomColor(QColor):


    def __init__(self):
        super(RandomColor, self).__init__()

        index = randint(0, 20)
        if index is 0: # limegreen
            self.setRgb(50, 205, 50)
        elif index is 1: # blue
            self.setRgb(0, 0, 255)
        elif index is 2: # blueviolet
            self.setRgb(138, 43, 226)
        elif index is 3: # brown
            self.setRgb(165, 42, 42)
        elif index is 4: # cadetblue
            self.setRgb(95, 158, 160)
        elif index is 5: # chocolate
            self.setRgb(210, 105, 30)
        elif index is 6: # coral
            self.setRgb(255, 127, 80)
        elif index is 7: # indianred
            self.setRgb(205, 92, 92)
        elif index is 8: # darkgoldenrod
            self.setRgb(184, 134, 11)
        elif index is 9: # darkgreen
            self.setRgb(0, 100, 0)
        elif index is 10: # darkorange
            self.setRgb(255, 140, 0)
        elif index is 11: # darkorchid
            self.setRgb(153, 50, 204)
        elif index is 12: # darkred
            self.setRgb(139, 0, 0)
        elif index is 13: # darksalmon
            self.setRgb(233, 150, 122)
        elif index is 14: # darkseagreen
            self.setRgb(143, 188, 143)
        elif index is 15: # mediumvioletred
            self.setRgb(199, 21, 133)
        elif index is 16: # darkslategray
            self.setRgb(47, 79, 79)
        elif index is 17: # darkslategrey
            self.setRgb(47, 79, 79)
        elif index is 18: # darkturquoise
            self.setRgb(0, 206, 209)
        elif index is 19: # darkviolet
            self.setRgb(148, 0, 211)

