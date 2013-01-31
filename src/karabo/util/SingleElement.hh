/*
 * $Id$
 *
 * File:   SingleElement.hh
 * Author: <wp76@xfel.eu>
 *
 * Created on July 1, 2011, 11:49 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_SINGLEELEMENT_HH
#define	KARABO_UTIL_SINGLEELEMENT_HH

#include "GenericElement.hh"

namespace karabo {
    namespace util {

        template<class BASE, class DERIVED = BASE>
        class SINGLE_ELEMENT : public GenericElement<SINGLE_ELEMENT<BASE, DERIVED>, std::string > {
            std::string m_classId;

        public:

            SINGLE_ELEMENT(Schema& expected) : GenericElement<SINGLE_ELEMENT<BASE, DERIVED>, std::string >(expected) {
                m_classId = DERIVED::classInfo().getClassId();
                this->initializeElementPointer(this);
            }

            
            void beforeAddition() {
                using namespace boost;
                if (m_classId.empty()) {
                    throw KARABO_PARAMETER_EXCEPTION("classId is missing, use the .classId() function");
                }
                this->m_node.singleElementType(BASE::expectedParameters(m_classId, this->m_schema->getAccessMode(), this->m_schema->getCurrentState(), this->m_node.get<std::string > ("key")));
                this->m_node.displayType(BASE::classInfo().getClassName());
                //alternatively we can take 'classId' instead of 'className' (to be shown as displayType element in expected parameters) :
                //this->m_element.displayType(BASE::classInfo().getClassId()); 
            }
        };
    }
}

#endif

