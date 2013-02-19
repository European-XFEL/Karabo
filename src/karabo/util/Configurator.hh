/* 
 * File:   Configurator.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 28, 2013, 2:09 PM
 */

#ifndef KARABO_UTIL_CONFIGURATOR_HH_hh
#define	KARABO_UTIL_CONFIGURATOR_HH_hh

#include <string>

#include "Hash.hh"
#include "Factory.hh"

#include "karaboDll.hh"
#include "Schema.hh"
#include "Validator.hh"

namespace karabo {
    namespace util {

        namespace confTools {

            //**********************************************
            //               Schema Assembly               *
            //**********************************************

            template<class Class, class Argument, void (*)(Argument&) >
            struct VoidArg1FunctionExists {
            };

            template<class Class, class Argument>
            inline bool appendToSchemaIfPossible(Argument& arg, VoidArg1FunctionExists<Class, Argument, &Class::expectedParameters>*) {
                Class::expectedParameters(arg);
                return true;
            }

            template<class Class, class Argument>
            inline bool appendToSchemaIfPossible(Argument& arg, ...) {
                // Do nothing
                return false;
            }

            template <class T>
            inline Schema assembleSchema(const std::string& classId, const Schema::AssemblyRules& rules) {
                Schema schema(classId, rules);
                karabo::util::confTools::appendToSchemaIfPossible<T, Schema > (schema, 0);
                return schema;
            }

            template <class T, class U>
            inline Schema assembleSchema(const std::string& classId, const Schema::AssemblyRules& rules) {
                Schema schema(classId, rules);
                karabo::util::confTools::appendToSchemaIfPossible<T, Schema > (schema, 0);
                karabo::util::confTools::appendToSchemaIfPossible<U, Schema > (schema, 0);
                return schema;
            }

            template <class T, class U, class V>
            inline Schema assembleSchema(const std::string& classId, const Schema::AssemblyRules& rules) {
                Schema schema(classId, rules);
                karabo::util::confTools::appendToSchemaIfPossible<T, Schema > (schema, 0);
                karabo::util::confTools::appendToSchemaIfPossible<U, Schema > (schema, 0);
                karabo::util::confTools::appendToSchemaIfPossible<V, Schema > (schema, 0);
                return schema;
            }

            template <class T, class U, class V, class W>
            inline Schema assembleSchema(const std::string& classId, const Schema::AssemblyRules& rules) {
                Schema schema(classId, rules);
                karabo::util::confTools::appendToSchemaIfPossible<T, Schema > (schema, 0);
                karabo::util::confTools::appendToSchemaIfPossible<U, Schema > (schema, 0);
                karabo::util::confTools::appendToSchemaIfPossible<V, Schema > (schema, 0);
                karabo::util::confTools::appendToSchemaIfPossible<W, Schema > (schema, 0);
                return schema;
            }

            template <class T, class U, class V, class W, class X>
            inline Schema assembleSchema(const std::string& classId, const Schema::AssemblyRules& rules) {
                Schema schema(classId, rules);
                karabo::util::confTools::appendToSchemaIfPossible<T, Schema > (schema, 0);
                karabo::util::confTools::appendToSchemaIfPossible<U, Schema > (schema, 0);
                karabo::util::confTools::appendToSchemaIfPossible<V, Schema > (schema, 0);
                karabo::util::confTools::appendToSchemaIfPossible<W, Schema > (schema, 0);
                karabo::util::confTools::appendToSchemaIfPossible<X, Schema > (schema, 0);
                return schema;
            }

            //**********************************************
            //            Configure and Create             *
            //**********************************************

            template<class Class, class Argument, void (Class::*)(const Argument&) >
            struct VoidConstArg1FunctionExists {
            };

            template <class Class, class Argument>
            inline bool configureIfPossible(const typename Class::Pointer& instance, const Argument& arg, VoidConstArg1FunctionExists<Class, Argument, &Class::configure >*) {
                instance->configure(arg);
                return true;
            }

            template <class Class, class Argument>
            inline bool configureIfPossible(const typename Class::Pointer& instance, const Argument& arg, ...) {
                // Do nothing
                return false;
            }

