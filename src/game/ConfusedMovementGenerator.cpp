/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos-zero>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ConfusedMovementGenerator.h"
#include "Creature.h"
#include "MapManager.h"
#include "Opcodes.h"
#include "DestinationHolderImp.h"
#include "VMapFactory.h"

template<class T>
void
ConfusedMovementGenerator<T>::Initialize(T &unit)
{
    const float wander_distance=11;
    float x,y,z;
    x = unit.GetPositionX();
    y = unit.GetPositionY();
    z = unit.GetPositionZ();

    TerrainInfo const* map = unit.GetTerrain();

    i_nextMove = 1;

    bool is_water_ok, is_land_ok;
    _InitSpecific(unit, is_water_ok, is_land_ok);

    for (uint8 idx = 0; idx <= MAX_CONF_WAYPOINTS; ++idx)
    {
        float wanderX = x + wander_distance*(float)rand_norm() - wander_distance/2;
        float wanderY = y + wander_distance*(float)rand_norm() - wander_distance/2;
        MaNGOS::NormalizeMapCoord(wanderX);
        MaNGOS::NormalizeMapCoord(wanderY);

        float new_z = map->GetHeight(wanderX, wanderY, z, true);
        if (new_z > INVALID_HEIGHT && unit.IsWithinLOS(wanderX, wanderY, new_z))
        {
            // Don't move in water if we're not already in
            // Don't move on land if we're not already on it either
            bool is_water_now = map->IsInWater(x, y, z);
            bool is_water_next = map->IsInWater(wanderX, wanderY, new_z);
            if ((is_water_now && !is_water_next && !is_land_ok) || (!is_water_now && is_water_next && !is_water_ok))
            {
                i_waypoints[idx][0] = idx > 0 ? i_waypoints[idx-1][0] : x; // Back to previous location
                i_waypoints[idx][1] = idx > 0 ? i_waypoints[idx-1][1] : y;
                i_waypoints[idx][2] = idx > 0 ? i_waypoints[idx-1][2] : z;
                continue;
            }

            // Taken from FleeingMovementGenerator
            if (!(new_z - z) || wander_distance / fabs(new_z - z) > 1.0f)
            {
                i_waypoints[idx][0] = wanderX;
                i_waypoints[idx][1] = wanderY;
                i_waypoints[idx][2] = new_z;
                continue;
            }
        }
        else    // Back to previous location
        {
            i_waypoints[idx][0] = idx > 0 ? i_waypoints[idx-1][0] : x;
            i_waypoints[idx][1] = idx > 0 ? i_waypoints[idx-1][1] : y;
            i_waypoints[idx][2] = idx > 0 ? i_waypoints[idx-1][2] : z;
            continue;
        }
    }

    unit.StopMoving();
    unit.CastStop();
    unit.addUnitState(UNIT_STAT_CONFUSED|UNIT_STAT_CONFUSED_MOVE);
}

template<>
void
ConfusedMovementGenerator<Creature>::_InitSpecific(Creature &creature, bool &is_water_ok, bool &is_land_ok)
{
    creature.RemoveSplineFlag(SPLINEFLAG_WALKMODE);

    is_water_ok = creature.CanSwim();
    is_land_ok  = creature.CanWalk();
}

template<>
void
ConfusedMovementGenerator<Player>::_InitSpecific(Player &, bool &is_water_ok, bool &is_land_ok)
{
    is_water_ok = true;
    is_land_ok  = true;
}

template<class T>
void ConfusedMovementGenerator<T>::Interrupt(T &unit)
{
    // confused state still applied while movegen disabled
    unit.clearUnitState(UNIT_STAT_CONFUSED_MOVE);
}

template<class T>
void ConfusedMovementGenerator<T>::Reset(T &unit)
{
    i_nextMove = 1;
    i_nextMoveTime.Reset(0);
    i_destinationHolder.ResetUpdate();
    unit.StopMoving();
    unit.addUnitState(UNIT_STAT_CONFUSED|UNIT_STAT_CONFUSED_MOVE);
}

template<class T>
bool ConfusedMovementGenerator<T>::Update(T &unit, const uint32 &diff)
{
    if(!&unit)
        return true;

    // ignore in case other no reaction state
    if (unit.hasUnitState(UNIT_STAT_CAN_NOT_REACT & ~UNIT_STAT_CONFUSED))
        return true;

    if (i_nextMoveTime.Passed())
    {
        // currently moving, update location
        unit.addUnitState(UNIT_STAT_CONFUSED_MOVE);
        Traveller<T> traveller(unit);
        if (i_destinationHolder.UpdateTraveller(traveller, diff, false))
        {
            if (!IsActive(unit))                            // force stop processing (movement can move out active zone with cleanup movegens list)
                return true;                                // not expire now, but already lost

            if (i_destinationHolder.HasArrived())
            {
                // arrived, stop and wait a bit
                unit.StopMoving();

                i_nextMove = urand(1,MAX_CONF_WAYPOINTS);
                i_nextMoveTime.Reset(urand(0, 1500-1));     // TODO: check the minimum reset time, should be probably higher
            }
        }
    }
    else
    {
        // waiting for next move
        i_nextMoveTime.Update(diff);
        if( i_nextMoveTime.Passed() )
        {
            // start moving
            unit.addUnitState(UNIT_STAT_CONFUSED_MOVE);
            MANGOS_ASSERT( i_nextMove <= MAX_CONF_WAYPOINTS );
            const float x = i_waypoints[i_nextMove][0];
            const float y = i_waypoints[i_nextMove][1];
            const float z = i_waypoints[i_nextMove][2];
            Traveller<T> traveller(unit);
            i_destinationHolder.SetDestination(traveller, x, y, z);
        }
    }
    return true;
}

template<>
void ConfusedMovementGenerator<Player>::Finalize(Player &unit)
{
    unit.clearUnitState(UNIT_STAT_CONFUSED|UNIT_STAT_CONFUSED_MOVE);
}

template<>
void ConfusedMovementGenerator<Creature>::Finalize(Creature &unit)
{
    unit.clearUnitState(UNIT_STAT_CONFUSED|UNIT_STAT_CONFUSED_MOVE);
    unit.AddSplineFlag(SPLINEFLAG_WALKMODE);
}

template void ConfusedMovementGenerator<Player>::Initialize(Player &player);
template void ConfusedMovementGenerator<Creature>::Initialize(Creature &creature);
template void ConfusedMovementGenerator<Player>::Interrupt(Player &player);
template void ConfusedMovementGenerator<Creature>::Interrupt(Creature &creature);
template void ConfusedMovementGenerator<Player>::Reset(Player &player);
template void ConfusedMovementGenerator<Creature>::Reset(Creature &creature);
template bool ConfusedMovementGenerator<Player>::Update(Player &player, const uint32 &diff);
template bool ConfusedMovementGenerator<Creature>::Update(Creature &creature, const uint32 &diff);
