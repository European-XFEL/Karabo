.. _cppDevice:


******************************
 How to write a device in C++
******************************

The "Conveyor" device
=====================

Let us start by having a look at a toy device that should simulate a conveyor belt...

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
	   KARABO_CLASSINFO(Conveyor, "Conveyor", "1.3")

	   /**
	    * Necessary method as part of the factory/configuration system
	    * @param expected Will contain a description of expected parameters for this device
	    */
	   static void expectedParameters(karabo::util::Schema& expected);

	   /**
	    * Constructor providing the initial configuration in form of a Hash object.
	    * If this class is constructed using the configuration system the Hash object will
	    * already be validated using the information of the expectedParameters function.
	    * The configuration is provided in a key/value fashion. 
	    */
	   Conveyor(const karabo::util::Hash& config);

	   /**
	    * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
	    */
	   virtual ~Conveyor() {
	       KARABO_LOG_INFO << "dead.";
	   }

	   /**
	    * This function acts as a hook and is called after an reconfiguration request was received,
	    * but BEFORE this reconfiguration request is actually merged into this device's state.
	    * 
	    * The reconfiguration information is contained in the Hash object provided as an argument.
	    * You have a chance to change the content of this Hash before it is merged into the device's current state.
	    * 
	    * NOTE: (a) The incomingReconfiguration was validated before
	    *       (b) If you do not need to handle the reconfigured data, there is no need to implement this function.
	    *           The reconfiguration will automatically be applied to the current state.
	    * @param incomingReconfiguration The reconfiguration information as was triggered externally
	    */
	   virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration);


	   /**
	    * This function acts as a hook and is called after an reconfiguration request was received,
	    * and AFTER this reconfiguration request got merged into this device's current state.
	    * You may access any (updated or not) parameters using the usual getters and setters.
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
    
provides you the full Karabo framework. Both, include pathes and namespaces follow the physical directory layout of the Karabo framework sources. Karabo comprises the following main functionalities (reflected as source directories):

* util: Factories, Configurator, Hash, Schema, String and Time tools, etc.
* io: Serializer, Input, Output, FileIO tools
* io/h5: HDF5 interface (HDF5, Hash serialization)
* log: unified logging using Log4Cpp as engine
* webAuth: Webservice based authentification (based on gsoap)
* net: TCP (point to point) and JMS (broker-based) networking in synchronous and asynchronous fashion.
* xms: Higher level communication API (Signals & Slots, Request/Response, etc.)
* xip: Image classes, processing, GPU code
* core: Device, DeviceServer, DeviceClient base classes

Consequently, if you want to include less, you can refer to a header of a specific functionality (like in boost, e.g. <karabo/util/util.hpp>, or <karabo/io/io.hpp>).

It is good practice to place your class into the karabo namespace

.. code-block:: c++

   namespace karabo {
		
        class Conveyor : public karabo::core::Device<> {

Any Device must in the end derive from the templated class Device<>, the template indicating which interface class to use (we look later to this). In the easiest case you leave the template empty (like here) and solely derive from the Device<> base class.

The KARABO_CLASSINFO macro

.. code-block:: c++

    KARABO_CLASSINFO(Conveyor, "Conveyor", "1.3")


adds what C++ does not provide by default: reflection (or introspection) information. It for example defines

.. code-block:: c++

    typedef Self Conveyor;

this is convenient in for example generic template code. Even more important is the string identifier for the class, called **classId**. The configurator system will utilize this information for factory-like object construction. The last argument (1.3) tells with which Karabo framework version the Device is compatible with. Only one version should be given here and it should only be specified up the minor.

The expected parameter function

.. code-block:: c++

    static void expectedParameters(karabo::util::Schema& expected);

is the one and only place where you should describe what properties and commands are available on this device. The function is static in order to be parsed before instantiation time and to generate meaningful graphical widgets that guide users to set up the initial configuration. This function will be called several times (whenever some other party needs to know about your configuration information).

The constructor

.. code-block:: c++

    Conveyor(const karabo::util::Hash& config);

will be called-back by the configurator mechanism. It else is a regular constructor.

**NOTE**: Currently, device construction happends in the main thread of the device-server. So make sure you do not have slow or even blocking code in your constructor as it will block the whole server.

If you opened any threads yourself in the device or allocated heaped memory that you have to free, the destructor is the place for doing so. It is guaranteed to be called, whenever a device instance gets killed.

.. code-block:: c++

    virtual ~Conveyor();

The preReconfigure and postReconfigure functions,

.. code-block:: c++

    virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration);
    virtual void postReconfigure()

are called after a reconfiguration request was received, respectively
*before* and *after* it has been merged into the device's state.

Karabo conceptually distinguishes between execution of commands
(state-machine event triggers) and settings of properties. Execution
of commands intend to be followed by a state change, whilst property
settings **should not** lead to a state change. Example: Starting the
conveyor would utilize a property setting and a command. First a
"targetSpeed" property would be set (no state change), and afterwards
a "start" command would be issued which really triggers the
state-machine and drives it into "Starting" and finally "Started"
state. This conceptual separation is reflected into the API and the
two functions above reflect the hook into the property configuration
system. They will be called any time an external user thinks he wants
to reconfigure something.

The remaining functions reflect each command that is available on this device.

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
		   .setNewDefaultValue("Initializing")
		   .commit();

	   SLOT_ELEMENT(expected).key("start")
		   .displayedName("Start")
		   .description("Instructs device to go to started state")
		   .allowedStates("Stopped")
		   .commit();

	   SLOT_ELEMENT(expected).key("stop")
		   .displayedName("Stop")
		   .description("Instructs device to go to stopped state")
		   .allowedStates("Started")
		   .commit();


	   SLOT_ELEMENT(expected).key("reset")
		   .displayedName("Reset")
		   .description("Resets in case of an error")
		   .allowedStates("Error")
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
		   .allowedStates("Stopped")
		   .reconfigurable()
		   .commit();

	   BOOL_ELEMENT(expected).key("injectError")
		   .displayedName("Inject Error")
		   .description("Does not correctly stop the conveyor, such that a Error is triggered during next start")
		   .assignmentOptional().defaultValue(false)                
		   .reconfigurable()
		   .expertAccess()
		   .commit();
       }


       Conveyor::Conveyor(const karabo::util::Hash& config) : Device<>(config) {

	   // Register initialState member function to be called after the run() member function is called
	   KARABO_INITIAL_FUNCTION(initialize);

	   KARABO_SLOT(start);
	   KARABO_SLOT(stop);
	   KARABO_SLOT(reset);
       }


       void Conveyor::preReconfigure(karabo::util::Hash& config) {

	   // The preReconfigure hook allows to forward the configuration to some connected h/w

	   try {

	       if (config.has("targetSpeed")) {
		   // Simulate setting to h/w
		   KARABO_LOG_INFO << "Setting to hardware: targetSpeed -> " << config.get<float>("targetSpeed");
	       }

	       if (config.has("reverseDirection")) {
		   // Simulate setting to h/w
		   KARABO_LOG_INFO << "Setting to hardware: targetSpeed -> " << config.get<bool>("reverseDirection");
	       }

	   } catch (...) {
	       // You may want to indicate that the h/w failed
	       updateState("Error");
	   }
       }


       void Conveyor::postReconfigure() {
       }


       void Conveyor::initialize() {
	   // As the Initializing state is not mentioned in the allowed states
	   // nothing else is possible during this state
	   updateState("Initializing");

	   KARABO_LOG_INFO << "Connecting to conveyer hardware...";

	   // Simulate some time it could need to connect and setup
	   boost::this_thread::sleep(boost::posix_time::seconds(2));

	   // Automatically trigger got the Stopped state
	   stop();
       }


       void Conveyor::start() {
	   updateState("Starting"); // use this if long-lasting work follows ...

	   // Retrieve current values from our own device-state
	   float tgtSpeed = get<float>("targetSpeed");
	   float currentSpeed = get<float>("currentSpeed");

	   // If we do not stand still here that is an error
	   if (currentSpeed > 0.0) {
	       KARABO_LOG_ERROR << "Conveyer does not stand still at start-up";
	       updateState("Error");
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

	   updateState("Started");

       }


       void Conveyor::stop() {
	   updateState("Stopping"); // use this if long-lasting work follows ...

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
	   updateState("Stopped");
       }


       void Conveyor::reset() {
	   set("injectError", false);
	   initialize();
       }
   }

and go through it step by step.

The macro

.. code-block:: c++

    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<>, Conveyor)

registers the device to BaseDevice configurator factory. The expected
parameters of all classes mentioned in this macro will be evaluated
and concatenated from left to right. In this way our Conveyor device
inherits all expected parameters from BaseDevice (which has none), and
from Device<> (which has a few).

In the expectedParameter() function the parameters for this device are
defined. See :ref:`here <cppSchema>` for more details of how doing so.

The constructor

.. code-block:: c++

    Conveyor::Conveyor(const karabo::util::Hash& config) : Device<>(config) {

        // Register initialState member function to be called after the run() member function is called
        KARABO_INITIAL_FUNCTION(initialize);      

        KARABO_SLOT(start);
        KARABO_SLOT(stop);
        KARABO_SLOT(reset);
    }


does not deal with the provided configuration, despite calling the parent class' constructor with it (as is proper C++). This is completely fine for two reasons:
1. The provided configuration got validated BEFORE the constructor was even called.
2. The Device<> base class keeps the configuration (actually in form of a :ref:`Hash <cppHash>`) and gives access to it with getters and setters.

Of course you can create a member variable and assign it by using the value in the provided configuration, like:

.. code-block:: c++

    Conveyor::Conveyor(const karabo::util::Hash& config) : Device<>(config) {
        m_speed = config.get<string>("targetSpeed");
    }

but then you have to be careful to keep the variable in sync! You
should update it yourself in the postReconfiguration() function. We
generally recommend not to keep any private members being copies of
the configuration variables. Karabo's setters and getters will do fine
and perform fast enough for most of the cases.

As said before, no long lasting or even blocking activities should be
implemented in the constructor. For that reason a macro is available
(KARABO_INITIAL_FUNCTION) which allows to bind a function that acts
like a "second constructor". In this function you can write whatever
code without the restrictions of the constructor. Use this function if
you want to already set some properties and not the constructor.

The last three statements in the constructor make the otherwise
regular functions start, stop and reset callable from outside
(slots). **IMPORTANT**: The function names must match the key names of
the SLOT_ELEMENTs defined in the expectedParameters function. Only
then will the automatically generated GUI or command line command
call-back the corresponding function.


The function preReconfigure

.. code-block:: c++

    void Conveyor::preReconfigure(karabo::util::Hash& config) {

        // The preReconfigure hook allows to forward the configuration to some connected h/w

        try {

            if (config.has("targetSpeed")) {
                // Simulate setting to h/w
                KARABO_LOG_INFO << "Setting to hardware: targetSpeed -> " << config.get<float>("targetSpeed");
            }

            if (config.has("reverseDirection")) {
                // Simulate setting to h/w
                KARABO_LOG_INFO << "Setting to hardware: targetSpeed -> " << config.get<bool>("reverseDirection");
            }

        } catch (...) {
            // You may want to indicate that the h/w failed
            updateState("Error");
        }
    }


acts as a hook *before* the requested reconfiguration is merged to the
device's internal state. All potential reconfiguration requests are
packed into the config Hash, and you have to check yourself for the
ones you are interested in.

For that you can use the *has* function of the Hash object like here:

.. code-block:: c++

            if (config.has("targetSpeed")) {
                // Simulate setting to h/w
                KARABO_LOG_INFO << "Setting to hardware: targetSpeed -> " << config.get<float>("targetSpeed");
            }

As we only simulate a real conveyer h/w, we send a message instead
pretending we did something. Messages using the KARABO_LOG_ prefix
will be visible to the end users (distributed via the broker), they
come in 4 categories DEBUG, INFO, WARN and ERROR.  **NOTE**: Use this
message sparingly to not pollute the network and the log-files. If you
need messages for local debugging use the KARABO_LOG_FRAMEWORK_ in
combination with DEBUG, INFO, WARN and ERROR instead.


Before looking closer to the initialize function, let's list some best
practices for all call-back functions (mostly slots) of Karabo:

(1) Never completely block and rely on another function to unblock it

(2) Always update the state

(3) Only use try/catch blocks if you want to react on an exception (by driving the device into "Error" state for example), else trust in Karabo handling them

Now in the intialize function (which is automatically called once the constructor finished)

.. code-block:: c++

    void Conveyor::initialize() {
        // As the Initializing state is not mentioned in the allowed states
        // nothing else is possible during this state
        updateState("Initializing");

        KARABO_LOG_INFO << "Connecting to conveyer hardware...";

        // Simulate some time it could need to connect and setup
        boost::this_thread::sleep(boost::posix_time::seconds(2));

        // Automatically trigger got the Stopped state
        stop();
    }

you see an immediate call to updateState. That is good practice, as
the following activity (namely connecting to the motor) may take some
time (here simulated to be 2 seconds). Most importantly the GUI will
be nicely graying out other buttons and informing the user what is
happening. Once connected we internally call the stop command (in
reality on should ask the h/w what state it is in an adapt
accordingly).

We are almost done, start and stop are very similar so lets only look
at the start function:

.. code-block:: c++

    void Conveyor::start() {
        updateState("Starting"); // use this if long-lasting work follows ...

        // Retrieve current values from our own device-state
        float tgtSpeed = get<float>("targetSpeed");
        float currentSpeed = get<float>("currentSpeed");

        // If we do not stand still here that is an error
        if (currentSpeed > 0.0) {
            KARABO_LOG_ERROR << "Conveyer does not stand still at start-up";
            updateState("Error");
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

        updateState("Started");

    }     

We simulate a slow ramping up of the speed and explicitely inform
about that using the intermediate state "Starting". 

.. code-block:: c++

    void Conveyor::start() {
        updateState("Starting"); // use this if long-lasting work follows ...


In the following lines you can see, how properties of your device (which must always be
part of the expectedParameters) can be read. A call to get is
always thread-safe and always returns the latest value
configured. 

.. code-block:: c++

        // Retrieve current values from our own device-state
        float tgtSpeed = get<float>("targetSpeed");
        float currentSpeed = get<float>("currentSpeed");

The next part shows one example to potentially drive your device into an Error state. Here we check, whether the conveyer stands still before starting it. Note the return statement to finish the execution of the function.

The last part of the start function simulates the ramping up by giving several updates on the "currentSpeed" property with some fixed delay. Setting a property value like here for "currentSpeed" does to things, it updates the own device
state and publishes this value to the broker, such that interested
clients will get an event.

 
