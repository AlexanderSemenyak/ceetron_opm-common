/*
  Copyright 2018 Statoil ASA

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <opm/output/eclipse/AggregateActionxData.hpp>
#include <opm/output/eclipse/AggregateGroupData.hpp>
#include <opm/output/eclipse/AggregateWellData.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/common/utility/String.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQDefine.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQAssign.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionAST.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionContext.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/Actions.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionX.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/Actdims.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQParams.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQFunctionTable.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/State.hpp>
#include <opm/output/eclipse/VectorItems/action.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>

// #####################################################################
// Class Opm::RestartIO::Helpers
// ---------------------------------------------------------------------


namespace {
    const std::map<std::string, int> lhsQuantityToIndex = {
                                                           {"F",   1},
                                                           {"W",   2},
                                                           {"G",   3},
                                                           {"D",  10},
                                                           {"M",  11},
                                                           {"Y",  12},
    };

    /*const std::map<std::string, int> lhsQuantityToItem_12 = {
                                                           {"F",   0},
                                                           {"W",   0},
                                                           {"G",   0},
                                                           {"D",   0},
                                                           {"M",   1},
                                                           {"Y",   0},
    };*/

    using cmp_enum = Opm::Action::Condition::Comparator;
    const std::map<cmp_enum, int> cmpToIacn_12 = {
                                                    {cmp_enum::GREATER,       0},
                                                    {cmp_enum::LESS,          1},
                                                    {cmp_enum::GREATER_EQUAL, 0},
                                                    {cmp_enum::LESS_EQUAL,    1},
                                                    {cmp_enum::EQUAL,         0},
                                                    {cmp_enum::INVALID,       0},
    };


const std::map<std::string, int> rhsQuantityToIndex = {
                                                {"F",   1},
                                                {"W",   2},
                                                {"G",   3},
};

using logic_enum = Opm::Action::Condition::Logical;

