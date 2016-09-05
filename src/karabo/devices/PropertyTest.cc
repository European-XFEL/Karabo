#include "PropertyTest.hh"



namespace karabo {
    namespace devices {

        using namespace std;
        using namespace karabo::core;
        using namespace karabo::util;
        using namespace karabo::net;
        using namespace karabo::xms;


        KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, PropertyTest)


        void PropertyTest::expectedParameters(Schema& expected) {

            OVERWRITE_ELEMENT(expected).key("state")
                    .setNewOptions(State::INIT, State::STOPPING, State::STOPPED, State::STARTING, State::STARTED, State::ERROR)
                    .setNewDefaultValue(State::INIT)
                    .commit();

            SLOT_ELEMENT(expected).key("start")
                    .displayedName("Start")
                    .description("Instructs device to go to started state")
                    .allowedStates(State::STOPPED)
                    .commit();

            SLOT_ELEMENT(expected).key("stop")
                    .displayedName("Stop")
                    .description("Instructs device to go to stopped state")
                    .allowedStates(State::STARTED)
                    .commit();


            SLOT_ELEMENT(expected).key("reset")
                    .displayedName("Reset")
                    .description("Resets everything, e.g. random generator seed")
                    .allowedStates(State::ERROR, State::STOPPED)
                    .commit();

            BOOL_ELEMENT(expected).key("boolProperty")
                    .displayedName("Bool property")
                    .description("A bool property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(false)
                    .commit();

            CHAR_ELEMENT(expected).key("charProperty")
                    .displayedName("Char property")
                    .description("A char property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue('A')
                    .commit();

            INT8_ELEMENT(expected).key("int8Property")
                    .displayedName("Int8 property")
                    .description("A int8 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(77)
                    .commit();

            UINT8_ELEMENT(expected).key("uint8Property")
                    .displayedName("UInt8 property")
                    .description("A uint8 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(177)
                    .commit();

            INT16_ELEMENT(expected).key("int16Property")
                    .displayedName("Int16 property")
                    .description("A int16 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(32000)
                    .commit();

            UINT16_ELEMENT(expected).key("uint16Property")
                    .displayedName("UInt16 property")
                    .description("A uint16 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(32000)
                    .commit();

            INT32_ELEMENT(expected).key("int32Property")
                    .displayedName("Int32 property")
                    .description("A int32 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(32000000)
                    .commit();

            UINT32_ELEMENT(expected).key("uint32Property")
                    .displayedName("UInt32 property")
                    .description("A uint32 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(32000000)
                    .commit();

            INT64_ELEMENT(expected).key("int64Property")
                    .displayedName("Int64 property")
                    .description("A int64 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3200000000LL)
                    .commit();

            UINT64_ELEMENT(expected).key("uint64Property")
                    .displayedName("UInt64 property")
                    .description("A uint64 property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3200000000ULL)
                    .commit();

            FLOAT_ELEMENT(expected).key("floatProperty")
                    .displayedName("Float property")
                    .description("A float property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3.141596)
                    .commit();

            DOUBLE_ELEMENT(expected).key("doubleProperty")
                    .displayedName("Double property")
                    .description("A double property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(3.1415967773331)
                    .commit();

            STRING_ELEMENT(expected).key("stringProperty")
                    .displayedName("String property")
                    .description("A string property")
                    .reconfigurable()
                    .assignmentOptional().defaultValue("Some arbitrary text.")
                    .commit();

            VECTOR_BOOL_ELEMENT(expected).key("vectorBoolProperty")
                    .displayedName("VectorBool property")
                    .description("A vector boolean property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({true, false, true, false, true, false})
            .commit();

            VECTOR_CHAR_ELEMENT(expected).key("vectorCharProperty")
                    .displayedName("VectorChar property")
                    .description("A vector character property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({'A', 'B', 'C', 'D', 'E', 'F'})
            .commit();

            VECTOR_INT8_ELEMENT(expected).key("vectorInt8Property")
                    .displayedName("VectorInt8 property")
                    .description("A vector int8 property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({41, 42, 43, 44, 45, 46})
            .commit();

            VECTOR_UINT8_ELEMENT(expected).key("vectorUInt8Property")
                    .displayedName("VectorUInt8 property")
                    .description("A vector uint8 property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({41, 42, 43, 44, 45, 46})
            .commit();

            VECTOR_INT16_ELEMENT(expected).key("vectorInt16Property")
                    .displayedName("VectorInt16 property")
                    .description("A vector int16 property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({20041, 20042, 20043, 20044, 20045, 20046})
            .commit();

            VECTOR_UINT16_ELEMENT(expected).key("vectorUInt16Property")
                    .displayedName("VectorUInt16 property")
                    .description("A vector uint16 property")
                    .reconfigurable()
                    .minSize(1)
                    .maxSize(10)
                    .assignmentOptional().defaultValue({20041, 20042, 20043, 20044, 20045, 20046})
            .commit();



            NDARRAY_BOOL_ELEMENT(expected).key("ndaBoolProperty")
                    .displayedName("NDA Bool property")
                    .description("A ndarray bool property")
                    .reconfigurable()
                    .shape(3,2)
                    .assignmentOptional().defaultValue({true, false, true, false, true, false})
                    .commit();
        }


        PropertyTest::PropertyTest(const Hash& input) : Device<>(input) {
            KARABO_INITIAL_FUNCTION(initialize);
            KARABO_SLOT(start);
            KARABO_SLOT(stop);
            KARABO_SLOT(reset);
        }


        PropertyTest::~PropertyTest() {
        }


        void PropertyTest::initialize() {
        }


        void PropertyTest::start() {
        }


        void PropertyTest::stop() {
        }


        void PropertyTest::reset() {
        }
    }
}
