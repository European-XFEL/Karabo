/*
 * File:   Configurator.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January 28, 2013, 2:09 PM
 */

#ifndef KARABO_UTIL_CONFIGURATOR_HH
#define KARABO_UTIL_CONFIGURATOR_HH

#include <string>

#include "Factory.hh"
#include "Hash.hh"
#include "Schema.hh"
#include "Validator.hh"
#include "karaboDll.hh"

namespace karabo {
    namespace util {
        namespace confTools {

/**
 * @define the name of the schema description function in Karabo
 */
#define _KARABO_SCHEMA_DESCRIPTION_FUNCTION expectedParameters
            typedef void (*PointerToSchemaDescriptionFunction)(Schema&);

            //**********************************************
            //               Schema Assembly               *
            //**********************************************

            template <class Class, class Argument, void (*)(Argument&)>
            struct VoidArg1FunctionExists {};

            /**
             * Return a pointer to a valid schema description function
             * @param
             * @return
             */
            template <class Class>
            inline PointerToSchemaDescriptionFunction getSchemaDescriptionFunction(
                  VoidArg1FunctionExists<Class, Schema, &Class::_KARABO_SCHEMA_DESCRIPTION_FUNCTION>*) {
                return &Class::_KARABO_SCHEMA_DESCRIPTION_FUNCTION;
            }

            /**
             * Return a NULL-Pointer if no schema description function exists
             * @param ...
             * @return
             */
            template <class Class>
            inline PointerToSchemaDescriptionFunction getSchemaDescriptionFunction(...) {
                return 0;
            }

            /**
             * Split a configuration into classId and configuration Hash
             * @param rootedConfiguration
             * @return a pair of class id and configuration Hash
             */
            inline std::pair<std::string, karabo::util::Hash> splitIntoClassIdAndConfiguration(
                  const karabo::util::Hash& rootedConfiguration) {
                if (rootedConfiguration.size() != 1)
                    throw KARABO_LOGIC_EXCEPTION(
                          "Expecting exactly one (root-)node identifying the classId in configuration");
                std::string classId = rootedConfiguration.begin()->getKey();
                karabo::util::Hash config = rootedConfiguration.begin()->getValue<Hash>();
                return std::make_pair(classId, config);
            }
        } // namespace confTools


        /**
         * @class Configurator
         * @brief The Configurator provides for creating and configuring factorized classes.
         */
        template <class BaseClass>
        class Configurator {
            typedef std::map<std::string, boost::any> CtorMap;
            typedef std::map<std::string, CtorMap> Registry;
            typedef std::vector<boost::function<void(Schema&)>> SchemaFuncs;
            typedef std::map<std::string, SchemaFuncs> SchemaFuncRegistry;

            Registry m_registry;
            SchemaFuncRegistry m_schemaFuncRegistry;
            std::string m_default;

           public:
            KARABO_CLASSINFO(Configurator<BaseClass>, "Configurator", "1.0");

            /**
             * Register a base class with standard Hash configuration constructor into the factory
             * @param classId identifying the class in the factory
             */
            template <class DerivedClass>
            static void registerClass(const std::string& classId) {
                // std::cout << "Registering class \"" << classId << "\" with constructor: " << classId << "("
                //           << ctorKey() << ") for configuration" << std::endl;

                // Map of constructors for classId
                CtorMap& ctrs = Configurator::init().m_registry[classId];

                // Try to insert construction method
                const std::pair<CtorMap::iterator, bool> ctrIt_isInserted =
                      ctrs.insert({ctorKey(), static_cast<boost::function<boost::shared_ptr<BaseClass>(const Hash&)>>(
                                                    boost::factory<boost::shared_ptr<DerivedClass>>())});

                if (!ctrIt_isInserted.second) { // So there was already something in the map
                    // Registering same constructor type again for a derived class smells like asking for trouble:
                    // Could come when loading different libs witht different versions of the same class
                    std::cerr
                          << "WARN: Refuse to register constructor key '" << ctorKey()
                          << "' a second time for class '" + classId + "'!\n"
                          << "      Better check whether different libraries provide different versions of that class."
                          << std::endl;
                }
            }

