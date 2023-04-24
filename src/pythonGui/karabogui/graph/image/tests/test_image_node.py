# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from ..image_node import KaraboImageNode


def test_image_node(gui_app):
    """Test the image node validation"""
    node = KaraboImageNode()
    assert not node.is_valid
