/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2010 MaNGOSZero <http://github.com/mangoszero/mangoszero/>
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

#ifndef OUTDOOR_PVP_SI_
#define OUTDOOR_PVP_SI_

#include "OutdoorPvP.h"

enum SI_Data
{
    SI_MAX_RESOURCES                = 200,

    SI_AREATRIGGER_A                = 4162,
    SI_AREATRIGGER_H                = 4168,

    SI_SILITHYST_FLAG               = 29519,
    SI_SUMMON_SILITHYST_MOND        = 29533,
    SI_TRACES_OF_SILITHYST          = 29534,
    SI_CENARION_FAVOR               = 30754
};

enum SI_WorldStates
{
    SI_GATHERED_A       = 2313,
    SI_GATHERED_H       = 2314,
    SI_SILITHYST_MAX    = 2317
};

const uint32 OutdoorPvPSIBuffZonesNum = 3;

const uint32 OutdoorPvPSIBuffZones[OutdoorPvPSIBuffZonesNum] = {1377, 3428, 3429};

class OutdoorPvPSI : public OutdoorPvP
{
public:
    OutdoorPvPSI();
    bool SetupOutdoorPvP();
    void HandlePlayerEnterZone(Player* plr, uint32 zone);
    void HandlePlayerLeaveZone(Player* plr, uint32 zone);
    bool Update(uint32 diff);
    void FillInitialWorldStates(WorldPacket &data);
    void SendRemoveWorldStates(Player* plr);
    bool HandleAreaTrigger(Player* plr, uint32 trigger);
    bool HandleDropFlag(Player* plr, uint32 spellId);
    void UpdateWorldState();

    void LoadWorldState();
    void SaveWorldState();
private:
    uint32 m_Gathered_A;
    uint32 m_Gathered_H;
    uint32 m_LastController;
};

#endif
