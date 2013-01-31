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

#ifndef KARABO_UTIL_LISTELEMENT_HH
#define	KARABO_UTIL_LISTELEMENT_HH

#include "GenericElement.hh"

namespace karabo {
  namespace util {

    template<class T>
    class LIST_ELEMENT : public GenericElement<LIST_ELEMENT<T>, std::string > {
    public:

      LIST_ELEMENT(Schema& expected) : GenericElement<LIST_ELEMENT<T>, std::string >(expected) {
        this->initializeElementPointer(this);
      }

      void beforeAddition() {
        this->m_node.listType(T::expectedParameters(this->m_schema->getAccessMode()));
        this->m_node.displayType(T::classInfo().getClassName());
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

      void beforeAddition() {
        this->m_node.nonEmptyListType(T::expectedParameters(this->m_schema->getAccessMode()));
        this->m_node.displayType(T::classInfo().getClassName());
        //alternatively we can take 'classId' instead of 'className' (to be shown as displayType element in expected parameters) :
        //this->m_element.displayType(T::classInfo().getClassId()); 
      }

    };
  }
}



#endif	/* KARABO_PACKAGENAME_LISTELEMENT_HH */

