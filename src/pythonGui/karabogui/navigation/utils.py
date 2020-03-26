import karabogui.icons as icons


def get_language_icon(node):
    """Return the appropriate language (`API`) icon from a `SystemTreeNode`
    """
    attrs = node.attributes
    language = attrs.get('lang', 'unknown')

    ICONS = {"python": icons.python,
             "bound": icons.bound,
             "cpp": icons.cpp}

    return ICONS.get(language, None)