const std::map<logic_enum, int> logicalToIndex_17 = {
                                                    {logic_enum::AND,   1},
                                                    {logic_enum::OR,    0},
                                                    {logic_enum::END,   0},
};




    namespace iACT {

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& actDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;

            int nwin = std::max(actDims[0], 1);
            int nitPrWin = std::max(actDims[1], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(nitPrWin) }
            };
        }

        template <class IACTArray>
        void staticContrib(const Opm::Action::ActionX& actx, IACTArray& iAct, const Opm::Action::State& action_state)
        {
            //item [0]: is unknown, (=0)
            iAct[0] = 0;
            //item [1]: The number of lines of schedule data including ENDACTIO
            iAct[1] = actx.keyword_strings().size();
            //item [2]: is the number of times an action has been triggered plus 1
            iAct[2] = action_state.run_count(actx) + 1;
            //item [3]: is unknown, (=7)
            iAct[3] = 7;
            //item [4]: is unknown, (=0)
            iAct[4] = 0;
            //item [5]: The number of times the action is triggered
            iAct[5] = actx.max_run();
            //item [6]: is unknown, (=0)
            iAct[6] = 0;
            //item [7]: is unknown, (=0)
            iAct[7] = 0;
            //item [8]: The number of conditions in an ACTIONX keyword
            iAct[8] = actx.conditions().size();
        }

    } // iAct

        namespace sACT {

        Opm::RestartIO::Helpers::WindowedArray<float>
        allocate(const std::vector<int>& actDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<float>;

            int nwin = std::max(actDims[0], 1);
            int nitPrWin = std::max(actDims[2], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(nitPrWin) }
            };
        }

        template <class SACTArray>
        void staticContrib(const Opm::Action::ActionX& actx,
                           const Opm::UnitSystem& units,
                           SACTArray& sAct)
        {
            using M  = ::Opm::UnitSystem::measure;
            sAct[0] = 0.;
            sAct[1] = 0.;
            sAct[2] = 0.;
            //item [3]:  Minimum time interval between action triggers.
            sAct[3] = units.from_si(M::time, actx.min_wait());
            sAct[4] = 0.;
        }

    } // sAct

    namespace zACT {

        Opm::RestartIO::Helpers::WindowedArray<
            Opm::EclIO::PaddedOutputString<8>
        >
        allocate(const std::vector<int>& actDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<
                Opm::EclIO::PaddedOutputString<8>
            >;

            int nwin = std::max(actDims[0], 1);
            int nitPrWin = std::max(actDims[3], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(nitPrWin) }
            };
        }

    template <class ZACTArray>
    void staticContrib(const Opm::Action::ActionX& actx, ZACTArray& zAct)
    {
        // entry 1 is udq keyword
        zAct[0] = actx.name();
    }
    } // zAct

    namespace zLACT {

         Opm::RestartIO::Helpers::WindowedArray<
            Opm::EclIO::PaddedOutputString<8>
        >
        allocate(const std::vector<int>& actDims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<
                Opm::EclIO::PaddedOutputString<8>
            >;
            int nwin = std::max(actDims[0], 1);
            int nitPrWin = std::max(actDims[4], 1);
            return WV {
                WV::NumWindows{ static_cast<std::size_t>(nwin) },
                WV::WindowSize{ static_cast<std::size_t>(nitPrWin) }
            };
        }

    template <class ZLACTArray>
    void staticContrib(const Opm::Action::ActionX& actx, int noEPrZlact, ZLACTArray& zLact)
        {
            std::size_t ind = 0;
            int l_sstr = 8;
            int max_l_str = 128;
            // write out the schedule input lines
            const auto& schedule_data = actx.keyword_strings();
            for (auto z_data : schedule_data) {
                z_data = Opm::ltrim_copy(z_data);
                int n_sstr =  z_data.size()/l_sstr;
                if (static_cast<int>(z_data.size()) > max_l_str) {
                    std::cout << "Too long input data string (max 128 characters): " << z_data << std::endl;
                    throw std::invalid_argument("Actionx: " + actx.name());
                }
                else {
                    for (int i = 0; i < n_sstr; i++) {
                        zLact[ind + i] = z_data.substr(i*l_sstr, l_sstr);
                    }
                    //add remainder of last non-zero string
                    if ((z_data.size() % l_sstr) > 0)
                        zLact[ind + n_sstr] = z_data.substr(n_sstr*l_sstr);
                }
                ind += static_cast<std::size_t>(noEPrZlact);
            }
        }
    } // zLact

    namespace zACN {

         Opm::RestartIO::Helpers::WindowedArray<
            Opm::EclIO::PaddedOutputString<8>
        >
        allocate(std::size_t num_actions, const Opm::Actdims& actdims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<
                Opm::EclIO::PaddedOutputString<8>
            >;

            return WV {
                WV::NumWindows{ num_actions },
                WV::WindowSize{ actdims.max_conditions() * Opm::RestartIO::Helpers::VectorItems::ZACN::ConditionSize }
            };
        }

    template <class ZACNArray>
    void staticContrib(const Opm::Action::ActionX& actx, ZACNArray& zAcn)
        {
            using Ix = Opm::RestartIO::Helpers::VectorItems::ZACN::index;
            std::size_t offset = 0;
            // write out the schedule Actionx conditions
            const auto& actx_cond = actx.conditions();
            for (auto z_data : actx_cond) {
                // left hand quantity
                if (!z_data.lhs.date())
                    zAcn[offset + Ix::LHSQuantity] = z_data.lhs.quantity;

                // right hand quantity
                if ((z_data.rhs.quantity.substr(0,1) == "W") ||
                    (z_data.rhs.quantity.substr(0,1) == "G") ||
                    (z_data.rhs.quantity.substr(0,1) == "F"))
                    zAcn[offset + Ix::RHSQuantity] = z_data.rhs.quantity;
                // operator (comparator)
                zAcn[offset + Ix::Comparator] = z_data.cmp_string;
                // well-name if left hand quantity is a well quantity
                if (z_data.lhs.quantity.substr(0,1) == "W") {
                    zAcn[offset + Ix::LHSWell] = z_data.lhs.args[0];
                }
                // well-name if right hand quantity is a well quantity
                if (z_data.rhs.quantity.substr(0,1) == "W") {
                    zAcn[offset + Ix::RHSWell] = z_data.rhs.args[0];
                }

                // group-name if left hand quantity is a group quantity
                if (z_data.lhs.quantity.substr(0,1) == "G") {
                    zAcn[offset + Ix::LHSGroup] = z_data.lhs.args[0];
                }
                // group-name if right hand quantity is a group quantity
                if (z_data.rhs.quantity.substr(0,1) == "G") {
                    zAcn[offset + Ix::RHSGroup] = z_data.rhs.args[0];
                }

                //increment offsetex according to no of items pr condition
                offset += Opm::RestartIO::Helpers::VectorItems::ZACN::ConditionSize;
            }
        }
    } // zAcn

    namespace iACN {

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(std::size_t num_actions, const Opm::Actdims& actdims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;
            return WV {
                WV::NumWindows{ num_actions },
                WV::WindowSize{ actdims.max_conditions() * Opm::RestartIO::Helpers::VectorItems::IACN::ConditionSize }
            };
        }



        template <class IACNArray>
        void staticContrib(const Opm::Action::ActionX& actx, IACNArray& iAcn)
        {
            using Ix = Opm::RestartIO::Helpers::VectorItems::IACN::index;
            std::size_t offset = 0;
            const auto& actx_cond = actx.conditions();
            int first_greater = 0;
            {
                const auto& first_cond = actx_cond[0];
                if (first_cond.cmp == Opm::Action::Condition::Comparator::LESS)
                    first_greater = 1;
            }

            for (const auto& cond : actx_cond) {
                iAcn[offset + Ix::LHSQuantityType] = cond.lhs.int_type();
                iAcn[offset + Ix::RHSQuantityType] = cond.rhs.int_type();
                iAcn[offset + Ix::FirstGreater] = first_greater;
                iAcn[offset + Ix::TerminalLogic] = cond.logic_as_int();
                iAcn[offset + Ix::Paren] = cond.paren_as_int();
                iAcn[offset + Ix::Comparator] = cond.comparator_as_int();

                offset += Opm::RestartIO::Helpers::VectorItems::IACN::ConditionSize;
            }

            /*item [17] - is an item that is non-zero for actions with several conditions combined using logical operators (AND / OR)
                * First condition => [17] =  0
                * Second+ conditions
                *Case - no parentheses
                    *If all conditions before current condition has AND => [17] = 1
                    *If one condition before current condition has OR   => [17] = 0
                *Case - parenthesis before first condition
                    *If inside first parenthesis
                        *If all conditions before current condition has AND [17] => 1
                        *If one condition before current condition has OR  [17] => 0
                    *If after first parenthesis end and outside parenthesis
                        *If all conditions outside parentheses and before current condition has AND => [17] = 1
                        *If one condition outside parentheses and before current condition has OR [17] => 0
                    *If after first parenthesis end and inside a susequent parenthesis
                        * [17] = 0
                *Case - parenthesis after first condition
                    *If inside parentesis => [17] = 0
                    *If outside parenthesis:
                        *If all conditions outside parentheses and before current condition has AND => [17] = 1
                        *If one condition outside parentheses and before current condition has OR [17] => 0
            */

            offset = 0;
            bool insideParen = false;
            bool parenFirstCond = false;
            bool allPrevLogicOp_AND = false;
            for (auto cond_it = actx_cond.begin(); cond_it < actx_cond.end(); cond_it++) {
                if (cond_it == actx_cond.begin()) {
                    if (cond_it->open_paren()) {
                        parenFirstCond = true;
                        insideParen = true;
                    }
                    if (cond_it->logic == Opm::Action::Condition::Logical::AND) {
                        allPrevLogicOp_AND = true;
                    }
                } else {
                    // update inside parenthesis or not plus whether in first parenthesis or not
                    if (cond_it->open_paren()) {
                        insideParen = true;
                        parenFirstCond = false;
                    } else if (cond_it->close_paren()) {
                        insideParen = false;
                        parenFirstCond = false;
                    }

                    // Assign [17] based on logic (see above)
                    if (parenFirstCond && allPrevLogicOp_AND) {
                        iAcn[offset + Ix::BoolLink] = 1;
                    }
                    else if (!parenFirstCond && !insideParen && allPrevLogicOp_AND) {
                        iAcn[offset + Ix::BoolLink] = 1;
                    } else {
                        iAcn[offset + Ix::BoolLink] = 0;
                    }

                    // update the previous logic-sequence
                    if (parenFirstCond && cond_it->logic == Opm::Action::Condition::Logical::OR) {
                        allPrevLogicOp_AND = false;
                    } else if (!insideParen && cond_it->logic == Opm::Action::Condition::Logical::OR) {
                        allPrevLogicOp_AND = false;
                    }
                }
                //increment index according to no of items pr condition
                offset += Opm::RestartIO::Helpers::VectorItems::IACN::ConditionSize;
            }
        }
    } // iAcn

        namespace sACN {

        Opm::RestartIO::Helpers::WindowedArray<double>
        allocate(std::size_t num_actions, const Opm::Actdims& actdims)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<double>;
            return WV {
                WV::NumWindows{ num_actions },
                WV::WindowSize{ actdims.max_conditions() * Opm::RestartIO::Helpers::VectorItems::SACN::ConditionSize }
            };
        }

        Opm::Action::Result
        act_res(const Opm::Schedule& sched, const Opm::Action::State& action_state, const Opm::SummaryState&  smry, const std::size_t sim_step, const Opm::Action::ActionX& action) {
            auto sim_time = sched.simTime(sim_step);
            if (action.ready(action_state, sim_time)) {
                Opm::Action::Context context(smry, sched[sim_step].wlist_manager.get());
                return action.eval(context);
            } else
                return Opm::Action::Result(false);
        }

        template <class SACNArray>
        void staticContrib(const Opm::Action::ActionX&                          action,
                           const Opm::Action::State&                            action_state,
                           const Opm::SummaryState&                             st,
                           const Opm::Schedule&                                 sched,
                           const std::size_t                                    simStep,
                           SACNArray&                                           sAcn)
        {
            using Ix = Opm::RestartIO::Helpers::VectorItems::SACN::index;
            std::size_t offset = 0;
            double undef_high_val = 1.0E+20;
            const auto& wells = sched.getWells(simStep);
            const auto ar = sACN::act_res(sched, action_state, st, simStep, action);
            // write out the schedule Actionx conditions
            for (const auto&  condition : action.conditions()) {
                const std::string& lhsQtype = condition.lhs.quantity.substr(0,1);
                const std::string& rhsQtype = condition.rhs.quantity.substr(0,1);

                const auto& it_rhsq = rhsQuantityToIndex.find(rhsQtype);
                if (it_rhsq == rhsQuantityToIndex.end()) {
                    //come here if constant value condition
                    double t_val = 0.;
                    if (lhsQtype == "M") {
                       const auto& monthToNo = Opm::TimeService::eclipseMonthIndices();
                       const auto& it_mnth = monthToNo.find(condition.rhs.quantity);
                       if (it_mnth != monthToNo.end()) {
                           t_val = it_mnth->second;
                       }
                       else {
                            std::cout << "Unknown Month: " << condition.rhs.quantity << std::endl;
                            throw std::invalid_argument("Actionx: " + action.name() + "  Condition: " + condition.lhs.quantity );
                        }
                    }
                    else {
                        t_val = std::stod(condition.rhs.quantity);
                    }
                    sAcn[offset + Ix::RHSValue0] = t_val;
                    sAcn[offset + Ix::RHSValue1] = sAcn[offset + Ix::RHSValue0];
                    sAcn[offset + Ix::RHSValue2] = sAcn[offset + Ix::RHSValue0];
                    sAcn[offset + Ix::RHSValue3] = sAcn[offset + Ix::RHSValue0];
                }


                //Treat well, group and field right hand side conditions
                if (it_rhsq != rhsQuantityToIndex.end()) {
                    //Well variable
                    if ((rhsQtype == "W") && (st.has_well_var(condition.rhs.args[0], condition.rhs.quantity))) {
                        sAcn[offset + Ix::RHSValue1] = st.get_well_var(condition.rhs.args[0], condition.rhs.quantity);
                        sAcn[offset + Ix::RHSValue2] = st.get_well_var(condition.rhs.args[0], condition.rhs.quantity);
                        sAcn[offset + Ix::RHSValue3] = st.get_well_var(condition.rhs.args[0], condition.rhs.quantity);
                    }
                    //group variable
                    if ((rhsQtype == "G") && (st.has_group_var(condition.rhs.args[0], condition.rhs.quantity))) {;
                        sAcn[offset + Ix::RHSValue1] = st.get_group_var(condition.rhs.args[0], condition.rhs.quantity);
                        sAcn[offset + Ix::RHSValue2] = st.get_group_var(condition.rhs.args[0], condition.rhs.quantity);
                        sAcn[offset + Ix::RHSValue3] = st.get_group_var(condition.rhs.args[0], condition.rhs.quantity);
                    }
                    //field variable
                    if ((rhsQtype == "F") && (st.has(condition.rhs.quantity))) {
                        sAcn[offset + Ix::RHSValue1] = st.get(condition.rhs.quantity);
                        sAcn[offset + Ix::RHSValue2] = st.get(condition.rhs.quantity);
                        sAcn[offset + Ix::RHSValue3] = st.get(condition.rhs.quantity);
                    }
                }



                if (condition.lhs.date()) {
                    sAcn[offset + Ix::LHSValue1] = undef_high_val;
                    sAcn[offset + Ix::RHSValue1] = undef_high_val;
                    sAcn[offset + Ix::LHSValue2] = undef_high_val;
                    sAcn[offset + Ix::RHSValue2] = undef_high_val;
                    sAcn[offset + Ix::LHSValue3] = undef_high_val;
                    sAcn[offset + Ix::RHSValue3] = undef_high_val;
                } else {
                    //Treat well, group and field left hand side conditions
                    //Well variable
                    if (lhsQtype == "W" && ar) {
                        //find the well that violates action if relevant
                        auto well_iter = std::find_if(wells.begin(), wells.end(), [&ar](const Opm::Well& well) { return ar.has_well(well.name()); });
                        if (well_iter != wells.end()) {
                            const auto& wn = well_iter->name();

                            if (st.has_well_var(wn, condition.lhs.quantity)) {
                                sAcn[offset + Ix::LHSValue1] = st.get_well_var(wn, condition.lhs.quantity);
                                sAcn[offset + Ix::LHSValue2] = st.get_well_var(wn, condition.lhs.quantity);
                                sAcn[offset + Ix::LHSValue3] = st.get_well_var(wn, condition.lhs.quantity);
                            }
                        }
                    }
                    //group variable
                    if ((lhsQtype == "G") && (st.has_group_var(condition.lhs.args[0], condition.lhs.quantity))) {
                        sAcn[offset + Ix::LHSValue1] = st.get_group_var(condition.lhs.args[0], condition.lhs.quantity);
                        sAcn[offset + Ix::LHSValue2] = st.get_group_var(condition.lhs.args[0], condition.lhs.quantity);
                        sAcn[offset + Ix::LHSValue3] = st.get_group_var(condition.lhs.args[0], condition.lhs.quantity);
                    }
                    //field variable
                    if ((lhsQtype == "F") && (st.has(condition.lhs.quantity))) {
                        sAcn[offset + Ix::LHSValue1] = st.get(condition.lhs.quantity);
                        sAcn[offset + Ix::LHSValue2] = st.get(condition.lhs.quantity);
                        sAcn[offset + Ix::LHSValue3] = st.get(condition.lhs.quantity);
                    }
                }

                //increment index according to no of items pr condition
                offset += Opm::RestartIO::Helpers::VectorItems::SACN::ConditionSize;
            }
        }

    } // sAcn

}
// =====================================================================

