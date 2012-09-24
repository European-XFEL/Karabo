/*
 * $Id$
 *
 * File:   ListElement.hh
 * Author: <wp76@xfel.eu>
 *
 * Created on July 1, 2011, 11:48 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_UTIL_LISTELEMENT_HH
#define	EXFEL_UTIL_LISTELEMENT_HH

#include "GenericElement.hh"

namespace exfel {
  namespace util {

    template<class T>
    class LIST_ELEMENT : public GenericElement<LIST_ELEMENT<T>, std::string > {
    public:

      LIST_ELEMENT(Schema& expected) : GenericElement<LIST_ELEMENT<T>, std::string >(expected) {
        this->initializeElementPointer(this);
      }

      void build() {
        this->m_element.listType(T::expectedParameters(this->m_expected->getAccessMode()));
        this->m_element.displayType(T::classInfo().getClassName());
        //alternatively we can take 'classId' instead of 'className' (to be shown as displayType element in expected parameters) :
        //this->m_element.displayType(T::classInfo().getClassId());
      }
    };

    template<class T>
    class NON_EMPTY_LIST_ELEMENT : public GenericElement<NON_EMPTY_LIST_ELEMENT<T>, std::string > {
    public:

      NON_EMPTY_LIST_ELEMENT(Schema& expected) : GenericElement<NON_EMPTY_LIST_ELEMENT<T>, std::string >(expected) {
        this->initializeElementPointer(this);
      }

      void build() {
        this->m_element.nonEmptyListType(T::expectedParameters(this->m_expected->getAccessMode()));
        this->m_element.displayType(T::classInfo().getClassName());
        //alternatively we can take 'classId' instead of 'className' (to be shown as displayType element in expected parameters) :
        //this->m_element.displayType(T::classInfo().getClassId()); 
      }

    };
  }
}



#endif	/* EXFEL_PACKAGENAME_LISTELEMENT_HH */

