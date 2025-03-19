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
import argparse
import sys
from collections import OrderedDict

from qtpy.QtWidgets import QApplication, QBoxLayout

import karabogui.binding.api as bindings
from karabo.common.scenemodel.api import BoxLayoutModel, LabelModel, SceneModel
from karabogui import icons
from karabogui.controllers.api import (
    get_class_const_trait, get_compatible_controllers, get_scene_model_class,
    populate_controller_registry)
from karabogui.panels.scenepanel import ScenePanel
from karabogui.singletons.api import get_manager, get_network


def _create_scene_model(devs, controllers=None):
    """ Creates a Scene model from registry """

    # these should match the types in the DataGenerator Device
    type_map = OrderedDict()
    type_map['state'] = (bindings.StringBinding, 'State', False)
    type_map['output.schema.node.image'] = (bindings.ImageBinding, '', False)
    type_map['boolProperty'] = (bindings.BoolBinding, '', True)
    type_map['boolPropertyReadOnly'] = (bindings.BoolBinding, '', False)
    type_map['charProperty'] = (bindings.CharBinding, '', True)
    type_map['charPropertyReadOnly'] = (bindings.CharBinding, '', False)
    type_map['int8Property'] = (bindings.Int8Binding, '', True)
    type_map['int8PropertyReadOnly'] = (bindings.Int8Binding, '', False)
    type_map['uint8Property'] = (bindings.Uint8Binding, '', True)
    type_map['uint8PropertyReadOnly'] = (bindings.Uint8Binding, '', False)
    type_map['int16Property'] = (bindings.Int16Binding, '', True)
    type_map['int16PropertyReadOnly'] = (bindings.Int16Binding, '', False)
    type_map['uint16Property'] = (bindings.Uint16Binding, '', True)
    type_map['uint16PropertyReadOnly'] = (bindings.Uint16Binding, '', False)
    type_map['int32Property'] = (bindings.Int32Binding, '', True)
    type_map['int32PropertyReadOnly'] = (bindings.Int32Binding, '', False)
    type_map['uint32Property'] = (bindings.Uint32Binding, '', True)
    type_map['uint32PropertyReadOnly'] = (bindings.Uint32Binding, '', False)
    type_map['int64Property'] = (bindings.Int64Binding, '', True)
    type_map['int64PropertyReadOnly'] = (bindings.Int64Binding, '', False)
    type_map['uint64Property'] = (bindings.Uint64Binding, '', True)
    type_map['uint64PropertyReadOnly'] = (bindings.Uint64Binding, '', False)
    type_map['floatProperty'] = (bindings.FloatBinding, '', True)
    type_map['floatPropertyReadOnly'] = (bindings.FloatBinding, '', False)
    type_map['doubleProperty'] = (bindings.FloatBinding, '', True)
    type_map['doublePropertyReadOnly'] = (bindings.FloatBinding, '', False)
    type_map['stringProperty'] = (bindings.StringBinding, '', True)
    type_map['vectors.boolProperty'] = (bindings.VectorBoolBinding, '', True)
    type_map['vectors.int8Property'] = (bindings.VectorInt8Binding, '', True)
    type_map['vectors.uint8Property'] = (bindings.VectorUint8Binding, '', True)
    type_map['vectors.int16Property'] = (bindings.VectorInt16Binding, '', True)
    type_map['vectors.uint16Property'] = (
        bindings.VectorUint16Binding, '', True)
    type_map['vectors.int32Property'] = (bindings.VectorInt32Binding, '', True)
    type_map['vectors.uint32Property'] = (
        bindings.VectorUint32Binding, '', True)
    type_map['vectors.int64Property'] = (bindings.VectorInt64Binding, '', True)
    type_map['vectors.uint64Property'] = (
        bindings.VectorUint64Binding, '', True)
    type_map['vectors.floatProperty'] = (bindings.VectorFloatBinding, '', True)
    type_map['vectors.doubleProperty'] = (
        bindings.VectorFloatBinding, '', True)
    type_map['vectors.stringProperty'] = (
        bindings.VectorStringBinding, '', True)
    type_map['table'] = (bindings.VectorHashBinding, '', True)

    rows = []
    all_controllers = set()
    for device_id in devs:
        for device_key, binding_data in type_map.items():
            binding, displayType, can_edit = binding_data
            klasses = get_compatible_controllers(binding())
            klasses += get_compatible_controllers(binding(), can_edit=True)
            klasses.sort(key=lambda x: x.__name__)
            all_controllers.update(klass.__name__ for klass in klasses)
            if controllers:
                klasses = [klass for klass in klasses
                           if klass.__name__ in controllers]
            # add the label
            key = '.'.join([device_id, device_key])
            label = LabelModel(text=key)
            row = [label]
            for klass in klasses:
                model_klass = get_scene_model_class(klass)
                class_label = LabelModel(text=klass.__name__)
                traits = {'keys': [key]}
                if get_class_const_trait(klass, '_can_edit'):
                    traits['parent_component'] = 'EditableApplyLaterComponent'
                if 'klass' in model_klass().editable_traits():
                    traits['klass'] = get_class_const_trait(klass,
                                                            '_klassname')
                widget_model = model_klass(**traits)
                model = BoxLayoutModel(direction=QBoxLayout.TopToBottom,
                                       children=[class_label, widget_model])
                row.append(model)
            box = BoxLayoutModel(direction=QBoxLayout.LeftToRight,
                                 children=row)
            rows.append(box)
    vbox = BoxLayoutModel(direction=QBoxLayout.TopToBottom, children=rows)
    scene = SceneModel(children=[vbox])

    print('\n'+'='*60+'\n')
    print('here is the list of all controllers')
    for name in all_controllers:
        print(name)
    print('='*60)
    return scene


def main():
    app = QApplication(sys.argv)

    icons.init()  # Very important!
    populate_controller_registry()

    psa = """
============================================================
This is a testbench gui viewer.

 It will generate a scene containing all possible widgets
 for a PropertyTest device.
 Add a PropertyTest device to the topic you will
 connect with instanceID 'x'
============================================================"""
    ap = argparse.ArgumentParser(
        description=psa, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('controllerName', nargs='*',
                    help='List of Controller Names')
    args = ap.parse_args(sys.argv[1:])
    ap.print_help()
    controllers = args.controllerName
    model = _create_scene_model(['x'], controllers)
    model.simple_name = 'test scene'
    panel = ScenePanel(model, True)
    # XXX: A hack to keep the toolbar visible
    panel.toolbar.setVisible(True)
    panel.show()
    panel.resize(1024, 768)

    # XXX: A hack to connect to the GUI Server for testing
    get_manager()
    get_network().connectToServer()

    app.exec()


if __name__ == '__main__':
    main()
