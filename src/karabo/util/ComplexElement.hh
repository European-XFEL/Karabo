/*
 * $Id$
 *
 * File:   ComplexElement.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 1, 2011, 11:19 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_COMPLEXELEMENT_HH
#define	KARABO_UTIL_COMPLEXELEMENT_HH

#include "GenericElement.hh"

namespace karabo {
  namespace util {

    class ComplexElement {
      Schema& m_expected;
      Schema m_element;
      std::string m_key;

    public:

      ComplexElement(Schema& expected) : m_expected(expected) {
      }

      ComplexElement& key(const std::string& key) {
        m_key = key;
        m_element.key(key);
        return *this;
      }
      
      template <class T>
      ComplexElement& alias(const T& alias) {
          m_element.alias(alias);
          return *this;
      }
      
      ComplexElement& displayedName(const std::string& displayedName) {
        m_element.displayedName(displayedName);
        return *this;
      }
      
      ComplexElement& reconfigureAndRead() {
        m_element.access(WRITE|READ);
        return *this;
      }
      
      ComplexElement& initAndRead() {
        m_element.access(INIT|READ);
        return *this;
      }

      ComplexElement& reconfigurable() {
        m_element.access(WRITE);
        return *this;
      }

      ComplexElement& readOnly() {
        m_element.access(READ);
        // Set the assignment and defaults here, as the API would look strange to assign something to a read-only
        m_element.assignment(Schema::OPTIONAL_PARAM);
        m_element.defaultValue("0");
        return *this;
      }

      ComplexElement& init() {
        m_element.access(INIT);
        return *this;
      }

      ComplexElement& description(const std::string& desc) {
        m_element.description(desc);
        return *this;
      }

      ComplexElement& assignmentMandatory() {
        m_element.assignment(Schema::MANDATORY_PARAM);
        return *this;
      }

      ComplexElement& assignmentOptional() {
        m_element.assignment(Schema::OPTIONAL_PARAM);
        return *this;
      }

      ComplexElement& assignmentInternal() {
        m_element.assignment(Schema::INTERNAL_PARAM);
        return *this;
      }
      
      ComplexElement& allowedStates(const std::string& states) {
         m_element.allowedStates(states);
         return *this;
      }
      
      ComplexElement& advanced() {
         m_element.expertLevel(Schema::ADVANCED);
         return *this;
      }
      
      ComplexElement& displayType(const std::string& type) {
          m_element.displayType(type);
          return *this;
      }

      // Returns a reference to the next level Schema if not leaf
      // Else returns empty Schema
      Schema& commit() {
        static Schema dummy;
        try {
          m_element.initParameterDescription(m_key, m_expected.getAccessMode()); // TODO Check whether to set the assignment type
          Schema& furtherExpected = m_expected.addElement(m_element);
          if (furtherExpected.has("elements")) {
            Schema& nextLevel = furtherExpected.get<Schema > ("elements");
            nextLevel.setAccessMode(m_expected.getAccessMode());
            return furtherExpected.get<Schema > ("elements");
          }
        } catch (...) {
          KARABO_RETHROW
        }
        // Have to break recursion here, this is the standard trick when references are involved
        return dummy;
      }
    };

    typedef ComplexElement COMPLEX_ELEMENT;
    
  }
}
#endif

