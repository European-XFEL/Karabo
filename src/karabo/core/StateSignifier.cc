#include <assert.h>
#include <karabo/util/StringTools.hh>
#include <karabo/util/Factory.hh>
#include "StateSignifier.hh"
#include "karabo/log/Logger.hh"
#include <algorithm>


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

        void StateSignifier::fillAncestorNames_r(const State& s, std::vector<std::string>& all) {
            all.push_back(s.name());
            if (s.parent()) fillAncestorNames_r(*(s.parent()), all);
        }
        
        size_t StateSignifier::rankedAt(const State& s) {
            vector<string> allnames;
            fillAncestorNames_r(s, allnames);   // fill array of state name and all its parent names
            for (vector<string>::const_iterator ii = allnames.begin(); ii != allnames.end(); ii++) {
                for (size_t i = 0; i < m_trumpList.size(); i++) {
                    if (*ii == m_trumpList[i].name()) return i+1;
                }
            }
            return 0;
        }
        
        const bool StateSignifier::inList(const std::vector<State> & list, const State & s) const{
            return std::find(list.begin(), list.end(), s) != list.end();
        }
        
        
        
        StateSignifier::StateSignifier(const karabo::core::State& staticMoreSignificant,
                                       const karabo::core::State& changingMoreSignificant):
                                       StateSignifier::StateSignifier(std::vector<karabo::core::State>(),
                                       staticMoreSignificant, changingMoreSignificant){
            
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
                m_trumpList.push_back(State::INTERLOCKED);
                m_trumpList.push_back(State::ERROR);
                m_trumpList.push_back(State::UNKNOWN);
            } else {
                m_trumpList = trumpList;
                
                if(inList(m_trumpList, State::CHANGING)){
                    if(!inList(m_trumpList, State::INCREASING) && !inList(m_trumpList, State::DECREASING)){
                        if(staticMoreSignificant == State::DECREASING){
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::CHANGING), State::INCREASING);
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::CHANGING), State::DECREASING);
                        } else if(staticMoreSignificant == State::INCREASING){
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::CHANGING), State::DECREASING);
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::CHANGING), State::INCREASING);
                        }
                    } else if (!inList(m_trumpList, State::INCREASING)){
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::CHANGING), State::INCREASING);
                    } else if (!inList(m_trumpList, State::DECREASING)){
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::CHANGING), State::DECREASING);
                    }
                }

                if(inList(m_trumpList, State::STATIC)){
                    if(!inList(m_trumpList, State::ACTIVE) && !inList(m_trumpList, State::PASSIVE)){
                        if(staticMoreSignificant == State::PASSIVE){
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::STATIC), State::ACTIVE);
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::STATIC), State::PASSIVE);
                        } else if(staticMoreSignificant == State::ACTIVE){
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::STATIC), State::PASSIVE);
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::STATIC), State::ACTIVE);
                        }
                    } else if (!inList(m_trumpList, State::ACTIVE)){
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::STATIC), State::ACTIVE);
                    } else if (!inList(m_trumpList, State::PASSIVE)){
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::STATIC), State::PASSIVE);
                    }
                }

                if(inList(m_trumpList, State::KNOWN)){
                    if (!inList(m_trumpList, State::DISABLED)){
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::DISABLED);
                    }

                    if(!inList(m_trumpList, State::ACTIVE) && !inList(m_trumpList, State::PASSIVE)){
                        if(staticMoreSignificant == State::PASSIVE){
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::ACTIVE);
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::PASSIVE);
                        } else if(staticMoreSignificant == State::ACTIVE){
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::PASSIVE);
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::ACTIVE);
                        }
                    } else if (!inList(m_trumpList, State::ACTIVE)){
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::ACTIVE);
                    } else if (!inList(m_trumpList, State::PASSIVE)){
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::PASSIVE);
                    }

                    if (!inList(m_trumpList, State::STATIC)){
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::STATIC);
                    }

                    if(!inList(m_trumpList, State::INCREASING) && !inList(m_trumpList, State::DECREASING)){
                        if(staticMoreSignificant == State::DECREASING){
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::INCREASING);
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::DECREASING);
                        } else if(staticMoreSignificant == State::INCREASING){
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::DECREASING);
                            m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::INCREASING);
                        }
                    } else if (!inList(m_trumpList, State::INCREASING)){
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::INCREASING);
                    } else if (!inList(m_trumpList, State::DECREASING)){
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::DECREASING);
                    }

                    if (!inList(m_trumpList, State::CHANGING)){
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::CHANGING);
                    }

                    if (!inList(m_trumpList, State::INTERLOCKED)){
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::INTERLOCKED);
                    }

                    if (!inList(m_trumpList, State::ERROR)){
                        m_trumpList.insert(std::find(m_trumpList.begin(), m_trumpList.end(), State::KNOWN), State::ERROR);
                    }
                }
                
                
            }
        }
    }
}
