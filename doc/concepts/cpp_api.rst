.. _cpp_api:

***********
The C++ API
***********


Usage Scenarios
===============

The C++ API is to be used when developing devices interfacing directly
with hardware or for processing tasks which benefit from using the Karabo
point-to-point communication interface. In terms of APIs it is Karabo's
reference implementation.

Benefits
++++++++

The C++ API allows for statically typed, compile-time checked coding. Interfacing
with hardware, which has fixed property types frequently benefits from this,
as consistency checks of date types are enforced by the compiler.

Additionally, depending on the task at hand a compiled device may also offer significant
performance benefits with respect to runtime-interpreted Python device.

Finally, some external library may only be available with C/C++ interfaces.
Rather than binding these from Python, developers should directly interface
from the C++ API.

Limitations
+++++++++++

The development cycles for compiled code tend to be longer, especially when
heavy use of template mechanisms, as is the case for Karabo, is made.
Additionally, more lines of code are frequently required in C++, than are
necessary for achieving the same functionality in Python.

Programming Policies
====================

Devices written in C++ using the Karabo C++ API should follow common coding
standards of the language. Specifically, the programmer should assure that

- objects are cleanly destroyed at the end of their lifetime
- allocated memory is freed
- data is encapsulated in objects and accessed only via public members
- namespaces are maintained and used.

Additionally, in Karabo programmers are asked to use the following policies:

CamelCasing

    is generally used when coding

Classes

    have capitalized names

Class member variables

    should be prefixed with ``m_``, i.e. ``m_aClassMember``

Additionally, Karabo makes heavy use of facilities provided by the Boost
library. Developers are advised to make use of Boost functionality when
possible.

C++11
=====

C++11 usage is now (officially) supported for framework code. The following
guidelines are suggested:

- Feel free to use new features where they make sense. E.g. use auto to shorten
  iterator syntax in loops, e.g.
  `std::map<MyComplexType, MyMoreComplexType<double> >::const_iterator it = foo.begin() -> auto it = foo.begin()`.

- Don’t use `auto` to indicate straight forward types, e.g. `auto i = 4;`

- Existing code does not need to be refactored for C++11 feature usage alone.
  E.g. if you happen to refactor something anyway, feel free to replace iterators
  with `auto` if it aids readability. You do not specifically have to refactor
  otherwise working code though.

- Do **not** use `std::shared_ptr`, we will continue to use `boost::shared_ptr`!

- In general, if a `boost` and a `std`-library feature coexist
  (smart pointers, mutices, bind, etc.), continue to use the boost implementation
  as we have done previously, especially if there is a risk that your new code
  needs to interact with existing code.

- When using more „advanced“ features, like late return type declaration
  (`->decltype(foo)`), variadic templates or reference forwarding, add a short
  comment to these lines to aid people less experienced with C++11 features in
  the review.

- We currently do not encourage to use newly introduced numerical types, e.g.
  `uint64_t` as the Karabo type system has not been fully prepared for them.

Implementing Devices
====================

The "Conveyor" device
+++++++++++++++++++++

Let us start by having a look at a toy device that simulates a conveyor belt...

.. code-block:: c++

   #ifndef KARABO_CONVEYOR_HH
   #define KARABO_CONVEYOR_HH

   #include <karabo/karabo.hpp>

   /**
    * The main Karabo namespace
    */
   namespace karabo {

       class Conveyor : public karabo::core::Device<> {

       public:

	   // Add reflection information and Karabo framework compatibility to this class
	   KARABO_CLASSINFO(Conveyor, "Conveyor", "2.0")

	   /**
	    * Necessary method as part of the factory/configuration system
	    * @param expected Will contain a description of expected parameters
	    * for this device
	    */
	   static void expectedParameters(karabo::util::Schema& expected);

	   /**
	    * Constructor providing the initial configuration in form of a Hash object.
	    * If this class is constructed using the configuration system the Hash object
	    * will already be validated using the information of the expectedParameters
	    * function. The configuration is provided in a key/value fashion.
	    */
	   Conveyor(const karabo::util::Hash& config);

	   /**
	    * The destructor will be called in case the device gets killed
	    * (i.e. the event-loop returns)
	    */
	   virtual ~Conveyor() {
	       KARABO_LOG_INFO << "dead.";
	   }

	   /**
	    * This function acts as a hook and is called after an reconfiguration
	    * request was received, but BEFORE this reconfiguration request is actually
	    * merged into this device's state.
	    *
	    * The reconfiguration information is contained in the Hash object provided
	    * as an argument.
	    * You have a chance to change the content of this Hash before it is merged
	    * into the device's current state.
	    *
	    * NOTE: (a) The incomingReconfiguration was validated before
	    *       (b) If you do not need to handle the reconfigured data, there is
	    *           no need to implement this function.
	    *           The reconfiguration will automatically be applied to the
	    *           current state.
	    * @param incomingReconfiguration The reconfiguration information as was
	    *         triggered externally
	    */
	   virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration);


	   /**
	    * This function acts as a hook and is called after an reconfiguration
	    * request was received, and AFTER this reconfiguration request has been
	    * merged into this device's current state.
	    * You may access any (updated or not) parameters using the usual
	    * getters and setters.
	    * @code
	    * int i = get<int>("myParam");
	    * @endcode
	    */
	   virtual void postReconfigure();


       private:

	   void initialize();

	   void start();

	   void stop();

	   void reset();

       };
   }

   #endif

