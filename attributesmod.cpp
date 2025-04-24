////////////////////////////////////////////////////////////////////////
// OpenTibia - an opensource roleplaying game
////////////////////////////////////////////////////////////////////////
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////
#include "otpch.h"
#include "attributesmod.h"

// Creates the lookup array for the skills_t enum. These can be changed
// but you need to make sure you do not switch positions or remove any of them
// since these rely on the fact that CONVERSION::SKILL[0] is fist fighting and
// CONVERSION::SKILL[6] is fish skill. 
//
// These are the names that you use when adding attributes in LUA, so to add an
// fist skill attribute to an armor you use the text described here. 
const char* CONVERSION::SKILL[7] =
{
    "skillfist",
    "skillclub",
    "skillsword",
    "skillaxe",
    "skilldist",
    "skillshield",
    "skillfish"
};

// Creates the lookup array for the stats_t enum. These can be changed
// but you need to make sure you do not switch positions or remove any of them
// since these rely on the fact that CONVERSION::STATS[0] is max health potion and
// CONVERSION::STATS[4] is magic level. 
//
// NOTE: 
//
// These are the names that you use when adding attributes in LUA, so to add an
// magic level attribute you use the text in the last one
const char* CONVERSION::STATS[5] =
{
    "maxhealthpoints",          // Increase max health
    "maxmanapoints",            // Increase max mana
    "soulpoints",               // Soul points bonus
    "level_not_implemented",    // Level increase not implemented, but can easily be
                                // Just set an appropriate attribute name here and follow the comments in movement.cpp and item.cpp
    "magiclevel"                // Magic level bonus
};