            /**
             * Register a class having constructor with additional A1 type parameter (besides the standard Hash
             * configuration) into the factory
             * @param classId identifying the class in the factory
             */
            template <class DerivedClass, typename A1>
            static void registerClass(const std::string& classId) {
                // std::cout << "Registering class \"" << classId << "\" with constructor: " << classId << "("
                //           << ctorKey<A1>() << ") for configuration" << std::endl;

                // Map of constructors for classId
                CtorMap& ctrs = Configurator::init().m_registry[classId];

                // Try to insert construction method
                const std::pair<CtorMap::iterator, bool> ctrIt_isInserted =
                      ctrs.insert({ctorKey<A1>(),
                                   static_cast<boost::function<boost::shared_ptr<BaseClass>(const Hash&, const A1&)>>(
                                         boost::factory<boost::shared_ptr<DerivedClass>>())});

                if (!ctrIt_isInserted.second) { // So there was already something in the map
                    // Registering same constructor type again for a derived class smells like asking for trouble:
                    // Could come when loading different libs with different versions of the same class
                    std::cerr
                          << "WARN: Refuse to register constructor key '" << ctorKey<A1>()
                          << "' a second time for class '" + classId + "'!\n"
                          << "      Better check whether different libraries provide different versions of that class."
                          << std::endl;
                }
            }

            /**
             * Register the schema decription function for classId in the factory
             * @param classId identifying the class in the factory
             */
            template <class T>
            static void registerSchemaFunction(const std::string& classId) {
                confTools::PointerToSchemaDescriptionFunction p = confTools::getSchemaDescriptionFunction<T>(0);
                if (p) Configurator::init().m_schemaFuncRegistry[classId].push_back(p);
            }

            /**
             * Set the default class id of the factory
             * @param classId
             */
            static void setDefault(const std::string& classId) {
                Configurator::init().m_default = classId;
            }

            /**
             * Get the schema defining a factorized class
             * @param classId identifying the class in the factory
             * @param rules defining how to assembly the returned Schema
             * @return
             */
            static Schema getSchema(const std::string& classId,
                                    const Schema::AssemblyRules& rules = Schema::AssemblyRules()) {
                Schema schema(classId, rules);
                SchemaFuncRegistry::const_iterator it = Configurator::init().m_schemaFuncRegistry.find(classId);
                if (it != Configurator::init().m_schemaFuncRegistry.end()) {
                    const SchemaFuncs& schemaFunctions = it->second;
                    for (size_t i = 0; i < schemaFunctions.size(); ++i) {
                        if (!schemaFunctions[i].empty()) {
                            schemaFunctions[i](schema);
                        }
                    }
                }
                return schema;
            }

            /**
             * Create an Object of the default class of this factory
             * @param validate
             * @return a pointer to the created object
             */
            inline static typename BaseClass::Pointer createDefault(const bool validate = true) {
                std::string defaultClassId = Configurator::init().m_default;
                if (defaultClassId.empty()) throw KARABO_INIT_EXCEPTION("No default was defined");
                return create(defaultClassId, Hash(), validate);
            }

            /**
             * Create an object as described by configuration from the factory
             * @param configuration where the root-nodes key identifies the classId
             * @param validate if true, validate the configuration against the classes Schema. Raises an exception if
             * validation fails
             * @return a pointer to the created object
             */
            inline static typename BaseClass::Pointer create(const karabo::util::Hash& configuration,
                                                             const bool validate = true) {
                try {
                    std::pair<std::string, karabo::util::Hash> p =
                          karabo::util::confTools::splitIntoClassIdAndConfiguration(configuration);
                    return create(p.first, p.second, validate);
                } catch (const LogicException& e) {
                    KARABO_RETHROW_AS(KARABO_INIT_EXCEPTION(
                          "This create method expects a rooted Hash with the root node name specifying the classId"));
                    return typename BaseClass::Pointer(); // Make the compiler happy
                }
            }

