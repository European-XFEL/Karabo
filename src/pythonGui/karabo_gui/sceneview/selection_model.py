from traits.api import HasTraits, List


class SceneSelectionModel(HasTraits):
    """ A selection model for the SceneView.
    """
    # The list of selected scene objects
    _selection = List

    def clear_selection(self):
        """ Remove all objects from the selection
        """
        self._selection = []

    def get_selection_bounds(self):
        """ Return the bounding rectangle for the objects which are selected.
        """
        left, top, right, bottom = 10000, 10000, 0, 0
        for obj in self._selection:
            rect = obj.geometry()
            left = min(left, rect.left())
            top = min(top, rect.top())
            bottom = max(bottom, rect.bottom())
            right = max(right, rect.right())

        return (left, top, right - left, bottom - top)

    def select_object(self, obj):
        """ Select an object.
        """
        self._selection.append(obj)