Opm::RestartIO::Helpers::AggregateActionxData::
AggregateActionxData( const std::vector<int>&   rst_dims,
                      std::size_t               num_actions,
                      const Opm::Actdims&       actdims,
                      const Opm::Schedule&      sched,
                      const Opm::Action::State& action_state,
                      const Opm::SummaryState&  st,
                      const std::size_t         simStep)
    : iACT_ (iACT::allocate(rst_dims))
    , sACT_ (sACT::allocate(rst_dims))
    , zACT_ (zACT::allocate(rst_dims))
    , zLACT_(zLACT::allocate(rst_dims))
    , zACN_ (zACN::allocate(num_actions, actdims))
    , iACN_ (iACN::allocate(num_actions, actdims))
    , sACN_ (sACN::allocate(num_actions, actdims))
{

    const auto& acts = sched[simStep].actions();
    std::size_t act_ind = 0;
    for (const auto& action : acts) {
        {
            auto i_act = this->iACT_[act_ind];
            iACT::staticContrib(action, i_act, action_state);
        }

        {
            auto s_act = this->sACT_[act_ind];
            sACT::staticContrib(action, sched.getUnits(), s_act);
        }

        {
            auto z_act = this->zACT_[act_ind];
            zACT::staticContrib(action, z_act);
        }

        {
            auto z_lact = this->zLACT_[act_ind];
            zLACT::staticContrib(action, rst_dims[8], z_lact);
        }

        {
            auto z_acn = this->zACN_[act_ind];
            zACN::staticContrib(action, z_acn);
        }

        {
            auto i_acn = this->iACN_[act_ind];
            iACN::staticContrib(action, i_acn);
        }

        {
            auto s_acn = this->sACN_[act_ind];
            sACN::staticContrib(action, action_state, st, sched, simStep, s_acn);
        }

        act_ind +=1;
    }
}

Opm::RestartIO::Helpers::AggregateActionxData::
AggregateActionxData( const Opm::Schedule&      sched,
                      const Opm::Action::State& action_state,
                      const Opm::SummaryState&  st,
                      const std::size_t         simStep)
    : AggregateActionxData( Opm::RestartIO::Helpers::createActionRSTDims(sched, simStep),
                            sched[simStep].actions().ecl_size(),
                            sched.runspec().actdims(),
                            sched,
                            action_state,
                            st,
                            simStep )
{
}