            /**
             * Create an object of classId from the factory, assign default values as given by the class Schema
             * @param configuration Hash containing the configuration
             * @param validate if true, validate the configuration against the classes Schema. Raises an exception if
             * validation fails
             * @return a pointer to the created object
             */
            inline static typename BaseClass::Pointer create(const std::string& classId,
                                                             const karabo::util::Hash& configuration = Hash(),
                                                             const bool validate = true) {
                CtorMap::const_iterator it = findCtor(classId, ctorKey());
                if (validate) {
                    Hash validated;
                    validateConfiguration(classId, configuration, validated);
                    return (boost::any_cast<boost::function<boost::shared_ptr<BaseClass>(const Hash&)>>(it->second))(
                          validated);
                } else {
                    return (boost::any_cast<boost::function<boost::shared_ptr<BaseClass>(const Hash&)>>(it->second))(
                          configuration);
                }
            }

            /**
             * Create an object  as described by configuration from the factory
             * @param configuration where the root-nodes key identifies the classId
             * @param arbitrary type parameter to be passed to the constructor
             * @param validate if true, validate the configuration against the classes Schema. Raises an exception if
             * validation fails
             * @return a pointer to the base class of created object
             */
            template <typename A1>
            inline static typename BaseClass::Pointer create(const karabo::util::Hash& configuration, const A1& a1,
                                                             const bool validate = true) {
                try {
                    std::pair<std::string, karabo::util::Hash> p =
                          karabo::util::confTools::splitIntoClassIdAndConfiguration(configuration);
                    return create(p.first, p.second, a1, validate);
                } catch (const LogicException& e) {
                    KARABO_RETHROW_AS(KARABO_INIT_EXCEPTION(
                          "This create method expects a rooted Hash with the root node name specifying the classId"));
                    return typename BaseClass::Pointer(); // Make the compiler happy
                }
            }

            /**
             * Create an object of classId as described by configuration from the factory
             * @param configuration Hash containing the configuration
             * @param arbitrary type parameter to be passed to the constructor
             * @param validate if true, validate the configuration against the classes Schema. Raises an exception if
             * validation fails
             * @return a pointer to the base class of created object
             */
            template <typename A1>
            inline static typename BaseClass::Pointer create(const std::string& classId,
                                                             const karabo::util::Hash& configuration, const A1& a1,
                                                             const bool validate = true) {
                CtorMap::const_iterator it = findCtor(classId, ctorKey<A1>());
                if (validate) {
                    Hash validated;
                    validateConfiguration(classId, configuration, validated);
                    return (boost::any_cast<boost::function<boost::shared_ptr<BaseClass>(const Hash&, const A1&)>>(
                          it->second))(validated, a1);
                } else {
                    return (boost::any_cast<boost::function<boost::shared_ptr<BaseClass>(const Hash&, const A1&)>>(
                          it->second))(configuration, a1);
                }
            }

            /**
             * Use this function to create a configurable object as part of a parent one (aggregation).
             *
             * The input configuration may contain regular Hash parameters under the key nodeName or
             * an already instantiated object of type BaseClass::Pointer.
             *
             * This signature of this function allows to specify a classId in case the Aggregate itself is a
             * derivative of BaseClass.
             *
             * @param nodeName The key name of the NODE_ELEMENT as defined by the parent class
             * @param classId The factory key of the to be created object (must inherit the BaseClass template)
             * @param input The input configuration of the parent class
             * @param validate Whether to validate or not
             * @return Shared pointer of the created object
             */
            inline static typename BaseClass::Pointer createNode(const std::string& nodeName,
                                                                 const std::string& classId,
                                                                 const karabo::util::Hash& input,
                                                                 const bool validate = true) {
                if (input.has(nodeName)) {
                    if (input.is<typename BaseClass::Pointer>(nodeName)) {
                        return input.get<typename BaseClass::Pointer>(nodeName);
                    } else {
                        return create(classId, input.get<Hash>(nodeName), validate);
                    }
                }
                throw KARABO_INIT_EXCEPTION("Given nodeName \"" + nodeName + "\" is not part of input configuration");
            }

