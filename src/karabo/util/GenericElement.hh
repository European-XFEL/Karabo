/*
 * $Id$
 *
 * File:   GenericElement.hh
 * Author: <wp76@xfel.eu>
 *
 * Created on July 1, 2011, 11:12 AM
 * Major re-design on January 30, 2013, 17:22 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_GENERIC_ELEMENT_HH
#define	KARABO_UTIL_GENERIC_ELEMENT_HH

#include "Schema.hh"

namespace karabo {
    namespace util {

        /**
         * The GenericElement class is a base class for various element types: simple, vector, choice, list
         * and single.
         *
         */
        template <class Derived>
        class GenericElement {

        protected:

            Schema* m_schema;
            boost::shared_ptr<Hash::Node> m_node;

        public:

            GenericElement(Schema& expected) : m_schema(&expected) {
            }

            virtual ~GenericElement() {
            }

            /**
             * The <b>key</b> method serves for setting up a unique name for the element.
             * @param name Unique name for the key
             * @return reference to the Element (to allow method's chaining)
             * 
             * <b>Example:</b>
             * @code
             * SOME_ELEMENT(expected)
             *         .key("type")
             *         ...
             *         .commit();
             * @endcode
             */
            virtual Derived& key(const std::string& name) {

                if (m_node) throw KARABO_LOGIC_EXCEPTION("The key() function must only be called once!");

                m_node = boost::shared_ptr<Hash::Node>(new Hash::Node(name, 0));

                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>alias</b> method serves for setting up just another name for the element.
             * Note:  this <i>another</i> name may not be necessarily a string. Just any type!
             * @param alias <i>Another</i> name for this element
             * @return reference to the Element (to allow method's chaining)
             */
            template <class AliasType>
            Derived& alias(const AliasType& alias) {
                m_node->setAttribute<AliasType > ("alias", alias);
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>tag</b> method allows to tag some expected parameters for later grouping/sorting
             * @param tag of any type
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& tags(const std::string& tags, const std::string& sep = " ,;") {
                m_node->setAttribute("tags", karabo::util::fromString<std::string, std::vector>(tags, sep));
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>displayedName</b> method serves for setting up an user friendly name for the element
             * to be used by GUI
             * @param name User friendly name for the element
             * @return reference to the Element (to allow method's chaining)
             * 
             * <b>Example:</b>
             * @code
             * SOME_ELEMENT(expected)
             *         ...
             *         .displayedName("Connection Type")
             *         ...
             *         .commit();
             * @endcode
             */
            Derived& displayedName(const std::string& name) {
                m_node->setAttribute("displayedName", name);
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>description</b> method serves for setting up a description of the element
             * @param desc Short description of the element
             * @return reference to the Element (to allow method's chaining)
             * 
             * <b>Example:</b>
             * @code
             * SOME_ELEMENT(expected)
             *         ...
             *         .description("Decide whether the connection is used to implement a TCP Server or TCP Client")
             *         ...
             *         .commit();
             * @endcode
             */
            Derived& description(const std::string& description) {
                m_node->setAttribute("description", description);
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>advanced</b> method serves for setting up the <i>expert level</i> attribute to be advanced.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& advanced() {
                m_node->setAttribute<int>("expertLevel", Schema::ADVANCED);
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>commit</b> method injects the element to the expected parameters list. If not called
             * the element is not usable. This must be called after the element is fully defined.
             */
            virtual void commit() {
                beforeAddition();
                if (m_schema) {
                    m_schema->addElement(*m_node);
                } else {
                    throw KARABO_INIT_EXCEPTION("Could not append element to non-initialized Schema object");
                }
            }

            Hash::Node& getNode() {
                return *m_node;

            }

        protected:

            virtual void beforeAddition() {
            }
        };


    }
}



#endif	

