from PyQt5.QtCore import QRect
from traits.api import HasStrictTraits, List

from .utils import calc_bounding_rect


class SceneSelectionModel(HasStrictTraits):
    """A selection model for the SceneView.
    """
    # The list of selected scene objects (shapes, widgets, layouts)
    _selection = List

    def __iter__(self):
        """Implement the Python iterator interface.
        """
        return iter(self._selection)

    def __len__(self):
        return len(self._selection)

    def clear_selection(self):
        """Remove all objects from the selection
        """
        self._selection = []

    def deselect_object(self, obj):
        """Deselect an object.
        """
        self._selection.remove(obj)

    def get_selection_bounds(self):
        """Return the bounding rectangle for the objects which are selected.
        """
        return self.get_item_rect()

    def get_item_rect(self):
        """Return the item rectangle for the objects which are selected.

        This rect is used for position determination and not for drawing!
        """
        return QRect(*calc_bounding_rect(self._selection))

    def has_selection(self):
        return len(self._selection) > 0

    def select_object(self, obj):
        """Select an object.
        """
        self._selection.append(obj)