            /**
             * Use this function to create a configurable object of class template type as part of a parent one
             * (aggregation).
             *
             * The input configuration may contain regular Hash parameters under the key nodeName or
             * an already instantiated object of type BaseClass::Pointer.
             *
             * @param nodeName The key name of the NODE_ELEMENT as defined by the parent class
             * @param input The input configuration of the parent class
             * @param validate Whether to validate or not
             * @return Shared pointer of the created object
             */
            inline static typename BaseClass::Pointer createNode(const std::string& nodeName,
                                                                 const karabo::util::Hash& input,
                                                                 const bool validate = true) {
                return createNode(nodeName, BaseClass::classInfo().getClassId(), input, validate);
            }

            /**
             * Create object from a choice of factorized classes as defined by choiceName from input configuration
             * @param choiceName
             * @param input
             * @param validate
             * @return
             */
            inline static typename BaseClass::Pointer createChoice(const std::string& choiceName,
                                                                   const karabo::util::Hash& input,
                                                                   const bool validate = true) {
                if (input.has(choiceName)) {
                    return create(input.get<Hash>(choiceName), validate);
                } else {
                    throw KARABO_INIT_EXCEPTION("Given choiceName \"" + choiceName +
                                                "\" is not part of input configuration");
                }
            }

            /**
             * Create a list of factorized classes as defined by input configuration. Classes need to be of the same
             * Base class
             * @param choiceName
             * @param input
             * @param validate
             * @return
             */
            inline static std::vector<typename BaseClass::Pointer> createList(const std::string& listName,
                                                                              const karabo::util::Hash& input,
                                                                              const bool validate = true) {
                if (input.has(listName)) {
                    const std::vector<Hash>& tmp = input.get<std::vector<Hash>>(listName);
                    std::vector<typename BaseClass::Pointer> instances(tmp.size());
                    for (size_t i = 0; i < tmp.size(); ++i) {
                        instances[i] = create(tmp[i], validate);
                    }
                    return instances;
                } else {
                    throw KARABO_INIT_EXCEPTION("Given listName \"" + listName +
                                                "\" is not part of input configuration");
                }
            }

            /**
             * Return a vector of classIds registered in this Configurator
             * @return
             */
            static std::vector<std::string> getRegisteredClasses() {
                std::vector<std::string> registeredClasses;
                for (Registry::const_iterator it = Configurator::init().m_registry.begin();
                     it != Configurator::init().m_registry.end(); ++it) {
                    registeredClasses.push_back(it->first);
                }
                return registeredClasses;
            }

            /**
             * Trigger a validation of class BaseClass (or a derivative) against the Schema as provided by the
             * static expectedParameters function
             *
             * NOTE: During regular factory construction validation already is done (if validate==true)            *
             *
             * @param classId The factory key of the to be created object (must inherit the BaseClass template)
             * @param configuration A hash that is checked against the expectedParameters requirements
             * @param validated The resultant validated hash (has defaults injected)
             */
            static void validateConfiguration(const std::string& classId, const Hash& configuration, Hash& validated) {
                Schema schema = getSchema(classId, Schema::AssemblyRules(INIT | WRITE | READ));
                Validator validator; // Default validation
                std::pair<bool, std::string> ret = validator.validate(schema, configuration, validated);
                if (ret.first == false) throw KARABO_PARAMETER_EXCEPTION("Validation failed. \n" + ret.second);
            }

           private:
            Configurator() {}

            virtual ~Configurator() {}

            static Configurator& init() {
                static Configurator f;
                return f;
            }

            static std::string ctorKey() {
                return std::string(typeid(Hash).name());
            }

            template <typename A1>
            static std::string ctorKey() {
                std::string h(typeid(Hash).name());
                std::string a1(typeid(A1).name());
                return h += a1;
            }

            template <typename A1, typename A2>
            static std::string ctorKey() {
                std::string h(typeid(Hash).name());
                std::string a1(typeid(A1).name());
                std::string a2(typeid(A2).name());
                return h += a1 += a2;
            }

            template <typename A1, typename A2, typename A3>
            static std::string ctorKey() {
                std::string h(typeid(Hash).name());
                std::string a1(typeid(A1).name());
                std::string a2(typeid(A2).name());
                std::string a3(typeid(A3).name());
                return h += a1 += a2 += a3;
            }

