import argparse

from karabo.common.scenemodel.api import read_scene, FixedLayoutChildData


def code_for(model, children=None):
    """Generate the Python code which can build a scene model object.
    """
    ignored_traits = ('children', 'uuid')
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
        traits.append('children=[{}]'.format(', '.join(children)))

    klass_name = model.__class__.__name__
    return klass_name, '{}({})'.format(klass_name, ', '.join(traits))


def convert_scene_model_to_code(model, name='scene'):
    """Given a `SceneModel` object, generate the Python code which can
    reproduce it.
    """
    code = []
    classes = set()

    children = getattr(model, 'children', [])
    child_names = []
    for i, child in enumerate(children):
        child_name = name + str(i)
        child_names.append(child_name)
        child_cls, child_stmts = convert_scene_model_to_code(child,
                                                             name=child_name)
        code.extend(child_stmts)
        classes.update(child_cls)

    klass_name, stmt = code_for(model, children=child_names)
    code.append('{} = {}'.format(name, stmt))
    classes.add(klass_name)

    return classes, code


def main():
    description = 'Convert a Karabo scene file to Python code.'
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('filename',
                        help='An SVG file containing a Karabo scene')
    parser.add_argument('deviceId', help='deviceId of the source device')
    ns = parser.parse_args()

    # main scene code generation
    symbols, code = convert_scene_model_to_code(read_scene(ns.filename))

    # import statement
    symbols.update({'write_scene', 'SceneTargetWindow'})
    imp_stmt = ('from karabo.common.scenemodel.api import {}'
                ''.format(', '.join(sorted(symbols))))
    code.insert(0, imp_stmt)

    # Dump it out
    indent = '\n' + ' '*4
    output_scene = code[0]  # <-- import statement
    output_scene += '\n\ndef get_scene(deviceId):\n'
    output_scene += ' '*4 + indent.join(code[1:])
    output_scene += indent + 'return write_scene(scene)\n'

    # Substitute deviceId and print
    print(output_scene.replace(f"keys=['{ns.deviceId}", "keys=[f'{deviceId}"))


if __name__ == '__main__':
    main()
