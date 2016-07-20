/*
 * $Id: GenericFactory.hh 5498 2012-03-09 23:01:53Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on August 15, 2010, 5:41 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_GENERICFACTORY_HH
#define KARABO_UTIL_GENERICFACTORY_HH


#include <string>
#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <boost/shared_ptr.hpp>

#include "Exception.hh"
#include "StringTools.hh"

namespace karabo {
    namespace util {

        /**
         * Template for generic factories. Each templated generic factory is a Singleton. 
         * After the type (class) is registered in the type specific registry, GenericFactory can 
         * create objects conforming to this class interface.
         * This template is internal and should not be used outside of the util package.
         * In fact, this template is used by Factory template to obtain factory for specific type.
         * In other words GenericFactory creates factories. For example a declaration may look like:
         * @code
         * karabo::util::GenericFactory<karabo::util::Factory<BaseType> >
         * @endcode
         * which means ManufacturedType is equal karabo::util::Factory<BaseType>
         * @tparam ManufacturedType Type of objects to be constructed by this factory
         * @tparam ClassIdKey  Type of the keys for registry (default std::string)
         */
        template <class ManufacturedType, typename ClassIDKey = std::string>
        class GenericFactory {


            typedef boost::shared_ptr<ManufacturedType> (*BaseCreateFunction)();
            typedef std::map<ClassIDKey, BaseCreateFunction> BaseCreateFunctionRegistry;

            template <class AbstractFactory, class ConcreteFactory, class ClassProvidingInfo > friend class RegisterInFactory;

        public:

            /**
             *  Obtain the only instance of the class (singleton). Note that the constructor is private.
             * @return GenericFactory instance
             */
            static GenericFactory& getInstance() {
                static GenericFactory genericFactory;
                return genericFactory;
            }

            /** 
             * @param classIdKey 
             * @return Shared pointer to an object of ManufacturedType (interface class)
             */
            boost::shared_ptr<ManufacturedType> create(const ClassIDKey& classIdKey) const {
                boost::shared_ptr<ManufacturedType> object;
                typename BaseCreateFunctionRegistry::const_iterator baseCreateFunctionIterator = m_registry.find(classIdKey);
                if (baseCreateFunctionIterator != m_registry.end()) {
                    object = baseCreateFunctionIterator->second(); // () - means call base create function
                } else {
                    throw KARABO_LOGIC_EXCEPTION("Could not find any factorized object associated "
                                                 "to key: \"" + karabo::util::toString(classIdKey) + "\"");
                }
                return object;
            }

            /**
             * Print all keys in genericFactory to output stream
             * TODO: Should be moved outside of the class
             */

            friend std::ostream& operator<<(std::ostream& os, const GenericFactory & genericFactory) {
                return os << genericFactory.getKeysAsString();
            }

            /**
             * Serialize the content of m_registry into a std::string.
             * Default separator between keys is CR
             */
            std::string getKeysAsString(const char sep = '\n') const {
                std::ostringstream oss;
                for (typename BaseCreateFunctionRegistry::const_iterator it = m_registry.begin(); it != m_registry.end(); ++it) {
                    oss << it->first << sep;
                }

                return oss.str();
            }

            /**
             * Ask, whether a given key is registered to the factory
             */
            bool has(const ClassIDKey& key) const {
                return (m_registry.find(key) != m_registry.end());
            }

            /**
             * Returns all registered function pointers in a vector
             * @return std::vector<ClassIDKey> object
             */
            std::vector<ClassIDKey> getKeysAsVector() const {
                std::vector<std::string > keys(m_registry.size());
                size_t i = 0;
                for (typename BaseCreateFunctionRegistry::const_iterator it = m_registry.begin(); it != m_registry.end(); ++it) {
                    keys[i] = it->first;
                    ++i;
                }
                return keys;
            }

            /**
             * Returns all registered function pointers in a vector
             * @return std::set<ClassIDKey> object
             */
            std::set<ClassIDKey> getKeysAsSet() const {
                std::set<std::string > keys;
                for (typename BaseCreateFunctionRegistry::const_iterator it = m_registry.begin(); it != m_registry.end(); ++it) {
                    keys.insert(it->first);
                }
                return keys;
            }

        private:

            GenericFactory() {
            }

            /**
             * Used by RegisterInFactory class (see friend declaration).
             */
            void registerBaseCreateFunction(const ClassIDKey & classIdKey, BaseCreateFunction baseCreateFunction) {
                //std::cout << "REGISTRATION EVENT: " << classIdKey << std::endl;
                m_registry[classIdKey] = baseCreateFunction;
            }


            BaseCreateFunctionRegistry m_registry; // values of this map are pointers to functions




        };

        /**
         * These templated class registers factories. The applied pattern introduces one more level of abstraction.
         * The classes registered within GenericFactory are actually the factories dedicated for producing
         * objects of yet another classes. The BaseClassFactory should provide interface to create objects.
         *
         * This template should be used by concrete factories.
         * An object of the type RegisterInFactory &lt;class AbstractFactory, class ConcreteFactory, class ClassProvidingInfo &gt;
         * must be defined as a private static const member of a concrete factory. Static initialization of this object performs
         * auto registration of the concrete factory which becomes known to the GenericFactory registry.
         * \tparam AbstractFactory     interface type
         * \tparam DerivedClassFactory concrete implementation type (must be a type derived from AbstractFactory)
         * \tparam ClassProvidingInfo class which implements classInfo() method. The classInfo().getClassId()
         * must return a string which identifies the factory
         */
        template <class AbstractFactory, class ConcreteFactory, class ClassProvidingInfo >
        class RegisterInFactory {

            public:

            /**
             *  Base "create function". It creates object of ConcreteFactory and returns shared boost pointer
             *  to the AbstractFactory. ConcreteFactory must implement parameterless constructor.
             */
            static boost::shared_ptr<AbstractFactory> createInstance() {
                return boost::shared_ptr<AbstractFactory > (new ConcreteFactory());
            }

            /** Constructor which registers create function createInstance.
             *  \param dummy - any number
             */
            RegisterInFactory(const int& dummy) {
                GenericFactory<AbstractFactory>::getInstance()
                        .registerBaseCreateFunction(ClassProvidingInfo::classInfo().getClassId(), createInstance);
            }

        };
    }
}

#endif
