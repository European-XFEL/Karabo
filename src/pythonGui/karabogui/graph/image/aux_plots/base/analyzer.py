# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from abc import abstractmethod

from traits.api import ABCHasStrictTraits, Dict, Property


class BaseAnalyzer(ABCHasStrictTraits):

    # A dictionary that contains stats. Override if `depends_on` is wanted
    stats = Property(Dict)

    @abstractmethod
    def _get_stats(self):
        """Reimplement to perform statistics specific to the analysis"""

    @abstractmethod
    def analyze(self, region, **config):
        """Reimplement to perform analysis in the input image region"""

    @abstractmethod
    def clear_data(self):
        """Reimplement to clear stored data based on stored schema"""