            static CtorMap::const_iterator findCtor(const std::string& factoryKey, const std::string& constructorKey) {
                Registry::const_iterator it = Configurator::init().m_registry.find(factoryKey);
                if (it == Configurator::init().m_registry.end())
                    throw KARABO_PARAMETER_EXCEPTION("No factorize-able class registered for key \"" + factoryKey +
                                                     "\"");
                CtorMap::const_iterator jt = it->second.find(constructorKey);
                if (jt == it->second.end())
                    throw KARABO_PARAMETER_EXCEPTION("No constructor expecting argument(s) \"" + constructorKey +
                                                     "\" registered for key \"" + factoryKey + "\"");
                return jt;
            }
        };

        template <class Base>
        struct ConfiguratorMember1 {
            ConfiguratorMember1(int) {
                std::string classId(Base::classInfo().getClassId());
                Configurator<Base>::template registerClass<Base>(classId);
                Configurator<Base>::template registerSchemaFunction<Base>(classId);
            }

            virtual ~ConfiguratorMember1() {}
        };

        template <class Base>
        struct RegisterConfigurator1 {
            static const ConfiguratorMember1<Base> registerMe;
        };

        template <class Base, class Sub1>
        struct ConfiguratorMember2 {
            ConfiguratorMember2(int) {
                std::string classId(Sub1::classInfo().getClassId());
                Configurator<Base>::template registerClass<Sub1>(classId);
                Configurator<Base>::template registerSchemaFunction<Base>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub1>(classId);
            }

            virtual ~ConfiguratorMember2() {}
        };

        template <class Base, class Sub1>
        struct RegisterConfigurator2 {
            static const ConfiguratorMember2<Base, Sub1> registerMe;
        };

        template <class Base, class Sub1, class Sub2>
        struct ConfiguratorMember3 {
            ConfiguratorMember3(int) {
                std::string classId(Sub2::classInfo().getClassId());
                Configurator<Base>::template registerClass<Sub2>(classId);
                Configurator<Base>::template registerSchemaFunction<Base>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub1>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub2>(classId);
            }

            virtual ~ConfiguratorMember3() {}
        };

        template <class Base, class Sub1, class Sub2>
        struct RegisterConfigurator3 {
            static const ConfiguratorMember3<Base, Sub1, Sub2> registerMe;
        };

        template <class Base, class Sub1, class Sub2, class Sub3>
        struct ConfiguratorMember4 {
            ConfiguratorMember4(int) {
                std::string classId(Sub3::classInfo().getClassId());
                Configurator<Base>::template registerClass<Sub3>(classId);
                Configurator<Base>::template registerSchemaFunction<Base>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub1>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub2>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub3>(classId);
            }

            virtual ~ConfiguratorMember4() {}
        };

        template <class Base, class Sub1, class Sub2, class Sub3>
        struct RegisterConfigurator4 {
            static const ConfiguratorMember4<Base, Sub1, Sub2, Sub3> registerMe;
        };

        template <class Base, class Sub1, class Sub2, class Sub3, class Sub4>
        struct ConfiguratorMember5 {
            ConfiguratorMember5(int) {
                std::string classId(Sub4::classInfo().getClassId());
                Configurator<Base>::template registerClass<Sub4>(classId);
                Configurator<Base>::template registerSchemaFunction<Base>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub1>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub2>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub3>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub4>(classId);
            }

            virtual ~ConfiguratorMember5() {}
        };

        template <class Base, class Sub1, class Sub2, class Sub3, class Sub4>
        struct RegisterConfigurator5 {
            static const ConfiguratorMember5<Base, Sub1, Sub2, Sub3, Sub4> registerMe;
        };

        // Allow to register statically constructor with one more argument
        template <class Base, class A1>
        struct ConfiguratorWithArgMember1 {
            ConfiguratorWithArgMember1(int) {
                std::string classId(Base::classInfo().getClassId());
                Configurator<Base>::template registerClass<Base, A1>(classId);
                Configurator<Base>::template registerSchemaFunction<Base>(classId);
            }

            virtual ~ConfiguratorWithArgMember1() {}
        };

