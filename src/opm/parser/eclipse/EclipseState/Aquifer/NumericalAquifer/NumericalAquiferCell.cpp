/*
  Copyright (C) 2020 SINTEF Digital

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
#include <opm/parser/eclipse/Parser/ParserKeywords/A.hpp>

#include <opm/parser/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

#include <opm/parser/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquiferCell.hpp>

namespace Opm {

    using AQUNUM = ParserKeywords::AQUNUM;
    NumericalAquiferCell::NumericalAquiferCell(const DeckRecord& record, const EclipseGrid& grid, const FieldPropsManager& field_props)
            : aquifer_id( record.getItem<AQUNUM::AQUIFER_ID>().get<int>(0) )
            , I ( record.getItem<AQUNUM::I>().get<int>(0) - 1 )
            , J ( record.getItem<AQUNUM::J>().get<int>(0) - 1 )
            , K ( record.getItem<AQUNUM::K>().get<int>(0) - 1 )
            , area (record.getItem<AQUNUM::CROSS_SECTION>().getSIDouble(0) )
            , length ( record.getItem<AQUNUM::LENGTH>().getSIDouble(0) )
            , permeability( record.getItem<AQUNUM::PERM>().getSIDouble(0) )
    {
        const auto& cell_depth = field_props.cellDepth();
        const auto& poro = field_props.get_double("PORO");
        const auto& pvtnum = field_props.get_int("PVTNUM");
        const auto& satnum = field_props.get_int("SATNUM");

        this->global_index = grid.getGlobalIndex(I, J, K);
        std::cout << " aquifer cell { " << I + 1 << " , " << J + 1 << " , " << K + 1 << std::endl;
        const size_t active_index = grid.activeIndex(this->global_index);

        if ( !record.getItem<AQUNUM::PORO>().defaultApplied(0) ) {
            this->porosity = record.getItem<AQUNUM::PORO>().getSIDouble(0);
        } else {
            this->porosity = poro[active_index];
        }

        if ( !record.getItem<AQUNUM::DEPTH>().defaultApplied(0) ) {
            this->depth = record.getItem<AQUNUM::DEPTH>().getSIDouble(0);
        } else {
            this->depth = cell_depth[active_index];
        }

        if ( !record.getItem<AQUNUM::INITIAL_PRESSURE>().defaultApplied(0) ) {
            this->init_pressure = record.getItem<AQUNUM::INITIAL_PRESSURE>().getSIDouble(0);
        }

        if ( !record.getItem<AQUNUM::PVT_TABLE_NUM>().defaultApplied(0) ) {
            this->pvttable = record.getItem<AQUNUM::PVT_TABLE_NUM>().get<int>(0);
        } else {
            this->pvttable = pvtnum[active_index];
        }

        if ( !record.getItem<AQUNUM::SAT_TABLE_NUM>().defaultApplied(0) ) {
            this->sattable = record.getItem<AQUNUM::SAT_TABLE_NUM>().get<int>(0);
        } else {
            this->sattable = satnum[active_index];
        }

        this->pore_volume = this->porosity * this->cellVolume();

        this->transmissibility = 2. * this->permeability * this->area / this->length;
    }

    double NumericalAquiferCell::cellVolume() const {
        return this->area * this->length;
    }
}




