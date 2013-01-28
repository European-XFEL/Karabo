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
            typedef std::map<std::string, boost::any > CtorMap;
            typedef std::map<std::string, CtorMap> Registry;

            Registry _registry;

        public:

            template <class ConcreteClass>
            static void registerClass(const std::string& factoryKey) {
                std::cout << "Registering class: " << factoryKey << " with constructor argument: " << ctorKey() << std::endl;
                Factory::init()._registry[factoryKey][ctorKey()] = static_cast<boost::function < boost::shared_ptr<AbstractClass > () > > (boost::factory<boost::shared_ptr<ConcreteClass> >());
            }

            template <class ConcreteClass, typename A1>
            static void registerClass(const std::string& factoryKey) {
                std::cout << "Registering class: " << factoryKey << " with constructor argument: " << ctorKey<A1 > () << std::endl;
                Factory::init()._registry[factoryKey][ctorKey<A1 > ()] = static_cast<boost::function < boost::shared_ptr<AbstractClass > (const A1&) > > (boost::factory<boost::shared_ptr<ConcreteClass> >());
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
                for (Registry::const_iterator it = Factory::init()._registry.begin(); it != Factory::init()._registry.end(); ++it) {
                    registeredClasses.push_back(it->first);
                }
                return registeredClasses;
            }

        private:

            static Factory& init() {
                static Factory f;
                return f;
            }

            Factory() {
            };

            Factory(const Factory&) {
            };

            virtual ~Factory() {
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

            static CtorMap::const_iterator findCtor(const std::string& factoryKey, const std::string& constructorKey) {
                Registry::const_iterator it = Factory::init()._registry.find(factoryKey);
                if (it == Factory::init()._registry.end()) throw KARABO_PARAMETER_EXCEPTION("No factorize-able class registered for key \"" + factoryKey + "\"");
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
        };

        template <class AbstractClass, class ConcreteClass, typename A1>
        struct FactoryMember1 {

            FactoryMember1(const std::string & factoryKey) {
                Factory<AbstractClass>::template registerClass<ConcreteClass, A1 > (factoryKey);
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
                karabo::util::Register0<abstractClass, concreteClass>::registerAs(concreteClass::classInfo().getClassId());
        
        #define KARABO_REGISTER_IN_FACTORY_AS(abstractClass, concreteClass, factoryKey) \
                template<> const karabo::util::FactoryMember0<abstractClass, concreteClass> \
                karabo::util::Register0<abstractClass, concreteClass>::registerAs(factoryKey);

        #define KARABO_REGISTER_IN_FACTORY_1(abstractClass, concreteClass, argType1) \
                template<> const karabo::util::FactoryMember1<abstractClass, concreteClass, argType1> \
                karabo::util::Register1<abstractClass, concreteClass, argType1>::registerAs(concreteClass::classInfo().getClassId());

    }
}
#endif

