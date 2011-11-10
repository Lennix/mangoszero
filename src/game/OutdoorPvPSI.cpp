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

#include "OutdoorPvPSI.h"
#include "WorldPacket.h"
#include "Player.h"
#include "GameObject.h"
#include "OutdoorPvPMgr.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "World.h"

OutdoorPvPSI::OutdoorPvPSI()
{
    m_TypeId = OUTDOOR_PVP_SI;
    m_Gathered_A = 0;
    m_Gathered_H = 0;
    m_LastController = 0;

    LoadWorldState();
}

void OutdoorPvPSI::FillInitialWorldStates(WorldPacket &data)
{
    data << SI_GATHERED_A << m_Gathered_A;
    data << SI_GATHERED_H << m_Gathered_H;
    data << SI_SILITHYST_MAX << SI_MAX_RESOURCES;
}

void OutdoorPvPSI::SendRemoveWorldStates(Player* plr)
{
    plr->SendUpdateWorldState(SI_GATHERED_A, 0);
    plr->SendUpdateWorldState(SI_GATHERED_H, 0);
    plr->SendUpdateWorldState(SI_SILITHYST_MAX, 0);
}

void OutdoorPvPSI::UpdateWorldState()
{
    SendUpdateWorldState(SI_GATHERED_A, m_Gathered_A);
    SendUpdateWorldState(SI_GATHERED_H, m_Gathered_H);
    SendUpdateWorldState(SI_SILITHYST_MAX, SI_MAX_RESOURCES);
}

void OutdoorPvPSI::LoadWorldState()
{
    QueryResult* result = CharacterDatabase.PQuery("SELECT data0, data1, data2 FROM outdoor_pvp WHERE id = '%u' LIMIT 1", m_TypeId);

    if (result)
    {
        Field* fields = result->Fetch();
        m_Gathered_A = fields[0].GetUInt32();
        m_Gathered_H = fields[1].GetUInt32();
        m_LastController = fields[2].GetUInt32();

        delete result;

        sLog.outString();
        sLog.outString("OutdoorPvP : SI >> World state has been loaded");
    }
    else
    {
        CharacterDatabase.PQuery("INSERT INTO outdoor_pvp (id, data0, data1, data2) VALUES ('%u', '%u', '%u', '%u')", m_TypeId, m_Gathered_A, m_Gathered_H, m_LastController);

        sLog.outString();
        sLog.outString("OutdoorPvP : SI >> Default world state has been loaded");
    }
}

void OutdoorPvPSI::SaveWorldState()
{
    CharacterDatabase.PQuery("UPDATE outdoor_pvp SET data0 = '%u', data1 = '%u', data2 = '%u' WHERE id = '%u'", m_Gathered_A, m_Gathered_H, m_LastController, m_TypeId);
}

bool OutdoorPvPSI::SetupOutdoorPvP()
{
    for (int i = 0; i < OutdoorPvPSIBuffZonesNum; ++i)
        RegisterZone(OutdoorPvPSIBuffZones[i]);
    return true;
}

bool OutdoorPvPSI::Update(uint32 diff)
{
    return false;
}

void OutdoorPvPSI::HandlePlayerEnterZone(Player* plr, uint32 zone)
{
    if (plr->HasAura(SI_CENARION_FAVOR, EFFECT_INDEX_0))
        plr->RemoveAurasDueToSpell(SI_CENARION_FAVOR);

    if (plr->GetTeam() == m_LastController)
        plr->CastSpell(plr, SI_CENARION_FAVOR, true);

    OutdoorPvP::HandlePlayerEnterZone(plr, zone);
}

void OutdoorPvPSI::HandlePlayerLeaveZone(Player* plr, uint32 zone)
{
    // remove buffs
    plr->RemoveAurasDueToSpell(SI_CENARION_FAVOR);
    OutdoorPvP::HandlePlayerLeaveZone(plr, zone);
}

bool OutdoorPvPSI::HandleAreaTrigger(Player* plr, uint32 trigger)
{
    switch(trigger)
    {
        case SI_AREATRIGGER_A:
            if (plr->GetTeam() == ALLIANCE && plr->HasAura(SI_SILITHYST_FLAG, EFFECT_INDEX_0))
            {
                plr->RemoveAurasDueToSpell(SI_SILITHYST_FLAG);
                plr->CastSpell(plr, SI_TRACES_OF_SILITHYST, true);

                ++m_Gathered_A;
                if (m_Gathered_A >= SI_MAX_RESOURCES)
                {
                    TeamApplyBuff(TEAM_ALLIANCE, SI_CENARION_FAVOR);
                    sWorld.SendZoneText(OutdoorPvPSIBuffZones[0], sObjectMgr.GetMangosStringForDBCLocale(LANG_OPVP_SI_CAPTURE_A));
                    m_LastController = ALLIANCE;
                    m_Gathered_A = 0;
                    m_Gathered_H = 0;
                }
                UpdateWorldState();
                SaveWorldState();
            }
            return true;
        case SI_AREATRIGGER_H:
            if (plr->GetTeam() == HORDE && plr->HasAura(SI_SILITHYST_FLAG, EFFECT_INDEX_0))
            {
                plr->RemoveAurasDueToSpell(SI_SILITHYST_FLAG);
                plr->CastSpell(plr, SI_TRACES_OF_SILITHYST, true);

                ++m_Gathered_H;
                if (m_Gathered_H >= SI_MAX_RESOURCES)
                {
                    TeamApplyBuff(TEAM_HORDE, SI_CENARION_FAVOR);
                    sWorld.SendZoneText(OutdoorPvPSIBuffZones[0], sObjectMgr.GetMangosStringForDBCLocale(LANG_OPVP_SI_CAPTURE_H));
                    m_LastController = HORDE;
                    m_Gathered_A = 0;
                    m_Gathered_H = 0;
                }
                UpdateWorldState();
                SaveWorldState();
            }
            return true;
    }
    return false;
}

bool OutdoorPvPSI::HandleDropFlag(Player* plr, uint32 spellId)
{
    if (spellId == SI_SILITHYST_FLAG)
    {
        // if it was dropped away from the player's turn-in point, then create a silithyst mound, if it was dropped near the areatrigger, then it was dispelled by the outdoorpvp, so do nothing
        AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(plr->GetTeam() == HORDE ? SI_AREATRIGGER_H : SI_AREATRIGGER_A);

        // if distance is > 5.0f -> summon mound
        if (atEntry && plr->GetDistance(atEntry->x,atEntry->y,atEntry->z) > 5.0f + atEntry->radius)
            plr->CastSpell(plr, SI_SUMMON_SILITHYST_MOND, true);

        plr->UpdateSpeed(MOVE_RUN, true);
        return true;
    }
    return false;
}
