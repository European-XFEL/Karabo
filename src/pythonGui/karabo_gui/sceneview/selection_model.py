from PyQt4.QtCore import QRect
from traits.api import HasStrictTraits, List

from .utils import calc_bounding_rect


class SceneSelectionModel(HasStrictTraits):
    """ A selection model for the SceneView.
    """
    # The list of selected scene objects (shapes, widgets, layouts)
    _selection = List

    def __iter__(self):
        """ Implement the Python iterator interface.
        """
        return iter(self._selection)

    def count(self):
        return len(self._selection)

    def clear_selection(self):
        """ Remove all objects from the selection
        """
        self._selection = []

    def deselect_object(self, obj):
        """ Deselect an object.
        """
        self._selection.remove(obj)

    def get_selection_bounds(self):
        """ Return the bounding rectangle for the objects which are selected.
        """
        x, y, w, h = calc_bounding_rect(self._selection)
        return QRect(x, y, w, h)

    def has_selection(self):
        return self.count() > 0

    def select_object(self, obj):
        """ Select an object.
        """
        self._selection.append(obj)
