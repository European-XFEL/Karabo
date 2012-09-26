/*
 * $Id$
 *
 * File:   ChoiceElement.hh
 * Author: <wp76@xfel.eu>
 *
 * Created on July 1, 2011, 11:48 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_CHOICEELEMENT_HH
#define	KARABO_UTIL_CHOICEELEMENT_HH

#include "GenericElement.hh"

namespace karabo {
  namespace util {

    template<class T >
    class CHOICE_ELEMENT : public GenericElement<CHOICE_ELEMENT<T>, std::string > {
    public:

      CHOICE_ELEMENT(Schema& expected) : GenericElement<CHOICE_ELEMENT<T>, std::string >(expected) {
        this->initializeElementPointer(this);
      }

      void build() {
        this->m_element.choiceType(T::expectedParameters(this->m_expected->getAccessMode()));
        this->m_element.displayType(T::classInfo().getClassId());
        //alternatively we can take 'className' instead of 'classId' (to be shown as 'displayType' element in expected parameters) :
        //this->m_element.displayType(T::classInfo().getClassName());
      }
    };

    template<>
    class CHOICE_ELEMENT<Schema> : public GenericElement<CHOICE_ELEMENT<Schema>, std::string > {
    public:
      
      CHOICE_ELEMENT(Schema& expected, const Schema& pythonExpected) : GenericElement<CHOICE_ELEMENT<Schema>, std::string >(expected), m_pythonExpected(pythonExpected) {
        this->initializeElementPointer(this);
      }

      void build() {
        this->m_element.choiceType(m_pythonExpected);
        this->m_element.displayType("Schema");
      }
    private:
      const Schema & m_pythonExpected;
    };
   
  }
}



#endif