        template <class Base, class A1>
        struct RegisterConfiguratorWithArg1 {
            static const ConfiguratorWithArgMember1<Base, A1> registerMe;
        };

        template <class Base, class A1, class Sub1>
        struct ConfiguratorWithArgMember2 {
            ConfiguratorWithArgMember2(int) {
                std::string classId(Sub1::classInfo().getClassId());
                Configurator<Base>::template registerClass<Sub1, A1>(classId);
                Configurator<Base>::template registerSchemaFunction<Base>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub1>(classId);
            }

            virtual ~ConfiguratorWithArgMember2() {}
        };

        template <class Base, class A1, class Sub1>
        struct RegisterConfiguratorWithArg2 {
            static const ConfiguratorWithArgMember2<Base, A1, Sub1> registerMe;
        };

        template <class Base, class A1, class Sub1, class Sub2>
        struct ConfiguratorWithArgMember3 {
            ConfiguratorWithArgMember3(int) {
                std::string classId(Sub2::classInfo().getClassId());
                Configurator<Base>::template registerClass<Sub2, A1>(classId);
                Configurator<Base>::template registerSchemaFunction<Base>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub1>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub2>(classId);
            }

            virtual ~ConfiguratorWithArgMember3() {}
        };

        template <class Base, class A1, class Sub1, class Sub2>
        struct RegisterConfiguratorWithArg3 {
            static const ConfiguratorWithArgMember3<Base, A1, Sub1, Sub2> registerMe;
        };

        template <class Base, class A1, class Sub1, class Sub2, class Sub3>
        struct ConfiguratorWithArgMember4 {
            ConfiguratorWithArgMember4(int) {
                std::string classId(Sub3::classInfo().getClassId());
                Configurator<Base>::template registerClass<Sub3, A1>(classId);
                Configurator<Base>::template registerSchemaFunction<Base>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub1>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub2>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub3>(classId);
            }

            virtual ~ConfiguratorWithArgMember4() {}
        };

        template <class Base, class A1, class Sub1, class Sub2, class Sub3>
        struct RegisterConfiguratorWithArg4 {
            static const ConfiguratorWithArgMember4<Base, A1, Sub1, Sub2, Sub3> registerMe;
        };

        template <class Base, class A1, class Sub1, class Sub2, class Sub3, class Sub4>
        struct ConfiguratorWithArgMember5 {
            ConfiguratorWithArgMember5(int) {
                std::string classId(Sub4::classInfo().getClassId());
                Configurator<Base>::template registerClass<Sub4, A1>(classId);
                Configurator<Base>::template registerSchemaFunction<Base>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub1>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub2>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub3>(classId);
                Configurator<Base>::template registerSchemaFunction<Sub4>(classId);
            }

            virtual ~ConfiguratorWithArgMember5() {}
        };