... and explain what is happening step by step.

The include statement

.. code-block:: c++

   #include <karabo/karabo.hpp>

provides you access to the full Karabo framework. Both include paths and namespaces
follow the physical directory layout of the Karabo framework sources.
Karabo comprises the following main functionalities (reflected as source directories):

* util: Factories, Configurator, Hash, Schema, String and Time tools, etc.
* io: Serializer, Input, Output, FileIO tools
* io/h5: HDF5 interface (HDF5, Hash serialization)
* log: unified logging using Log4Cpp as engine
* webAuth: Webservice based authentification (based on gsoap)
* net: TCP (point to point) and JMS (broker-based) networking in synchronous and
  asynchronous fashion.
* xms: Higher level communication API (Signals & Slots, Request/Response, etc.)
* xip: Image classes, processing, GPU code
* core: Device, DeviceServer, DeviceClient base classes

Consequently, if you want to include less, you can refer to a header of a
specific functionality (like in boost, e.g. <karabo/util.hpp>, or <karabo/io.hpp>) or
of a single class (e.g. <karabo/webAuth/Authenticator.hh>).

It is good practice to place your class into the karabo namespace

.. code-block:: c++

   namespace karabo {

        class Conveyor : public karabo::core::Device<> {

Any device must by some means derive from the templated class Device<>,
the template indicating which interface class to use (we look later to this).
In the simplest case you leave the template empty (like here) and solely derive
from the Device<> base class.

The ``KARABO_CLASSINFO`` macro

.. code-block:: c++

    KARABO_CLASSINFO(Conveyor, "Conveyor", "2.0")


adds what C++ does not provide by default: reflection (or introspection) information.
It for example defines

.. code-block:: c++

    typedef Self Conveyor;

this is convenient to use e.g. in generic template code. Even more important
is the string identifier for the class, called ``classId``. The configurator
system will utilize this information for factory-like object construction.
The final argument (e.g. 2.0) indicates with which Karabo framework version a device
is compatible with. Only one version should be given here and it should only be
specified up the minor version number.

The expected parameter function

.. code-block:: c++

    static void expectedParameters(karabo::util::Schema& expected);

is where you should describe what properties and commands are available on this device.
The function is static in order to be evaluated before actual device instantiation
and to generate meaningful graphical widgets that guide users in setting up the initial
device configuration. This function is called several times (whenever some other party
needs to know about your device's schema).

The constructor

.. code-block:: c++

    Conveyor(const karabo::util::Hash& config);

is called-back by the configurator mechanism. Otherwise it is a regular constructor.

.. warning::

	While still being constructed the device does not (fully) interact with the
	distributed system. You should thus not have long running, or even blocking
	code in the contructor. Such code belongs into initialization functions.

If you are managing your own threads in the device, which need joining or allocated
heap memory that you need to free upon device destruction, the destructor is the place
for doing so. It is guaranteed to be called, whenever a device instance gets killed.

.. code-block:: c++

    virtual ~Conveyor();

The ``preReconfigure`` and ``postReconfigure`` functions,

.. code-block:: c++

    virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration);
    virtual void postReconfigure()

are called after a reconfiguration request on the device's properties has been received,
respectively *before* and *after* the new configuration has been merged into the device's
state.

Karabo conceptually distinguishes between the execution of commands
(state-machine event triggers) and setting of properties. A command execution may be
followed by a state change, whilst property setting **should not** lead to a state
change.

.. note::

    There is an exception to this policy, in that assingment may trigger an
    action or state change but must be indicated by setting the ``setAndExecute``
    attribute on the property as detailed in Section :ref:`setandexecute`.

In our example: starting if the conveyor would utilize a property setting and a command.
First a "targetSpeed" property would be set (no state change), and afterwards
a "start" command would be issued which actually triggers the
state-machine and drives it into "Starting" and finally "Started"
state.

This conceptual separation is reflected in all APIs and the
two functions above reflect the hook into the property configuration
system.

The remaining functions reflect each *command* that is available on this device.

.. code-block:: c++

   void initialize();

   void start();

   void stop();

   void reset();


Now let us have a look at the implementation, here is the complete file

.. code-block:: c++

   #include "Conveyor.hh"

   using namespace std;

   USING_KARABO_NAMESPACES;

   namespace karabo {


       KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, Conveyor);


       void Conveyor::expectedParameters(Schema& expected) {

	   OVERWRITE_ELEMENT(expected).key("state")
		   .setNewOptions("Initializing,Error,Started,Stopping,Stopped,Starting")
		   .setNewDefaultValue(States::INIT)
		   .commit();

	   SLOT_ELEMENT(expected).key("start")
		   .displayedName("Start")
		   .description("Instructs device to go to started state")
		   .allowedStates(States::STOPPED)
		   .commit();

	   SLOT_ELEMENT(expected).key("stop")
		   .displayedName("Stop")
		   .description("Instructs device to go to stopped state")
		   .allowedStates(States::STARTED)
		   .commit();


	   SLOT_ELEMENT(expected).key("reset")
		   .displayedName("Reset")
		   .description("Resets in case of an error")
		   .allowedStates(States::ERROR)
		   .commit();

	   FLOAT_ELEMENT(expected).key("targetSpeed")
		   .displayedName("Target Conveyor Speed")
		   .description("Configures the speed of the conveyor belt")
		   .unit(Unit::METER_PER_SECOND)
		   .assignmentOptional().defaultValue(0.8)
		   .reconfigurable()
		   .commit();

	   FLOAT_ELEMENT(expected).key("currentSpeed")
		   .displayedName("Current Conveyor Speed")
		   .description("Shows the current speed of the conveyor")
		   .readOnly()
		   .initialValue(0.0)
		   .commit();

	   BOOL_ELEMENT(expected).key("reverseDirection")
		   .displayedName("Reverse Direction")
		   .description("Reverses the direction of the conveyor band")
		   .assignmentOptional().defaultValue(false)
		   .allowedStates(States::STOPPED)
		   .reconfigurable()
		   .commit();

	   BOOL_ELEMENT(expected).key("injectError")
		   .displayedName("Inject Error")
		   .description("Does not correctly stop the conveyor,
		                 such that a Error is triggered during next start")
		   .assignmentOptional().defaultValue(false)
		   .reconfigurable()
		   .expertAccess()
		   .commit();
       }


       Conveyor::Conveyor(const karabo::util::Hash& config) : Device<>(config) {

	   // Register initialState member function to be called after the run()
	   // member function is called
	   registerInitialFunction(initialize);

	   KARABO_SLOT(start);
	   KARABO_SLOT(stop);
	   KARABO_SLOT(reset);
       }


       void Conveyor::preReconfigure(karabo::util::Hash& config) {

	   // The preReconfigure hook allows to forward the configuration to
	   // some connected h/w

	   try {

	       if (config.has("targetSpeed")) {
		   // Simulate setting to h/w
		   KARABO_LOG_INFO << "Setting to hardware: targetSpeed -> "
		                   << config.get<float>("targetSpeed");
	       }

	       if (config.has("reverseDirection")) {
		   // Simulate setting to h/w
		   KARABO_LOG_INFO << "Setting to hardware: reverseDirection -> "
		                   << config.get<bool>("reverseDirection");
	       }

	   } catch (...) {
	       // You may want to indicate that the h/w failed
	       updateState(States::ERROR);
	   }
       }


       void Conveyor::postReconfigure() {
       }


       void Conveyor::initialize() {
	   // As the Initializing state is not mentioned in the allowed states
	   // nothing else is possible during this state
	   updateState(States::INIT);

	   KARABO_LOG_INFO << "Connecting to conveyer hardware...";

	   // Simulate some time it could need to connect and setup
	   boost::this_thread::sleep(boost::posix_time::seconds(2));

	   // Automatically trigger got the Stopped state
	   stop();
       }


       void Conveyor::start() {
	   updateState(States::STARTING); // use this if long-lasting work follows ...

	   // Retrieve current values from our own device-state
	   float tgtSpeed = get<float>("targetSpeed");
	   float currentSpeed = get<float>("currentSpeed");

	   // If we do not stand still here that is an error
	   if (currentSpeed > 0.0) {
	       KARABO_LOG_ERROR << "Conveyer does not stand still at start-up";
	       updateState(States::ERROR);
	       return;
	   }

	   // Separate ramping into 50 steps
	   float increase = tgtSpeed / 50.0;

	   // Simulate a slow ramping up of the conveyor
	   for (int i = 0; i < 50; ++i) {
	       currentSpeed += increase;
	       set("currentSpeed", currentSpeed);
	       boost::this_thread::sleep(boost::posix_time::millisec(50));
	   }
	   // Be sure to finally run with targetSpeed
	   set<float>("currentSpeed", tgtSpeed);

	   updateState(States::STARTED);

       }


       void Conveyor::stop() {
	   updateState(States::STOPPING); // use this if long-lasting work follows ...

	   // Retrieve current value from our own device-state
	   float currentSpeed = get<float>("currentSpeed");

	   if (currentSpeed != 0.0f) {
	       // Separate ramping into 50 steps
	       float decrease = currentSpeed / 50.0;

	       // Simulate a slow ramping down of the conveyor
	       for (int i = 0; i < 50; ++i) {
		   currentSpeed -= decrease;
		   set("currentSpeed", currentSpeed);
		   boost::this_thread::sleep(boost::posix_time::millisec(50));
	       }
	       // Be sure to finally stand still
	       if (get<bool>("injectError")) {
		   set<float>("currentSpeed", 0.1);
	       } else {
		   set<float>("currentSpeed", 0.0);
	       }
	   }
	   updateState(States::STOPPED);
       }


       void Conveyor::reset() {
	   set("injectError", false);
	   set<float>("currentSpeed", 0.0);
	   initialize();
       }
   }

and go through it step by step.

The macro

.. code-block:: c++

    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, Conveyor)

registers the device to the BaseDevice configurator factory. The expected
parameters of all classes mentioned in this macro will be evaluated
and concatenated from left to right. In this way our Conveyor device
inherits all expected parameters from BaseDevice (which has none), and
from Device<> (which has a few).

In the expectedParameters() function the parameters for this device are
defined. See :ref:`here <data_types>` for more details of how this is done.

The constructor

.. code-block:: c++

    Conveyor::Conveyor(const karabo::util::Hash& config) : Device<>(config) {

        // Register initialState member function to be called after the run()
        // member function is called
        registerInitialFunction(initialize);

        KARABO_SLOT(start);
        KARABO_SLOT(stop);
        KARABO_SLOT(reset);
    }


does not deal with the provided configuration, despite calling the parent
class' constructor with it (as is proper C++). This is completely fine for two reasons:
1. The provided configuration got validated BEFORE the constructor was even called.
2. The Device<> base class manages the configuration (actually in form of a
   :ref:`Hash <cppHash>`) and provides access to it with its getters and setters.

Of course you can create a member variable and assign it by using the value
in the configuration passed, like:

.. code-block:: c++

    Conveyor::Conveyor(const karabo::util::Hash& config) : Device<>(config) {
        m_speed = config.get<string>("targetSpeed");
    }

but then you have to be careful to keep this member variable in sync! You
should update it yourself in the postReconfiguration() function.

.. note::

	It is generally not recommended to keep any private members as copies of
	configuration variables. Karabo's setters and getters will perform well enough
	for most of use cases and assure that the device properties are kept syncronized
	with your configuration.

As said before, no long lasting or even blocking activities should be
implemented in the constructor. For that reason a macro is available
(``registerInitialFunction``) which allows to bind a function that acts
like a "second constructor". In this function you can write
code without the restrictions of the constructor. Use this function if
you need to interact with properties of the device.

.. warning::

	In the constructor you should only access devices properties through the
	configuration hash passed to it. Accessing properties with getter/setter
	methods upon initialization should happen in the function registered
	via registerInitialFunction.

The last three statements in the constructor make the otherwise
regular functions start, stop and reset callable from the distributed system
(slots).

.. note::

	The function names must match the key names of
	the SLOT_ELEMENTs defined in the expectedParameters function. Only
	then will the automatically generated GUI or the command-line interface
	execute the slot bound to a given SLOT_ELEMENT.

	Functions mapping to slots in node elements should replace any "." separators
	in the expected parameter key with underscores ("\_").

The function preReconfigure

.. code-block:: c++

    void Conveyor::preReconfigure(karabo::util::Hash& config) {

        // The preReconfigure hook allows to forward the configuration
        // to some connected h/w

        try {

            if (config.has("targetSpeed")) {
                // Simulate setting to h/w
                KARABO_LOG_INFO << "Setting to hardware: targetSpeed -> "
                                << config.get<float>("targetSpeed");
            }

            if (config.has("reverseDirection")) {
                // Simulate setting to h/w
                KARABO_LOG_INFO << "Setting to hardware: targetSpeed -> "
                                << config.get<bool>("reverseDirection");
            }

        } catch (...) {
            // You may want to indicate that the h/w failed
            updateState(States::ERROR);
        }
    }


acts as a hook *before* the requested reconfiguration is merged to the
device's internal state. All potential reconfiguration requests are
packaged into the config hash. You should check yourself for the
ones you are interested in.

For this you can use the *has* function of the Hash object like here:

.. code-block:: c++

            if (config.has("targetSpeed")) {
                // Simulate setting to h/w
                KARABO_LOG_INFO << "Setting to hardware: targetSpeed -> "
                                << config.get<float>("targetSpeed");
            }

As we only simulate a conveyor h/w, we send a message instead,
pretending we did something. Messages using the KARABO_LOG\_ prefix
will be visible to the users (distributed via the broker), they
come in for categories DEBUG, INFO, WARN and ERROR.

..warning::

	Use log messaging sparsely to not pollute the network and the log-files. If you
	need messages for local debugging use the KARABO_LOG_FRAMEWORK\_ in
	combination with DEBUG, INFO, WARN and ERROR instead. Also, please refer to
	Section :ref:`alarm_system`.


Before looking closer at the initialize function, let's list some best
practices for all call-back functions (mostly slots) of Karabo:

1. Never completely block and rely on another function to unblock it

2. Always update the state

3. Only use try/catch blocks if you want to react on an exception
   (by driving the device into *ERROR* state for example), else trust in Karabo in
   handling them.

Now, in the initialize function (which is automatically called once the constructor
finished execution)

.. code-block:: c++

    void Conveyor::initialize() {
        // As the Initializing state is not mentioned in the allowed states
        // nothing else is possible during this state
        updateState(States::INIT);

        KARABO_LOG_INFO << "Connecting to conveyer hardware...";

        // Simulate some time it could need to connect and setup
        boost::this_thread::sleep(boost::posix_time::seconds(2));

        // Automatically trigger got the Stopped state
        stop();
    }

you see an immediate call to updateState. This is good practice, as
the following activity (namely connecting to the motor) may take some
time (here simulated to be two seconds). Most importantly the GUI will
be nicely graying out other buttons and informing the user what is
happening. Once connected we internally call the stop command (in
reality on should ask the hardware which state it is in an adapt
accordingly).

We are almost done, start and stop are very similar and reset is
almost trivial, so lets only look
at the start function:

.. code-block:: c++

    void Conveyor::start() {
        updateState(States::STARTING); // use this if long-lasting work follows ...

        // Retrieve current values from our own device-state
        float tgtSpeed = get<float>("targetSpeed");
        float currentSpeed = get<float>("currentSpeed");

        // If we do not stand still here that is an error
        if (currentSpeed > 0.0) {
            KARABO_LOG_ERROR << "Conveyer does not stand still at start-up";
            updateState(States::ERROR);
            return;
        }

        // Separate ramping into 50 steps
        float increase = tgtSpeed / 50.0;

        // Simulate a slow ramping up of the conveyor
        for (int i = 0; i < 50; ++i) {
            currentSpeed += increase;
            set("currentSpeed", currentSpeed);
            boost::this_thread::sleep(boost::posix_time::millisec(50));
        }
        // Be sure to finally run with targetSpeed
        set<float>("currentSpeed", tgtSpeed);

        updateState(States::STARTED);

    }

We simulate a slow ramping up of the speed and explicitly inform
about that using the intermediate state *STARTING*.

.. code-block:: c++

    void Conveyor::start() {
        updateState(States::STARTING); // use this if long-lasting work follows ...


In the following lines you can see, how properties of your device (which must always be
part of the expectedParameters) can be read. A call to get is
always thread-safe and always returns the latest value
configured.

.. code-block:: c++

        // Retrieve current values from our own device-state
        float tgtSpeed = get<float>("targetSpeed");
        float currentSpeed = get<float>("currentSpeed");

The next part shows one example to potentially drive your device into an *ERROR* state.
Here we check, whether the conveyer stands still before starting it.
Note the return statement to finish the execution of the function.

The last part of the start function simulates the ramping up by giving several updates
on the "currentSpeed" property with some fixed delay. Setting a property value like
here for "currentSpeed" does two things, it updates the own device
state and publishes this value to the broker, such that interested
clients will get an event.

