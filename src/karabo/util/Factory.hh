/* 
 * File:   Factory.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on August 26, 2012, 6:22 PM
 * 
 * © Copyright 2012 Burkhard C. Heisen
 */

#ifndef FOOCONBAR_FACTORY_H
#define	FOOCONBAR_FACTORY_H

#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/functional/factory.hpp>
#include <boost/functional/value_factory.hpp>

#include "Exception.hh"
#include "FromTypeInfo.hh"
#include "ToCppString.hh"

namespace karabo {

    namespace util {

        /**
         * The factory class.
         * The factory uses something like a hidden (private) Singleton-Pattern.
         * This solves the problem of static initialization order but leaves a simple
         * looking API to the user.
         * 
         * A single instance of a factory holds the function objects to all registered
         * constructors. Constructors with zero or more parameters may be registered
         * and directly called.
         */
        template <class AbstractClass>
        class Factory {
        protected:

            typedef std::map<std::string, boost::any > CtorMap;
            typedef std::map<std::string, CtorMap> Registry;

            Registry m_registry;

        public:

            template <class ConcreteClass>
            static void registerClass(const std::string& factoryKey) {
                //std::cout << "Registering class: " << factoryKey << " with constructor: " << factoryKey << "(" << ctorKey() << ")" << std::endl;
                Factory::init().m_registry[factoryKey][ctorKey()] = static_cast<boost::function < boost::shared_ptr<AbstractClass > () > > (boost::factory<boost::shared_ptr<ConcreteClass> >());
            }

            template <class ConcreteClass, typename A1>
            static void registerClass(const std::string& factoryKey) {
                //std::cout << "Registering class: " << factoryKey << " with constructor: " << factoryKey << "(const " << ctorKey<A1 > () << "&)" << std::endl;
                Factory::init().m_registry[factoryKey][ctorKey<A1 > ()] = static_cast<boost::function < boost::shared_ptr<AbstractClass > (const A1&) > > (boost::factory<boost::shared_ptr<ConcreteClass> >());
            }

            static boost::shared_ptr<AbstractClass> create(const std::string& factoryKey) {
                CtorMap::const_iterator it = Factory::findCtor(factoryKey, ctorKey());
                return (boost::any_cast < boost::function < boost::shared_ptr<AbstractClass > () > >(it->second))();
            }

            template <typename A1>
            static boost::shared_ptr<AbstractClass> create(const std::string& factoryKey, const A1& a1) {
                CtorMap::const_iterator it = Factory::findCtor(factoryKey, ctorKey<A1 > ());
                return (boost::any_cast < boost::function < boost::shared_ptr<AbstractClass > (const A1&) > >(it->second))(a1);
            }

            static std::vector<std::string> getRegisteredClasses() {
                std::vector<std::string> registeredClasses;
                for (Registry::const_iterator it = Factory::init().m_registry.begin(); it != Factory::init().m_registry.end(); ++it) {
                    registeredClasses.push_back(it->first);
                }
                return registeredClasses;
            }

            static bool has(const std::string& factoryKey) {
                return Factory::init().m_registry.find(factoryKey) != Factory::init().m_registry.end();
            }

            protected:

            Factory() {
            };

            Factory(const Factory&) {
            };

            virtual ~Factory() {
            }
            
            static Factory& init() {
                static Factory f;
                return f;
            }

            static std::string ctorKey() {
                return "";
            }

            template <typename A1>
            static std::string ctorKey() {
                try {
                    return Types::convert<FromTypeInfo, ToCppString > (typeid (A1));
                } catch (const ParameterException&) {
                    // Type not registered use typeinfo.name() as fallback
                    return std::string(typeid (A1).name());
                }
            }

            template <typename A1, typename A2>
            static std::string ctorKey() {
                try {
                    std::string a1(Types::convert<FromTypeInfo, ToCppString > (typeid (A1)));
                    std::string a2(Types::convert<FromTypeInfo, ToCppString > (typeid (A2)));
                    return a1 + "," + a2;
                } catch (const ParameterException&) {
                    // Type not registered use typeinfo.name() as fallback
                    std::string a1(typeid (A1).name());
                    std::string a2(typeid (A2).name());
                    return a1 + "," + a2;
                }
            }

            static CtorMap::const_iterator findCtor(const std::string& factoryKey, const std::string& constructorKey) {
                Registry::const_iterator it = Factory::init().m_registry.find(factoryKey);
                if (it == Factory::init().m_registry.end()) throw KARABO_PARAMETER_EXCEPTION("No factorize-able class registered for key \"" + factoryKey + "\"");
                CtorMap::const_iterator jt = it->second.find(constructorKey);
                if (jt == it->second.end()) throw KARABO_PARAMETER_EXCEPTION("No constructor expecting argument(s) \"" + constructorKey + "\" registered for key \"" + factoryKey + "\"");
                return jt;
            }


        };

        template <class AbstractClass, class ConcreteClass>
        struct FactoryMember0 {

            FactoryMember0(const std::string & factoryKey) {
                Factory<AbstractClass>::template registerClass<ConcreteClass > (factoryKey);
            }

            FactoryMember0(int) {
                Factory<AbstractClass>::template registerClass<ConcreteClass > (ConcreteClass::classInfo().getClassId());
            }
        };

        template <class AbstractClass, class ConcreteClass, typename A1>
        struct FactoryMember1 {

            FactoryMember1(const std::string & factoryKey) {
                Factory<AbstractClass>::template registerClass<ConcreteClass, A1 > (factoryKey);
            }

            FactoryMember1(int) {
                Factory<AbstractClass>::template registerClass<ConcreteClass, A1 > (ConcreteClass::classInfo().getClassId());
            }
        };

        template <class AbstractClass, class ConcreteClass>
        struct Register0 {
            static const FactoryMember0<AbstractClass, ConcreteClass> registerAs;
        };

        template <class AbstractClass, class ConcreteClass, typename A1>
        struct Register1 {
            static const FactoryMember1<AbstractClass, ConcreteClass, A1> registerAs;
        };

        #define KARABO_REGISTER_IN_FACTORY(abstractClass, concreteClass) \
                template<> const karabo::util::FactoryMember0<abstractClass, concreteClass> \
                karabo::util::Register0<abstractClass, concreteClass>::registerAs(1);

        #define KARABO_REGISTER_IN_FACTORY_AS(abstractClass, concreteClass, factoryKey) \
                template<> const karabo::util::FactoryMember0<abstractClass, concreteClass> \
                karabo::util::Register0<abstractClass, concreteClass>::registerAs(factoryKey);

        #define KARABO_REGISTER_IN_FACTORY_1(abstractClass, concreteClass, argType1) \
                template<> const karabo::util::FactoryMember1<abstractClass, concreteClass, argType1> \
                karabo::util::Register1<abstractClass, concreteClass, argType1>::registerAs(1);

    }
}
#endif

