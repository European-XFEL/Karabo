#include <assert.h>
#include <karabo/util/StringTools.hh>
#include <karabo/util/Factory.hh>
#include "States.hh"


using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace core {
            
        State StateSignifier::returnMostSignificant(const std::vector<State>& listOfStates) {
            if (listOfStates.empty())
                throw KARABO_PARAMETER_EXCEPTION("Empty list of states in StateSignifier::returnMostSignificant");
            
            const State* state = 0;
            size_t stateRank = 0;
            for (vector<State>::const_iterator ii = listOfStates.begin(); ii != listOfStates.end(); ++ii) {
                size_t rank = rankedAt(*ii);
                if (rank > stateRank) {
                    state = &(*ii);
                    stateRank = rank;
                }
            }
            if (state) return *state;
            throw KARABO_PARAMETER_EXCEPTION("Wrong configuration: no states from input list are found in the trumplist!");
        }

        void StateSignifier::fillNames_p(const State& s, std::vector<std::string>& all) {
            all.push_back(s.name());
            if (s.parent()) fillNames_p(*(s.parent()), all);
        }
        
        size_t StateSignifier::rankedAt(const State& s) {
            vector<string> allnames;
            fillNames_p(s, allnames);
            for (vector<string>::const_iterator ii = allnames.begin(); ii != allnames.end(); ii++) {
                for (size_t i = 0; i < m_trumpList.size(); i++) {
                    if (*ii == m_trumpList[i].name()) return i+1;
                }
            }
            std::cout << "Failed to find " << toString(allnames) << " in trump list" << std::endl;
            return 0;
        }

        StateSignifier::StateSignifier(const std::vector<karabo::core::State>& trumpList,
                                       const karabo::core::State& staticMoreSignificant,
                                       const karabo::core::State& changingMoreSignificant)
        : m_trumpList() {

            if (trumpList.empty()) {
                m_trumpList.push_back(State::DISABLED);
                m_trumpList.push_back(State::INIT);

                // Take care to compare the objects, not the pointers:
                if (staticMoreSignificant == State::PASSIVE) {
                    m_trumpList.push_back(State::ACTIVE);
                    m_trumpList.push_back(State::PASSIVE);
                } else if (staticMoreSignificant == State::ACTIVE) {
                    m_trumpList.push_back(State::PASSIVE);
                    m_trumpList.push_back(State::ACTIVE);
                }

                m_trumpList.push_back(State::STATIC);

                if (changingMoreSignificant == State::DECREASING) {
                    m_trumpList.push_back(State::INCREASING);
                    m_trumpList.push_back(State::DECREASING);
                } else if (changingMoreSignificant == State::INCREASING) {
                    m_trumpList.push_back(State::DECREASING);
                    m_trumpList.push_back(State::INCREASING);
                }

                m_trumpList.push_back(State::CHANGING);
                m_trumpList.push_back(State::ERROR);
                m_trumpList.push_back(State::UNKNOWN);
            } else {
                m_trumpList = trumpList;
            }
        }
    }
}
