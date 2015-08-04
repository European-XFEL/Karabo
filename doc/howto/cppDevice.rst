.. _cppDevice:


******************************
 How to write a device in C++
******************************
.. sectionauthor:: Burkhard Heisen <burkhard.heisen@xfel.eu>

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
    
provides you the full Karabo framework. Both, include paths and namespaces follow the physical directory layout of the Karabo framework sources. Karabo comprises the following main functionalities (reflected as source directories):

* util: Factories, Configurator, Hash, Schema, String and Time tools, etc.
* io: Serializer, Input, Output, FileIO tools
* io/h5: HDF5 interface (HDF5, Hash serialization)
* log: unified logging using Log4Cpp as engine
* webAuth: Webservice based authentification (based on gsoap)
* net: TCP (point to point) and JMS (broker-based) networking in synchronous and asynchronous fashion.
* xms: Higher level communication API (Signals & Slots, Request/Response, etc.)
* xip: Image classes, processing, GPU code
* core: Device, DeviceServer, DeviceClient base classes

Consequently, if you want to include less, you can refer to a header of a specific functionality (like in boost, e.g. <karabo/util.hpp>, or <karabo/io.hpp>) or of a single class (e.g. <karabo/webAuth/Authenticator.hh>).

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

**NOTE**: Currently, device construction happens in the main thread of the device-server. So make sure you do not have slow or even blocking code in your constructor as it will block the whole server.

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

In the expectedParameters() function the parameters for this device are
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

Now in the initialize function (which is automatically called once the constructor finished)

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

We are almost done, start and stop are very similar and reset is
almost trivial, so lets only look
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

We simulate a slow ramping up of the speed and explicitly inform
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

The last part of the start function simulates the ramping up by giving several updates on the "currentSpeed" property with some fixed delay. Setting a property value like here for "currentSpeed" does two things, it updates the own device
state and publishes this value to the broker, such that interested
clients will get an event.


Point-to-Point Communication
============================
.. sectionauthor:: Gero Flucke <gero.flucke@xfel.eu>

The conveyor belt example is typical for a device that represents some hardware
that is controlled and monitored. No data is *produced* by these devices,
but just property changes are communicated. This is done via the central
message broker.

Other devices like cameras produce large amounts of data that cannot be
distributed this way. Instead, the data should be sent directly from one device
producing it to one or more other devices that have registered themselves
at the producer for that purpose. In the Karabo framework this can be done
using a point-to-point protocol between so called output and input channels
of devices.

Sending Data: Output Channel
----------------------------
First, we have to tell the framework what data is to be sent via the output
channel, i.e. to declare its schema.
This is done inside the ``expectedParameters`` method.
Here is an example of a device sending a 32-bit integer, a string and
a vector of 64-bit integers:

.. code-block:: c++

        Schema data;
        INT32_ELEMENT(data).key("dataId")
                .readOnly()
                .commit();

        STRING_ELEMENT(data).key("string")
                .readOnly()
                .commit();

        VECTOR_INT64_ELEMENT(data).key("vector_int64")
                .readOnly()
                .commit();

Next (but still within ``expectedParameters``), the output channel has to be
declared. Here we create one with the key *output*:

.. code-block:: c++

        OUTPUT_CHANNEL(expected).key("output")
                .displayedName("Output")
                .dataSchema(data)
                .commit();

Whenever the device should write data to this output channel, 
a ``Data`` object from <karabo/xms/Data.hh> has to be created and filled
according to the schema defined above, e.g.:

.. code-block:: c++

        Data data;
        data.set("dataId", 5);
        data.set("string", std::string("This is a string to be sent."));
        std::vector<long long> vec = ...; // create and fill the array here
        data.set("vector_int64", vec);

Note that Karabo does not (yet) check that the data sent matches the
declared schema.

Finally, the data is sent by calling the device method

.. code-block:: c++

        this->writeChannel("output", data);

with the key of the channel as the first and the ``Data`` object as the second
argument.

Once the data stream is finished, i.e. no further data is to be sent, the
end of stream method has to be called with the output channel key as argument
to inform all input channels that receive the data:

.. code-block:: c++

        this->signalEndOfStream("output");


Receiving Data: Input Channel
-----------------------------
Also for input channels oen has to declare what data they expect to receive.
This is done in exactly the same way as for output channels inside the
``expectedParameters`` method.
Declaring the input channel is also analogue to the way an output channels is
declared:

.. code-block:: c++

        INPUT_CHANNEL(expected).key("input")
                .displayedName("Input")
                .description("Input channel: client") // optional, for GUI
                .dataSchema(data)
                .commit();

The next step is to prepare a member function of the device that should be
called whenever new data arrives. The signature of that function has to be

.. code-block:: c++

   void onData(const karabo::xms::Data& data);


Inside the function the data sent can be unpacked in the following way:

.. code-block:: c++

   int id = data.get<int>("dataId");
   const std::string& str = data.get<std::string>("string");
   const vector<long long>& vec = data.get<std::vector<long long> >("vector_int64");


Finally, the framework has to be informed that this method should be called
whenever data arrives. This has to be done in the ``initialize()`` member
function (or, more precisely, in the function registered in the constructor
using the ``KARABO_INITIAL_FUNCTION`` macro) in the following way:

