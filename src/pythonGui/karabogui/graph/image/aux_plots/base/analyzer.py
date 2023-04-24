# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
