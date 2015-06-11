.. _cppDevice:


******************************
 How to write a device in C++
******************************

The "Hello World" device
========================

Let us start by having a look to the header of the HelloWorld device...

.. code-block:: c++

    #ifndef KARABO_HELLOWORLD_HH
    #define KARABO_HELLOWORLD_HH
    
    #include <karabo/karabo.hpp>
    
    /**
     * The main Karabo namespace
     */
    namespace karabo {
    
        class HelloWorld : public karabo::core::Device<karabo::core::OkErrorFsm> {
    
        public:
    
            // Adds reflection and versioning information to the class
            KARABO_CLASSINFO(HelloWorld, "HelloWorld", "1.3")
    
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
            HelloWorld(const karabo::util::Hash& config);
    
            virtual ~HelloWorld();
           
            /**
             * This function acts as a hook and is called after an reconfiguration request was received,
             * but BEFORE this reconfiguration request is actually merged into this device's state.
             * 
             * The reconfiguration information is contained in the Hash object provided as an argument.
             * You have a chance to change the content of this Hash before it is merged into the device's current state.
             * 
             * NOTE: (a) the incomingReconfiguration was validated before
             *       (b) if you need not to handle the reconfigured data, there is no need to implement this function.
             *           the reconfiguration will automatically be applied to the current state.
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
            
        private: // State-machine call-backs (override)
    
        private: // Functions
    
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
    
        class HelloWorld : public karabo::core::Device<karabo::core::OkErrorFsm> {

Any Device must in the end derive from the templated class Device<>, the template indicating which :ref:`state machine <stateMachines>` to use. If you think you need no state-machine at all you should, like in this example, use the OkErrorFsm. It consists of only two states (Ok and Error) and provides a reset event to recover from error. If you leave the template empty, a custom state-machine must be defined within the device itself. We will look at such an example later.

The KARABO_CLASSINFO macro

.. code-block:: c++

    KARABO_CLASSINFO(HelloWorld, "HelloWorld", "1.3")


adds what C++ does not provide by default: reflection (or introspection) information. It for example defines

.. code-block:: c++

    typedef Self HelloWorld;

this is convenient in for example generic template code. Even more important is the string identifier for the class, called **classId**. The configurator system will utilize this information for factory-like object construction. The last argument (1.3) tells with which Karabo framework version the Device is compatible.

The expected parameter function

.. code-block:: c++

    static void expectedParameters(karabo::util::Schema& expected);

is the one and only place where you should describe what properties and commands are available on this device. The function is static in order to be parsed before instantiation time and to generate meaningful graphical widgets that guide users to set up the initial configuration. This function will be called several times (whenever some other party needs to know about your configuration information).

The constructor

.. code-block:: c++

    HelloWorld(const karabo::util::Hash& config);

will be called-back by the configurator mechanism. It else is a regular constructor.

**NOTE**: Currently, device construction happends in the main thread of the device-server. So make sure you do not have slow or even blocking code in your constructor as it will block the whole server.

If you opened any threads yourself in the device or allocated heaped memory that you have to free, the destructor is the place for doing so. It is guaranteed to be called, whenever a device instance gets killed.

.. code-block:: c++

    virtual ~HelloWorld();

The preReconfigure and postReconfigure functions,

.. code-block:: c++

    virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration);
    virtual void postReconfigure()

are called after a reconfiguration request was received, respectively *before* and *after* it has been merged into the device's state.

Karabo conceptually distinguishes between execution of commands (state-machine event triggers) and settings of properties. Execution of commands intend to be followed by a state change as described in each device's transition table, whilst property settings **should not** lead to a state change. Example: Moving a motor would utilize a property setting and a command. First a "targetPosition" property would be set (no state change), and afterwards a "move" command would be issued which really triggers the state-machine and drives it into "Moving" state. This conceptual separation is reflected into the API and the two functions above reflect the hook into the property configuration system. They will be called any time an external user thinks he wants to reconfigure something.

Now let us have a look at the implementation, here is the complete file

.. code-block:: c++

    #include "HelloWorld.hh"
    
    using namespace std;
    USING_KARABO_NAMESPACES
    
    namespace karabo {
    
        KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>, HelloWorld)
    
        void HelloWorld::expectedParameters(Schema& expected) {
    
            STRING_ELEMENT(expected).key("word1")
                    .displayedName("Word One")
                    .description("The first word")
                    .assignmentOptional().defaultValue("Hello")
                    .reconfigurable()
                    .commit();
    
            STRING_ELEMENT(expected).key("word2")
                    .displayedName("Word Two")
                    .description("The second word")
                    .assignmentOptional().defaultValue("World")
                    .reconfigurable()
                    .commit();
    
            STRING_ELEMENT(expected).key("result")
                    .displayedName("Result")
                    .description("The resultant string")
                    .readOnly()
                    .commit();
    
            NODE_ELEMENT(expected).key("catenation")
                    .displayedName("Composition")
                    .description("Composition options")
                    .commit();
    
            BOOL_ELEMENT(expected).key("catenation.reverse")
                    .displayedName("Reverse")
                    .description("Reverse the order of word catenation")
                    .assignmentOptional().defaultValue(false)
                    .reconfigurable()
                    .commit();
    
            INT32_ELEMENT(expected).key("catenation.characterCount")
                    .displayedName("Character count")
                    .description("The total number of characters of concatenated word")
                    .readOnly()
                    .initialValue(0)
                    .alarmHigh(20)
                    .alarmLow(2)
                    .commit();
        }
    
    
        HelloWorld::HelloWorld(const karabo::util::Hash& config) : Device<OkErrorFsm>(config) {
        }
    
    
        HelloWorld::~HelloWorld() {
            KARABO_LOG_DEBUG << "HelloWorldDevice destructor was called";
        }
    
    
        void HelloWorld::preReconfigure(karabo::util::Hash& incomingReconfiguration) {
        }
    
    
        void HelloWorld::postReconfigure() {
            string result;
            if (get<bool>("catenation.reverse")) {
                result = get<string>("word2") + " " + get<string>("word1");
            } else {
                result = get<string>("word1") + " " + get<string>("word2");
            }
            int count = result.size();
    
            // Here we can send in two ways: 1) one by one 2) pre-packed Hash
            // The second variant performs better, both work
    
            //set("result", result);
            //set("catenation.characterCount", count);
    
            set(Hash("result", result, "catenation.characterCount", count));
    
        }
    
    }

and go through it step by step.

The macro

.. code-block:: c++

    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>, HelloWorld)

registers the device to BaseDevice configurator factory. The expected parameters of all classes mentioned in this macro will be evaluated an concatenated from left to right. In this way our HelloWord device inherits all expected parameters from BaseDevice (which has none), and from Device<OkErrorFsm> (which has a few).

	In the expectedParameter() function the parameters for this device are defined. See :ref:`here <cppSchema>` for more details of how doing so.

The constructor

.. code-block:: c++

    HelloWorld::HelloWorld(const karabo::util::Hash& config) : Device<OkErrorFsm>(config) {}

has no code, despite calling the parent class' constructor (as is proper C++). This is completely fine for two reasons:
1. The provided configuration got validated BEFORE the constructor was even called.
2. The Device<> base class keeps the configuration (actually in form of a :ref:`Hash <cppHash>`) and gives access to it with getters and setters.

Of course you can create a member variable and assign it by using the value in the provided configuration, like:

.. code-block:: c++

    HelloWorld::HelloWorld(const karabo::util::Hash& config) : Device<OkErrorFsm>(config) {
        m_word1 = config.get<string>("word1");
    }

but then you have to be careful to keep the variable in sync! You should update it yourself in the postReconfiguration() function. We generally recommend not to keep any private members being copies of the configuration variables. Karabo's setters and getters will do fine and perform fast enough for most of the cases.

The postReconfigure() function in this example is implemented to update a depended property, the "result" string. We have to react, whenever the value of "word1" or "word2" changes. For exactly this reason the two hooks to the property configuration exist.

**NOTE:** The set() function does not only update the value within the your device instance but will also inform the whole distributed system and also check for possibly set warn or alarm thresholds and trigger them if reached.