            template <class Base>
            inline typename Base::Pointer createAndConfigure(const karabo::util::Hash& configuration) {
                typename Base::Pointer basePointer = typename Base::Pointer(new Base());
                karabo::util::confTools::configureIfPossible<Base, karabo::util::Hash > (basePointer, configuration, 0);
                return basePointer;
            }

            template <class Base, class Sub1>
            inline typename Base::Pointer createAndConfigure(const karabo::util::Hash& configuration) {
                typename Base::Pointer basePointer = typename Base::Pointer(new Sub1());
                typename Sub1::Pointer sub1Pointer = boost::static_pointer_cast<Sub1 > (basePointer);
                karabo::util::confTools::configureIfPossible<Base, karabo::util::Hash > (basePointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub1, karabo::util::Hash > (sub1Pointer, configuration, 0);
                return basePointer;
            }

            template <class Base, class Sub1, class Sub2>
            typename Base::Pointer createAndConfigure(const karabo::util::Hash& configuration) {
                typename Base::Pointer basePointer = typename Base::Pointer(new Sub2());
                typename Sub1::Pointer sub1Pointer = boost::static_pointer_cast<Sub1 > (basePointer);
                typename Sub2::Pointer sub2Pointer = boost::static_pointer_cast<Sub2 > (basePointer);
                karabo::util::confTools::configureIfPossible<Base, karabo::util::Hash > (basePointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub1, karabo::util::Hash > (sub1Pointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub2, karabo::util::Hash > (sub2Pointer, configuration, 0);
                return basePointer;
            }

            template <class Base, class Sub1, class Sub2, class Sub3>
            inline typename Base::Pointer createAndConfigure(const karabo::util::Hash& configuration) {
                typename Base::Pointer basePointer = typename Base::Pointer(new Sub3());
                typename Sub1::Pointer sub1Pointer = boost::static_pointer_cast<Sub1 > (basePointer);
                typename Sub2::Pointer sub2Pointer = boost::static_pointer_cast<Sub2 > (basePointer);
                typename Sub3::Pointer sub3Pointer = boost::static_pointer_cast<Sub3 > (basePointer);
                karabo::util::confTools::configureIfPossible<Base, karabo::util::Hash > (basePointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub1, karabo::util::Hash > (sub1Pointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub2, karabo::util::Hash > (sub2Pointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub3, karabo::util::Hash > (sub3Pointer, configuration, 0);
                return basePointer;
            }

            template <class Base, class Sub1, class Sub2, class Sub3, class Sub4>
            inline typename Base::Pointer createAndConfigure(const karabo::util::Hash& configuration) {
                typename Base::Pointer basePointer = typename Base::Pointer(new Sub4());
                typename Sub1::Pointer sub1Pointer = boost::static_pointer_cast<Sub1 > (basePointer);
                typename Sub2::Pointer sub2Pointer = boost::static_pointer_cast<Sub2 > (basePointer);
                typename Sub3::Pointer sub3Pointer = boost::static_pointer_cast<Sub3 > (basePointer);
                typename Sub4::Pointer sub4Pointer = boost::static_pointer_cast<Sub4 > (basePointer);
                karabo::util::confTools::configureIfPossible<Base, karabo::util::Hash > (basePointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub1, karabo::util::Hash > (sub1Pointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub2, karabo::util::Hash > (sub2Pointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub3, karabo::util::Hash > (sub3Pointer, configuration, 0);
                karabo::util::confTools::configureIfPossible<Sub4, karabo::util::Hash > (sub4Pointer, configuration, 0);
                return basePointer;
            }

            inline std::pair<std::string, karabo::util::Hash> spitIntoClassIdAndConfiguration(const karabo::util::Hash& rootedConfiguration) {
                if (rootedConfiguration.size() != 1) throw KARABO_LOGIC_EXCEPTION("Expecting exactly one (root-)node identifying the classId in configuration");
                std::string classId = rootedConfiguration.begin()->getKey();
                karabo::util::Hash config = rootedConfiguration.begin()->getValue<Hash > ();
                return std::make_pair(classId, config);
            }
        }


        //**********************************************
        //               Configurator                  *
        //**********************************************

        template <class Base>
        class Configurator {
        public:

            KARABO_CLASSINFO(Configurator<Base>, "Configurator", "1.0");

            inline static std::vector<std::string> getRegisteredClasses() {
                std::vector<std::string> registeredClasses;
                return Factory< Configurator<Base> >::getRegisteredClasses();
            }

            inline static typename Base::Pointer create(const karabo::util::Hash& configuration, const bool validate = true) {
                try {
                    std::pair<std::string, karabo::util::Hash> p = karabo::util::confTools::spitIntoClassIdAndConfiguration(configuration);
                    return create(p.first, p.second, validate);
                } catch (const LogicException& e) {
                    KARABO_RETHROW_AS(KARABO_INIT_EXCEPTION("This create method expects a rooted Hash with the root node name specifying the classId"));
                    return typename Base::Pointer(); // Make the compiler happy
                }
            }

            inline static typename Base::Pointer create(const std::string& classId, const karabo::util::Hash& configuration, const bool validate = true) {
                Pointer p = Factory<Configurator<Base> >::create(classId); // This creates a configurator for desired class
                if (validate) {
                    Schema schema = p->assemble(classId, Schema::AssemblyRules());
                    Validator validator; // Default validation
                    Hash validated;
                    std::pair<bool, std::string> ret = validator.validate(schema, configuration, validated);
                    if (ret.first == false) throw KARABO_PARAMETER_EXCEPTION("Validation failed. \n" + ret.second);
                    return p->createAndConfigure(validated); // This creates the desired class (by zero/default construction)
                } else {
                    return p->createAndConfigure(configuration);
                }
            }

            inline static typename Base::Pointer createNode(const std::string& nodeName, const std::string& classId, const karabo::util::Hash& input, const bool validate = true) {
                if (input.has(nodeName)) {
                    return create(classId, input.get<Hash > (nodeName), validate);
                } else {
                    throw KARABO_INIT_EXCEPTION("Given nodeName \"" + nodeName + "\" is not part of input configuration");
                }
            }

            inline static typename Base::Pointer createChoice(const std::string& choiceName, const karabo::util::Hash& input, const bool validate = true) {
                if (input.has(choiceName)) {
                    // choiceName should have a Hash which has exactly one key that associates a hash
                    return create(input.get<Hash > (choiceName), validate);
                } else {
                    throw KARABO_INIT_EXCEPTION("Given choiceName \"" + choiceName + "\" is not part of input configuration");
                }
            }

            inline static std::vector<typename Base::Pointer> createList(const std::string& listName, const karabo::util::Hash& input, const bool validate = true) {
                if (input.has(listName)) {
                    const Hash& tmp = input.get<Hash > (listName);
                    std::vector<typename Base::Pointer> instances;
                    for (Hash::const_iterator it = tmp.begin(); it != tmp.end(); ++it) {
                        instances.push_back(create(it->getKey(), it->getValue<Hash > (), validate));
                    }
                } else {
                    throw KARABO_INIT_EXCEPTION("Given listName \"" + listName + "\" is not part of input configuration");
                }
            }

            inline static Schema assembleSchema(const std::string& classId, const Schema::AssemblyRules& rules = Schema::AssemblyRules()) {
                Pointer p = Factory<Configurator<Base> >::create(classId);
                return p->assemble(classId, rules);
            }

        protected:

            virtual Schema assemble(const std::string& classId, const Schema::AssemblyRules& rules) = 0;
            virtual typename Base::Pointer createAndConfigure(const karabo::util::Hash& configuration) = 0;
        };
        
        
        template <class Base>
        struct Configurator0 : public Configurator<Base> {

            Schema assemble(const std::string& classId, const Schema::AssemblyRules & rules) {
                return karabo::util::confTools::assembleSchema<Base> (classId, rules);
            }

            typename Base::Pointer createAndConfigure(const karabo::util::Hash & configuration) {
                return karabo::util::confTools::createAndConfigure<Base> (configuration);
            }
        };

        template <class Base, typename Sub1>
        struct Configurator1 : public Configurator<Base> {

            Schema assemble(const std::string& classId, const Schema::AssemblyRules & rules) {
                return karabo::util::confTools::assembleSchema<Base, Sub1 > (classId, rules);
            }

            typename Base::Pointer createAndConfigure(const karabo::util::Hash & configuration) {
                return karabo::util::confTools::createAndConfigure<Base, Sub1 > (configuration);
            }
        };

        template <class Base, class Sub1, class Sub2>
        struct Configurator2 {

            Schema assemble(const std::string& classId, const Schema::AssemblyRules & rules) {
                return karabo::util::confTools::assembleSchema<Base, Sub1, Sub2 > (classId, rules);
            }

            typename Base::Pointer createAndConfigure(const karabo::util::Hash & configuration) {
                return karabo::util::confTools::createAndConfigure<Base, Sub1, Sub2 > (configuration);
            }
        };

        template <class Base, class Sub1, class Sub2, class Sub3>
        struct Configurator3 {

            Schema assemble(const std::string& classId, const Schema::AssemblyRules & rules) {
                return karabo::util::confTools::assembleSchema<Base, Sub1, Sub2, Sub3 > (classId, rules);
            }

            typename Base::Pointer createAndConfigure(const karabo::util::Hash & configuration) {
                return karabo::util::confTools::createAndConfigure<Base, Sub1, Sub2, Sub3 > (configuration);
            }
        };

        template <class Base, class Sub1, class Sub2, class Sub3, class Sub4>
        struct Configurator4 {

            Schema assemble(const std::string& classId, const Schema::AssemblyRules & rules) {
                return karabo::util::confTools::assembleSchema<Base, Sub1, Sub2, Sub3, Sub4 > (classId, rules);
            }

            typename Base::Pointer createAndConfigure(const karabo::util::Hash & configuration) {
                return karabo::util::confTools::createAndConfigure<Base, Sub1, Sub2, Sub3, Sub4 > (configuration);
            }
        };

        /**
         * If we are importing symbols from a dll in windows, we have to tell the compiler that he should use a single
         * "version" of our templated factory only. This happens through KARABO_TEMPLATE_DLL which resolves to "extern"
         * in case of dll import. If karaboFramework is compiled into a dll, the symbols are exported, if we have an
         * application linking against the karabo dll, the symbols are imported and templates must be flagged extern!
         */
        #ifdef _WIN32
        #ifdef __DLL__
        #define KARABO_CONFIGURATION_BASE_CLASS \
                static boost::shared_ptr<Self> create(const karabo::util::Hash& configuration) { \
                return karabo::util::Configurator<Self>::create(configuration); } \
                \
                static boost::shared_ptr<Self> create(const std::string& classId, const karabo:::util::Hash& configuration) { \
                return karabo::util::Configurator<Self>::create(classId, configuration); } \
                \
                static boost::shared_ptr<Self> createNode(const std::string& nodeName, const std::string& classId, const karabo::util::Hash& input) { \
                return karabo::util::Configurator<Self>::createNode(nodeName, classId, input); } \
                \
                static boost::shared_ptr<Self> createChoice(const std::string& choiceName, const karabo::util::Hash& input) { \
                return karabo::util::Configurator<Self>::createChoice(choiceName, input); } \
                \
                static std::vector<boost::shared_ptr<Self> > createList(const std::string& listName, const karabo::util::Hash& input) { \
                return karabo::util::Configurator<Self>::createList(listName, input); } \
                \
                template class __declspec(dllexport) karabo::util::Factory< karabo::util::Configurator<Self> >;
        #else
        #define KARABO_CONFIGURATION_BASE_CLASS \
                static boost::shared_ptr<Self> create(const karabo::util::Hash& configuration) { \
                return karabo::util::Configurator<Self>::create(configuration); } \
                \
                static boost::shared_ptr<Self> create(const std::string& classId, const karabo:::util::Hash& configuration) { \
                return karabo::util::Configurator<Self>::create(classId, configuration); } \
                \
                static boost::shared_ptr<Self> createNode(const std::string& nodeName, const std::string& classId, const karabo::util::Hash& input) { \
                return karabo::util::Configurator<Self>::createNode(nodeName, classId, input); } \
                \
                static boost::shared_ptr<Self> createChoice(const std::string& choiceName, const karabo::util::Hash& input) { \
                return karabo::util::Configurator<Self>::createChoice(choiceName, input); } \
                \
                static std::vector<boost::shared_ptr<Self> > createList(const std::string& listName, const karabo::util::Hash& input) { \
                return karabo::util::Configurator<Self>::createList(listName, input); } \
                \
                extern template class __declspec(dllimport) karabo::util::Factory< karabo::util::Configurator<Self> >;
        #endif
        #else
        #define KARABO_CONFIGURATION_BASE_CLASS \
                static boost::shared_ptr<Self> create(const karabo::util::Hash& configuration) { \
                return karabo::util::Configurator<Self>::create(configuration); } \
                \
                static boost::shared_ptr<Self> create(const std::string& classId, const karabo::util::Hash& configuration) { \
                return karabo::util::Configurator<Self>::create(classId, configuration); } \
                \
                static boost::shared_ptr<Self> createNode(const std::string& nodeName, const std::string& classId, const karabo::util::Hash& input) { \
                return karabo::util::Configurator<Self>::createNode(nodeName, classId, input); } \
                \
                static boost::shared_ptr<Self> createChoice(const std::string& choiceName, const karabo::util::Hash& input) { \
                return karabo::util::Configurator<Self>::createChoice(choiceName, input); } \
                \
                static std::vector<boost::shared_ptr<Self> > createList(const std::string& listName, const karabo::util::Hash& input) { \
                return karabo::util::Configurator<Self>::createList(listName, input); }
        #endif



        #define KARABO_REGISTER_FOR_CONFIGURATION_1(Base) \
                template<> const karabo::util::FactoryMember0<karabo::util::Configurator<Base>, karabo::util::Configurator0<Base> > \
                karabo::util::Register0<karabo::util::Configurator<Base>, karabo::util::Configurator0<Base> >::registerAs(Base::classInfo().getClassId());
        
        #define KARABO_REGISTER_FOR_CONFIGURATION_2(Base, Sub1) \
                template<> const karabo::util::FactoryMember0<karabo::util::Configurator<Base>, karabo::util::Configurator1<Base, Sub1> > \
                karabo::util::Register0<karabo::util::Configurator<Base>, karabo::util::Configurator1<Base, Sub1> >::registerAs(Sub1::classInfo().getClassId());

        #define KARABO_REGISTER_FOR_CONFIGURATION_3(Base, Sub1, Sub2) \
                template<> const karabo::util::FactoryMember0<karabo::util::Configurator<Base>, karabo::util::Configurator2<Base, Sub1, Sub2> > \
                karabo::util::Register0<karabo::util::Configurator<Base>, karabo::util::Configurator2<Base, Sub1, Sub2> >::registerAs(Sub2::classInfo().getClassId());

        #define KARABO_REGISTER_FOR_CONFIGURATION_4(Base, Sub1, Sub2, Sub3) \
                template<> const karabo::util::FactoryMember0<karabo::util::Configurator<Base>, karabo::util::Configurator3<Base, Sub1, Sub2, Sub3> > \
                karabo::util::Register0<karabo::util::Configurator<Base>, karabo::util::Configurator3<Base, Sub1, Sub2, Sub3> >::registerAs(Sub3::classInfo().getClassId());

        #define KARABO_REGISTER_FOR_CONFIGURATION_5(Base, Sub1, Sub2, Sub3, Sub4) \
                template<> const karabo::util::FactoryMember0<karabo::util::Configurator<Base>, karabo::util::Configurator4<Base, Sub1, Sub2, Sub3, Sub4> > \
                karabo::util::Register0<karabo::util::Configurator<Base>, karabo::util::Configurator4<Base, Sub1, Sub2, Sub3, Sub4> >::registerAs(Sub3::classInfo().getClassId());

    }
}
#endif
