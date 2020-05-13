import karabogui.icons as icons

ICONS = {"python": icons.python,
         "bound": icons.bound,
         "cpp": icons.cpp,
         "macro": icons.macro}


def get_language_icon(node):
    """Return the appropriate language (`API`) icon from a `SystemTreeNode`
    """
    attrs = node.attributes
    language = attrs.get('lang', 'unknown')

    return ICONS.get(language, None)