.. code-block:: c++

   KARABO_ON_DATA("input", onData);

with the key of the input channel as first and the function name as the second
argument.

A similar macro can be used to register a member function that should be called
when the data stream terminates, i.e. when the sending device calls 
``this->signalEndOfStream("<output channel name>");``:

.. code-block:: c++

  KARABO_ON_EOS("input", onEndOfStream);

The signature of this member function has to be

.. code-block:: c++

   void onEndOfStream(const karabo::xms::InputChannel::Pointer& input);



Hierarchies in the Schema
-------------------------

The data that is sent from an output to an input channel can have a hierarchical
structure. This structure is declared in the usual way in
``expectedParameters``, for both input and output channels:

.. code-block:: c++

        Schema data;
        // Add whatever data on first hierarchy level:
        // ...
        // First level done - now add second level:
        NODE_ELEMENT(data).key("node")
                .commit();
        
        FLOAT_ELEMENT(data).key("node.afloat")
                .readOnly()
                .commit();

When writing to an output channel, one first has to create and fill the node.
Then the node can be added and the data can be sent:

.. code-block:: c++

        Data data; // top level data structure
        // Here e.g. fill top level content:
        // ...
        Data node;
        float floatValue = 1.3f;  // or whatever...
        node.set("afloat", floatValue);
        data.setNode("node", node);
        this->writeChannel("output", data);

In the ``onData`` member function of a device receiving the data in an input
channel, the node can be unpacked in the following way:

.. code-block:: c++

    void onData(const karabo::xms::Data& data)
    {
      // ...    
      Data node(data.getNode<Data>("node"));
      const float afloat = node.get<float>("afloat");
      // ...    
    }


Treatment of Image Data
-----------------------
Image data can be sent using the class ``ImageData`` which extends the
``Data`` class by some predefined properties, i.e. it serves as a special node
with convenience methods for conversions to and from more useful image data
formats.
The schema of an output channel for image data is defined in
``expectedParameters`` as follows:

.. code-block:: c++

        Schema data;
        IMAGEDATA(data).key("image")
                .commit();

        OUTPUT_CHANNEL(expected).key("output") // or any other key
                .displayedName("Output")       // or whatever name you choose
                .dataSchema(data)
                .commit();

For input channels simply replace ``OUTPUT_CHANNEL`` by ``INPUT_CHANNEL``.

If only the image is sent and if it is available as an instance of 
``karabo::xip::CpuImage<T>``, there is a convenience method for writing
to the output channel:

.. code-block:: c++

   const karabo::xip::CpuImage<float> image = ...;  
   // Write image with key "image" to channel "output":
   this->writeChannel("output", "image", image);

Otherwise, ``ImageData`` has to be constructed properly, e.g. from
an instance of ``karabo::xip::CpuImage<T>``, and can then be treated like a
node:

.. code-block:: c++

   Data data;
   const karabo::xip::CpuImage<float> image = ...;  
   ImageData imageData(image);
   data.setNode("image", imageData);
   // Add other data if promised in the schema definition, e.g. a float:
   // data.set("afloat", 2.1f);
   this->writeChannel("output", data);

If the data is received from an input channel, the ``ImageData`` object can be
accessed similarly to a simple ``Data`` object representing a node:

.. code-block:: c++

    void onData(const karabo::xms::Data& data)
    {
      // ...    
      ImageData imageData(data.getNode<ImageData>("image"));
      // ...    
    }

Conversion methods from ``ImageData`` to an image data class like  
``karabo::xip::CpuImage<T>`` are still to be provided.


Interface *per TCP Message*
---------------------------

Point-to-point communication in the Karabo framework generally uses TCP for
data transfer between devices.
Whenever ``writeChannel`` is called for an output channel, the data is sent as
a separate message to all connected input channels.
There might be circumstances where it is advantageous to pack more than one
data item into a TCP message. For this a lower level API is provided as
described in the following.

To sent several data items in a single TCP message, the following few lines
of code should be used instead of ``this->writeChannel(channelName, data)``:

.. code-block:: c++

    data.attachTimestamp(this->getActualTimestamp());
    karabo::xms::OutputChannel::Pointer channel = this->getOutputChannel(channelName);
    channel->write(data);

Once there is enough data accumulated to be actually sent, 

.. code-block:: c++

    channel->update();

has to be called.

For a device with an input channel it does not matter much whether several
data items that it receives have been sent in a single TCP message or not.
A member function registered with ``KARABO_ON_DATA`` will be called
for each item. Nevertheless, in case it matters which data items are sent
together (which should not be the case), the device can register a method
that receives all data items in one go.
Instead of using ``KARABO_ON_DATA``, such a method has to be registered
using ``KARABO_ON_INPUT``. The signature of this method has to be 

.. code-block:: c++

   void onInput(const karabo::xms::InputChannel::Pointer& input);


Inside the method one has to loop over the data items. Finally one has to 
tell the ``InputChannel`` that reading the data is done by calling
``update()`` at the very end of the method:

.. code-block:: c++

       for (size_t i = 0; i < input->size(); ++i) {
            Data data(input->read(i));
            ... // whatever you want to do with the data
        }
        // Tell the input channel that you are done with all data
        input->update();
