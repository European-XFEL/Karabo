..
  Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

**************
Karabo GUI API
**************

.. testcode::
   :hide:

   import karabogui.api


.. automodule:: karabogui.api


The Bindings
============

.. autoclass:: karabogui.api.BaseBinding
   :members: check, is_allowed


There are several util functions for bindings available.


Binding functions
-----------------

.. autofunction:: karabogui.api.has_options

.. autofunction:: karabogui.api.get_default_value

.. autofunction:: karabogui.api.get_binding_value

.. autofunction:: karabogui.api.get_min_max

.. autofunction:: karabogui.api.get_native_min_max

.. autofunction:: karabogui.api.get_min_max_size

.. autofunction:: karabogui.api.get_binding_array_value

.. autofunction:: karabogui.api.has_min_max_attributes

.. autofunction:: karabogui.api.build_binding


The atomic bindings
-------------------

.. autoclass:: karabogui.api.BoolBinding()

.. autoclass:: karabogui.api.ByteArrayBinding()

.. autoclass:: karabogui.api.CharBinding()

.. autoclass:: karabogui.api.ComplexBinding()

.. autoclass:: karabogui.api.FloatBinding()

.. autoclass:: karabogui.api.Int8Binding()

.. autoclass:: karabogui.api.Int16Binding()

.. autoclass:: karabogui.api.Int32Binding()

.. autoclass:: karabogui.api.Int64Binding()

.. autoclass:: karabogui.api.StringBinding()

.. autoclass:: karabogui.api.TableBinding()

.. autoclass:: karabogui.api.Uint8Binding()

.. autoclass:: karabogui.api.Uint16Binding()

.. autoclass:: karabogui.api.Uint32Binding()

.. autoclass:: karabogui.api.Uint64Binding()

.. autoclass:: karabogui.api.VectorBoolBinding()

.. autoclass:: karabogui.api.VectorCharBinding()

.. autoclass:: karabogui.api.VectorComplexDoubleBinding()

.. autoclass:: karabogui.api.VectorComplexFloatBinding()

.. autoclass:: karabogui.api.VectorDoubleBinding()

.. autoclass:: karabogui.api.VectorFloatBinding()

.. autoclass:: karabogui.api.VectorHashBinding()

.. autoclass:: karabogui.api.VectorInt8Binding()

.. autoclass:: karabogui.api.VectorInt16Binding()

.. autoclass:: karabogui.api.VectorInt32Binding()

.. autoclass:: karabogui.api.VectorInt64Binding()

.. autoclass:: karabogui.api.VectorStringBinding()

.. autoclass:: karabogui.api.VectorUint8Binding()

.. autoclass:: karabogui.api.VectorUint16Binding()

.. autoclass:: karabogui.api.VectorUint32Binding()

.. autoclass:: karabogui.api.VectorUint64Binding()

.. autoclass:: karabogui.api.VectorNoneBinding()


Category Bindings
-----------------

.. autoclass:: karabogui.api.VectorBinding()

.. autoclass:: karabogui.api.VectorNumberBinding()

.. autoclass:: karabogui.api.NodeBinding()

.. autoclass:: karabogui.api.IntBinding()

.. autoclass:: karabogui.api.SignedIntBinding()

.. autoclass:: karabogui.api.UnsignedIntBinding()


Available Node Bindings
-----------------------

.. autoclass:: karabogui.api.NDArrayBinding()

.. autoclass:: karabogui.api.ImageBinding()

.. autoclass:: karabogui.api.WidgetNodeBinding()


The Property Proxy
==================

The key proxy that is contained in a widget controller on a scene. The so-called
``PropertyProxy``. It contains all the necessary information and is passed on updates
to the controller's methods.

.. autoclass:: karabogui.api.PropertyProxy


Util functions for Property Proxies
-----------------------------------

.. autofunction:: karabogui.api.get_editor_value

.. autofunction:: karabogui.api.axis_label

.. autofunction:: karabogui.api.is_proxy_allowed


The GUI Parameter - The displayType
===================================

Karabo devices provide a self description of their properties, the so-called device schema. In most
cases this Schema is static and does not change on runtime of the device.

In order for the KaraboGUI client to know how to act on certain properties, the ``displayType`` attribute
can be specified on certain device properties.

The GUI then can react with a different kind of coloring in the Configurator, or provide a different set
of widget controllers for this property.


The Widget Controller
=====================

The `BaseBindingController` class is the base class of the controllers that are visible
on a scene. Every widget containing a device property must inherit this class.

.. autoclass:: karabogui.api.BaseBindingController
   :members: add_proxy, binding_update, create_widget, deferred_update, state_update, update_later, value_update

A `BaseBindingController` must be registered in the scene model factory

.. autoclass:: karabogui.api.register_binding_controller


Util functions for controllers
------------------------------

.. autofunction:: karabogui.api.with_display_type

.. autofunction:: karabogui.api.add_unit_label


Util functions to work with arrays and images
---------------------------------------------

.. autofunction:: karabogui.api.get_array_data

.. autofunction:: karabogui.api.get_image_data

.. autofunction:: karabogui.api.get_dimensions_and_encoding


Validators
==========

The KaraboGUI api offers a variety of QValidators that can be set on widgets.

.. autoclass:: karabogui.api.BindingValidator

.. autoclass:: karabogui.api.ListValidator

.. autoclass:: karabogui.api.SimpleValidator
