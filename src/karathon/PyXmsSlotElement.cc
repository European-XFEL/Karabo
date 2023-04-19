/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include <karabo/util/State.hh>
#include <karabo/util/Validator.hh>
#include <karabo/xms/SlotElement.hh>

#include "PythonMacros.hh"
#include "boost/python.hpp"
#include "boost/python/raw_function.hpp"


namespace bp = boost::python;
using namespace karabo::xms;
using namespace karabo::util;
using namespace std;


struct SlotElementBase_Wrapper : SlotElementBase<SLOT_ELEMENT>, bp::wrapper<SlotElementBase<SLOT_ELEMENT> > {
    SlotElementBase_Wrapper(Schema &expected)
        : SlotElementBase<SLOT_ELEMENT>(boost::ref(expected)), bp::wrapper<SlotElementBase<SLOT_ELEMENT> >() {}


    virtual void commit() {
        bp::override func_commit = this->get_override("commit");
        func_commit();
    }


    virtual void beforeAddition() {
        bp::override func_beforeAddition = this->get_override("beforeAddition");
        func_beforeAddition();
    }
};


struct SLOT_ELEMENT_Wrapper : SLOT_ELEMENT, bp::wrapper<SLOT_ELEMENT> {
    SLOT_ELEMENT_Wrapper(SLOT_ELEMENT const &arg) : SLOT_ELEMENT(arg), bp::wrapper<SLOT_ELEMENT>() {}


    SLOT_ELEMENT_Wrapper(Schema &expected) : SLOT_ELEMENT(boost::ref(expected)), bp::wrapper<SLOT_ELEMENT>() {}


    virtual void commit() {
        if (bp::override func_commit = this->get_override("commit")) func_commit();
        else this->SLOT_ELEMENT::commit();
    }


    void default_commit() {
        SLOT_ELEMENT::commit();
    }
};


class SlotElementWrap {
   public:
    static bp::object allowedStatesPy(bp::tuple args, bp::dict kwargs) {
        std::vector<karabo::util::State> states;
        SLOT_ELEMENT &self = bp::extract<SLOT_ELEMENT &>(args[0]);
        for (unsigned int i = 1; i < bp::len(args); ++i) {
            const std::string state = bp::extract<std::string>(args[i].attr("name"));
            states.push_back(karabo::util::State::fromString(state));
        }
        self.allowedStates(states);
        return args[0];
    }
};


void exportPyXmsSlotElement() {
    bp::class_<SlotElementBase_Wrapper, boost::noncopyable> sl("SlotElementBase",
                                                               bp::init<Schema &>(bp::arg("expected")));

    sl.def("allowedStates", bp::raw_function(&SlotElementWrap::allowedStatesPy, 2));

    sl.def("commit",
           bp::pure_virtual((void(SlotElementBase<SLOT_ELEMENT>::*)())(&SlotElementBase<SLOT_ELEMENT>::commit)),
           bp::return_internal_reference<>());

    sl.def("description",
           (SLOT_ELEMENT &
            (SlotElementBase<SLOT_ELEMENT>::*)(string const &))(&SlotElementBase<SLOT_ELEMENT>::description),
           (bp::arg("desc")), bp::return_internal_reference<>());

    sl.def("displayedName",
           (SLOT_ELEMENT &
            (SlotElementBase<SLOT_ELEMENT>::*)(string const &))(&SlotElementBase<SLOT_ELEMENT>::displayedName),
           (bp::arg("displayedName")), bp::return_internal_reference<>());

    sl.def("key",
           (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)(string const &))(&SlotElementBase<SLOT_ELEMENT>::key),
           (bp::arg("name")), bp::return_internal_reference<>());

    sl.def("alias", &AliasAttributeWrap<SLOT_ELEMENT>::aliasPy, bp::return_internal_reference<>());

    sl.def("tags",
           (SLOT_ELEMENT &
            (SlotElementBase<SLOT_ELEMENT>::*)(string const &, string const &))(&SlotElementBase<SLOT_ELEMENT>::tags),
           (bp::arg("tags"), bp::arg("sep") = " ,;"), bp::return_internal_reference<>());

    sl.def("tags",
           (SLOT_ELEMENT &
            (SlotElementBase<SLOT_ELEMENT>::*)(vector<string> const &))(&SlotElementBase<SLOT_ELEMENT>::tags),
           (bp::arg("tags")), bp::return_internal_reference<>());

    sl.def("observerAccess",
           (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)())(&SlotElementBase<SLOT_ELEMENT>::observerAccess),
           bp::return_internal_reference<>());

    sl.def("userAccess",
           (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)())(&SlotElementBase<SLOT_ELEMENT>::userAccess),
           bp::return_internal_reference<>());

    sl.def("operatorAccess",
           (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)())(&SlotElementBase<SLOT_ELEMENT>::operatorAccess),
           bp::return_internal_reference<>());

    sl.def("expertAccess",
           (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)())(&SlotElementBase<SLOT_ELEMENT>::expertAccess),
           bp::return_internal_reference<>());

    sl.def("adminAccess",
           (SLOT_ELEMENT & (SlotElementBase<SLOT_ELEMENT>::*)())(&SlotElementBase<SLOT_ELEMENT>::adminAccess),
           bp::return_internal_reference<>());

    { // karabo::xms::SLOT_ELEMENT
        bp::class_<SLOT_ELEMENT_Wrapper, bp::bases<SlotElementBase<SLOT_ELEMENT> > > elem(
              "SLOT_ELEMENT", bp::init<karabo::util::Schema &>(bp::arg("expected")));

        bp::implicitly_convertible<Schema &, SLOT_ELEMENT>();

        elem.def("commit", (void(SLOT_ELEMENT::*)())(&SLOT_ELEMENT::commit),
                 (void(SLOT_ELEMENT_Wrapper::*)())(&SLOT_ELEMENT_Wrapper::default_commit));
    }
}
