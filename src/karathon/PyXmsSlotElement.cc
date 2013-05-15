/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "boost/python.hpp"

#include <karabo/xms/SlotElement.hh>

namespace bp = boost::python;
using namespace karabo::xms;
using namespace karabo::util;
using namespace std;


struct SlotElementBase_Wrapper : SlotElementBase< SLOT_ELEMENT >, bp::wrapper<SlotElementBase< SLOT_ELEMENT > > {


    SlotElementBase_Wrapper(Schema & expected) : SlotElementBase< SLOT_ELEMENT > (boost::ref(expected))
    , bp::wrapper< SlotElementBase< SLOT_ELEMENT > >() {
    }


    virtual void commit() {
        bp::override func_commit = this->get_override("commit");
        func_commit();
    }


    virtual void beforeAddition() {
        bp::override func_beforeAddition = this->get_override("beforeAddition");
        func_beforeAddition();
    }

};


struct SLOT_ELEMENT_Wrapper : SLOT_ELEMENT, bp::wrapper<SLOT_ELEMENT > {


    SLOT_ELEMENT_Wrapper(SLOT_ELEMENT const & arg) : SLOT_ELEMENT(arg)
    , bp::wrapper< SLOT_ELEMENT > () {
    }


    SLOT_ELEMENT_Wrapper(Schema & expected) : SLOT_ELEMENT(boost::ref(expected))
    , bp::wrapper< SLOT_ELEMENT > () {
    }


    virtual void commit() {
        if (bp::override func_commit = this->get_override("commit"))
            func_commit();
        else
            this->SLOT_ELEMENT::commit();
    }


    void default_commit() {
        SLOT_ELEMENT::commit();
    }

};


void exportPyXmsSlotElement() {

    bp::class_< SlotElementBase_Wrapper, boost::noncopyable > sl("SlotElementBase", bp::init< Schema & > (bp::arg("expected")));
    
    sl.def("allowedStates"
           , (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)(string const &, string const &))(&SlotElementBase<SLOT_ELEMENT >::allowedStates)
           , (bp::arg("states"), bp::arg("sep")=" ,;")
           , bp::return_internal_reference<> ());
    
    sl.def("allowedRoles"
           , (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)(string const &, string const &))(&SlotElementBase<SLOT_ELEMENT >::allowedRoles)
           , (bp::arg("states"), bp::arg("sep")=" ,;")
           , bp::return_internal_reference<> ());
    
    sl.def("commit"
           , bp::pure_virtual((void (SlotElementBase<SLOT_ELEMENT>::*)())(&SlotElementBase< SLOT_ELEMENT >::commit))
           , bp::return_internal_reference<> ());

    sl.def("connectionAssignmentIsMandatory"
           , (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)())(&SlotElementBase< SLOT_ELEMENT>::connectionAssignmentIsMandatory)
           , bp::return_internal_reference<> ());

    sl.def("connectionAssignmentIsOptional"
           , (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)())(&SlotElementBase< SLOT_ELEMENT>::connectionAssignmentIsOptional)
           , bp::return_internal_reference<> ());

    sl.def("connectionsAreNotReconfigurable"
           , (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)())(&SlotElementBase< SLOT_ELEMENT>::connectionsAreNotReconfigurable)
           , bp::return_internal_reference<> ());

    sl.def("connectionsAreReconfigurable"
           , (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)())(&SlotElementBase< SLOT_ELEMENT>::connectionsAreReconfigurable)
           , bp::return_internal_reference<> ());

    sl.def("description"
           , (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)(string const &))(&SlotElementBase<SLOT_ELEMENT >::description)
           , (bp::arg("desc"))
           , bp::return_internal_reference<> ());

    sl.def("displayedName"
           , (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)(string const &))(&SlotElementBase<SLOT_ELEMENT >::displayedName)
           , (bp::arg("displayedName"))
           , bp::return_internal_reference<> ());

    sl.def("key"
           , (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)(string const &))(&SlotElementBase<SLOT_ELEMENT >::key)
           , (bp::arg("name"))
           , bp::return_internal_reference<> ());

    { //karabo::xms::SLOT_ELEMENT
        bp::class_< SLOT_ELEMENT_Wrapper, bp::bases<SlotElementBase< SLOT_ELEMENT > > > elem("SLOT_ELEMENT", bp::init< karabo::util::Schema &>(bp::arg("expected")));

        bp::implicitly_convertible< Schema &, SLOT_ELEMENT > ();

        elem.def("commit"
                 , (void (SLOT_ELEMENT::*)())(&SLOT_ELEMENT::commit)
                 , (void (SLOT_ELEMENT_Wrapper::*)())(&SLOT_ELEMENT_Wrapper::default_commit));
    }
}