        template <class Base, class A1, class Sub1, class Sub2, class Sub3, class Sub4>
        struct RegisterConfiguratorWithArg5 {
            static const ConfiguratorWithArgMember5<Base, A1, Sub1, Sub2, Sub3, Sub4> registerMe;
        };


#define _KARABO_REGISTER_FOR_CONFIGURATION_1(base) \
    template <>                                    \
    const karabo::util::ConfiguratorMember1<base> karabo::util::RegisterConfigurator1<base>::registerMe(1);

#define _KARABO_REGISTER_FOR_CONFIGURATION_2(base, sub1)                                                             \
    template <>                                                                                                      \
    const karabo::util::ConfiguratorMember2<base, sub1> karabo::util::RegisterConfigurator2<base, sub1>::registerMe( \
          1);

#define _KARABO_REGISTER_FOR_CONFIGURATION_3(base, sub1, sub2) \
    template <>                                                \
    const karabo::util::ConfiguratorMember3<base, sub1, sub2>  \
          karabo::util::RegisterConfigurator3<base, sub1, sub2>::registerMe(1);

#define _KARABO_REGISTER_FOR_CONFIGURATION_4(base, sub1, sub2, sub3) \
    template <>                                                      \
    const karabo::util::ConfiguratorMember4<base, sub1, sub2, sub3>  \
          karabo::util::RegisterConfigurator4<base, sub1, sub2, sub3>::registerMe(1);

#define _KARABO_REGISTER_FOR_CONFIGURATION_5(base, sub1, sub2, sub3, sub4) \
    template <>                                                            \
    const karabo::util::ConfiguratorMember5<base, sub1, sub2, sub3, sub4>  \
          karabo::util::RegisterConfigurator5<base, sub1, sub2, sub3, sub4>::registerMe(1);

#define _KARABO_REGISTER_FOR_CONFIGURATION_N(x0, x1, x2, x3, x4, x5, FUNC, ...) FUNC

#define KARABO_REGISTER_FOR_CONFIGURATION(...)                                                                  \
    _KARABO_REGISTER_FOR_CONFIGURATION_N(                                                                       \
          , ##__VA_ARGS__, _KARABO_REGISTER_FOR_CONFIGURATION_5(__VA_ARGS__),                                   \
          _KARABO_REGISTER_FOR_CONFIGURATION_4(__VA_ARGS__), _KARABO_REGISTER_FOR_CONFIGURATION_3(__VA_ARGS__), \
          _KARABO_REGISTER_FOR_CONFIGURATION_2(__VA_ARGS__), _KARABO_REGISTER_FOR_CONFIGURATION_1(__VA_ARGS__), \
          _KARABO_REGISTER_FOR_CONFIGURATION_0(__VA_ARGS__))


#define _KARABO_REGISTER_FOR_CONFIGURATION_ADDON_1(a1, base) \
    template <>                                              \
    const karabo::util::ConfiguratorWithArgMember1<base, a1> \
          karabo::util::RegisterConfiguratorWithArg1<base, a1>::registerMe(1);

#define _KARABO_REGISTER_FOR_CONFIGURATION_ADDON_2(a1, base, sub1) \
    template <>                                                    \
    const karabo::util::ConfiguratorWithArgMember2<base, a1, sub1> \
          karabo::util::RegisterConfiguratorWithArg2<base, a1, sub1>::registerMe(1);

#define _KARABO_REGISTER_FOR_CONFIGURATION_ADDON_3(a1, base, sub1, sub2) \
    template <>                                                          \
    const karabo::util::ConfiguratorWithArgMember3<base, a1, sub1, sub2> \
          karabo::util::RegisterConfiguratorWithArg3<base, a1, sub1, sub2>::registerMe(1);

#define _KARABO_REGISTER_FOR_CONFIGURATION_ADDON_4(a1, base, sub1, sub2, sub3) \
    template <>                                                                \
    const karabo::util::ConfiguratorWithArgMember4<base, a1, sub1, sub2, sub3> \
          karabo::util::RegisterConfiguratorWithArg4<base, a1, sub1, sub2, sub3>::registerMe(1);

#define _KARABO_REGISTER_FOR_CONFIGURATION_ADDON_5(a1, base, sub1, sub2, sub3, sub4) \
    template <>                                                                      \
    const karabo::util::ConfiguratorWithArgMember5<base, a1, sub1, sub2, sub3, sub4> \
          karabo::util::RegisterConfiguratorWithArg5<base, a1, sub1, sub2, sub3, sub4>::registerMe(1);

#define _KARABO_REGISTER_FOR_CONFIGURATION_ADDON_N(x0, x1, x2, x3, x4, x5, FUNC, ...) FUNC

#define KARABO_REGISTER_FOR_CONFIGURATION_ADDON(a1, ...)                                                    \
    _KARABO_REGISTER_FOR_CONFIGURATION_ADDON_N(, ##__VA_ARGS__,                                             \
                                               _KARABO_REGISTER_FOR_CONFIGURATION_ADDON_5(a1, __VA_ARGS__), \
                                               _KARABO_REGISTER_FOR_CONFIGURATION_ADDON_4(a1, __VA_ARGS__), \
                                               _KARABO_REGISTER_FOR_CONFIGURATION_ADDON_3(a1, __VA_ARGS__), \
                                               _KARABO_REGISTER_FOR_CONFIGURATION_ADDON_2(a1, __VA_ARGS__), \
                                               _KARABO_REGISTER_FOR_CONFIGURATION_ADDON_1(a1, __VA_ARGS__), \
                                               _KARABO_REGISTER_FOR_CONFIGURATION_ADDON_0(a1, __VA_ARGS__))
/**
 * If we are importing symbols from a dll in windows, we have to tell the compiler that he should use a single
 * "version" of our templated factory only. This happens through KARABO_TEMPLATE_DLL which resolves to "extern"
 * in case of dll import. If karaboFramework is compiled into a dll, the symbols are exported, if we have an
 * application linking against the karabo dll, the symbols are imported and templates must be flagged extern!
 */
#ifdef _WIN32
#ifdef __DLL__
#define KARABO_REGISTER_CONFIGURATION_BASE_CLASS(className) \
    template class __declspec(dllexport) karabo::util::Configurator<className>;
#else
#define KARABO_REGISTER_CONFIGURATION_BASE_CLASS(className) \
    extern template class __declspec(dllimport) karabo::util::Configurator<className>;
#endif
#else
#ifdef __SO__
#define KARABO_REGISTER_CONFIGURATION_BASE_CLASS(className)
#else
#define KARABO_REGISTER_CONFIGURATION_BASE_CLASS(className) extern template class karabo::util::Configurator<className>;
#endif
#endif

#define KARABO_EXPLICIT_TEMPLATE(className) template class className;

#define KARABO_CONFIGURATION_BASE_CLASS                                                                          \
    static boost::shared_ptr<Self> create(const karabo::util::Hash& configuration, const bool validate = true) { \
        return karabo::util::Configurator<Self>::create(configuration, validate);                                \
    }                                                                                                            \
                                                                                                                 \
    static boost::shared_ptr<Self> create(const std::string& classId,                                            \
                                          const karabo::util::Hash& configuration = karabo::util::Hash(),        \
                                          const bool validate = true) {                                          \
        return karabo::util::Configurator<Self>::create(classId, configuration, validate);                       \
    }                                                                                                            \
                                                                                                                 \
    static boost::shared_ptr<Self> createNode(const std::string& nodeName, const std::string& classId,           \
                                              const karabo::util::Hash& input, const bool validate = true) {     \
        return karabo::util::Configurator<Self>::createNode(nodeName, classId, input, validate);                 \
    }                                                                                                            \
                                                                                                                 \
    static boost::shared_ptr<Self> createChoice(const std::string& choiceName, const karabo::util::Hash& input,  \
                                                const bool validate = true) {                                    \
        return karabo::util::Configurator<Self>::createChoice(choiceName, input, validate);                      \
    }                                                                                                            \
                                                                                                                 \
    static std::vector<boost::shared_ptr<Self>> createList(                                                      \
          const std::string& listName, const karabo::util::Hash& input, const bool validate = true) {            \
        return karabo::util::Configurator<Self>::createList(listName, input, validate);                          \
    }                                                                                                            \
                                                                                                                 \
    static karabo::util::Schema getSchema(                                                                       \
          const std::string& classId,                                                                            \
          const karabo::util::Schema::AssemblyRules& rules = karabo::util::Schema::AssemblyRules()) {            \
        return karabo::util::Configurator<Self>::getSchema(classId, rules);                                      \
    }                                                                                                            \
                                                                                                                 \
    static std::vector<std::string> getRegisteredClasses() {                                                     \
        return karabo::util::Configurator<Self>::getRegisteredClasses();                                         \
    }

#define KARABO_CONFIGURATION_BASE_CLASS_ADDON(A1)                                                              \
    static boost::shared_ptr<Self> create(const karabo::util::Hash& configuration, const A1& a1,               \
                                          const bool validate = true) {                                        \
        return karabo::util::Configurator<Self>::create<A1>(configuration, a1, validate);                      \
    }                                                                                                          \
                                                                                                               \
    static boost::shared_ptr<Self> create(const std::string& classId, const karabo::util::Hash& configuration, \
                                          const A1& a1, const bool validate = true) {                          \
        return karabo::util::Configurator<Self>::create<A1>(classId, configuration, a1, validate);             \
    }

    } // namespace util
} // namespace karabo

#endif
