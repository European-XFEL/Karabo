# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import argparse

from traits.api import Instance, List

from karabo.common.api import BaseSavableModel
from karabo.common.scenemodel.api import FixedLayoutChildData, read_scene


def code_for(model, key, children=None):
    """Generate the Python code which can build a scene model object.

    :param key: the key under which more children items can be found
    """
    ignored_traits = (key, 'uuid')
    traits = []
    for name in sorted(model.copyable_trait_names()):
        value = getattr(model, name)
        if name in ignored_traits or isinstance(value, FixedLayoutChildData):
            continue

        trait_default = model.trait(name).default
        if value == trait_default:
            continue

        traits.append('{}={}'.format(name, repr(value)))

    if children:
        traits.append('{}=[{}]'.format(key, ', '.join(children)))

    klass_name = model.__class__.__name__
    return klass_name, '{}({})'.format(klass_name, ', '.join(traits))


def _list_of_savable(trait):
    if not isinstance(trait.trait_type, List):
        return False
    inner_type = trait.inner_traits[0].trait_type
    if not isinstance(inner_type, Instance):
        return False
    if not issubclass(inner_type.klass, BaseSavableModel):
        return False
    return True


def _find_children(obj):
    children = [name for name in obj.copyable_trait_names()
                if _list_of_savable(obj.trait(name))]
    if not children:
        children.append("children")
    return children


def convert_scene_model_to_code(model, name='scene'):
    """Given a `SceneModel` object, generate the Python code which can
    reproduce it.
    """
    code = []
    classes = set()

    # Note: Compatibility, not all models have a `children` list, but others.
    # If we find multiple children, we still choose `children`
    keys = _find_children(model)
    key = "children" if len(keys) > 1 else keys[0]
    children = getattr(model, key, [])
    child_names = []
    for i, child in enumerate(children):
        child_name = name + str(i)
        child_names.append(child_name)
        child_cls, child_stmts = convert_scene_model_to_code(
            child, name=child_name)
        code.extend(child_stmts)
        classes.update(child_cls)

    klass_name, stmt = code_for(model, key, children=child_names)
    code.append('{} = {}'.format(name, stmt))
    classes.add(klass_name)

    return classes, code


def main():
    description = 'Convert a Karabo scene file to Python code.'
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('filename',
                        help='An SVG file containing a Karabo scene')
    parser.add_argument('deviceId', nargs='?', default=None,
                        help='deviceId of the source device'
                             ' (optional argument)')
    ns = parser.parse_args()

    # main scene code generation
    symbols, code = convert_scene_model_to_code(read_scene(ns.filename))

    # import statement
    symbols.update({'write_scene', 'SceneTargetWindow'})
    imp_stmt = ('from karabo.common.scenemodel.api import {}'
                ''.format(', '.join(sorted(symbols))))
    code.insert(0, imp_stmt)

    # Dump it out
    indent = '\n' + ' ' * 4
    output_scene = code[0]  # <-- import statement
    output_scene += '\n\ndef get_scene(deviceId):\n'
    output_scene += ' ' * 4 + indent.join(code[1:])
    output_scene += indent + 'return write_scene(scene)\n'

    # change Ubuntu font (can be removed after some transitional period)
    output_scene = output_scene.replace("font='Ubuntu,",
                                        "font='Source Sans Pro,")

    # Substitute deviceId and print
    if ns.deviceId:
        if not output_scene.count(f"keys=['{ns.deviceId}"):
            print(f"# WARNING from scene2py: "
                  f"no occurrences of '{ns.deviceId}' found")

        output = output_scene.replace(f"keys=['{ns.deviceId}",
                                      "keys=[f'{deviceId}")
        output = output.replace(f"'{ns.deviceId}", "f'{deviceId}")
        output = output.replace(f"{ns.deviceId}", "{deviceId}")
        print(output)
    else:
        print(output_scene)


if __name__ == '__main__':
    main()
