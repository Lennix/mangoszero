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

#include "OutdoorPvPMgr.h"
#include "OutdoorPvPSI.h"
#include "OutdoorPvPEP.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Policies/SingletonImp.h"

INSTANTIATE_SINGLETON_1(OutdoorPvPMgr);

OutdoorPvPMgr::OutdoorPvPMgr()
{
    m_UpdateTimer = 0;
    //sLog.outDebug("Instantiating OutdoorPvPMgr");
}

OutdoorPvPMgr::~OutdoorPvPMgr()
{
    //sLog.outDebug("Deleting OutdoorPvPMgr");
    for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
        delete *itr;
}

void OutdoorPvPMgr::InitOutdoorPvP()
{
    // create new opvp
    OutdoorPvP * pOP = new OutdoorPvPSI;
    // respawn, init variables
    if (!pOP->SetupOutdoorPvP())
    {
        sLog.outDebug("OutdoorPvP : SI init failed.");
        delete pOP;
    }
    else
    {
        m_OutdoorPvPSet.push_back(pOP);
        sLog.outString();
        sLog.outString("OutdoorPvP : SI successfully initiated.");
    }

    pOP = new OutdoorPvPEP;
    // respawn, init variables
    if (!pOP->SetupOutdoorPvP())
    {
        sLog.outDebug("OutdoorPvP : EP init failed.");
        delete pOP;
    }
    else
    {
        m_OutdoorPvPSet.push_back(pOP);
        sLog.outString();
        sLog.outString("OutdoorPvP : EP successfully initiated.");
        sLog.outString();
    }
}

void OutdoorPvPMgr::AddZone(uint32 zoneid, OutdoorPvP* handle)
{
    m_OutdoorPvPMap[zoneid] = handle;
}

void OutdoorPvPMgr::HandlePlayerEnterZone(Player* plr, uint32 zoneid)
{
    OutdoorPvPMap::iterator itr = m_OutdoorPvPMap.find(zoneid);
    if (itr == m_OutdoorPvPMap.end())
        return;

    if (itr->second->HasPlayer(plr))
        return;

    itr->second->HandlePlayerEnterZone(plr, zoneid);
    sLog.outDebug("Player %u entered outdoorpvp id %u", plr->GetGUIDLow(), itr->second->GetTypeId());
}

void OutdoorPvPMgr::HandlePlayerLeaveZone(Player* plr, uint32 zoneid)
{
    OutdoorPvPMap::iterator itr = m_OutdoorPvPMap.find(zoneid);
    if (itr == m_OutdoorPvPMap.end())
        return;

    // teleport: remove once in removefromworld, once in updatezone
    if (!itr->second->HasPlayer(plr))
        return;

    itr->second->HandlePlayerLeaveZone(plr, zoneid);
    sLog.outDebug("Player %u left outdoorpvp id %u",plr->GetGUIDLow(), itr->second->GetTypeId());
}

OutdoorPvP* OutdoorPvPMgr::GetOutdoorPvPToZoneId(uint32 zoneid)
{
    OutdoorPvPMap::iterator itr = m_OutdoorPvPMap.find(zoneid);
    if (itr == m_OutdoorPvPMap.end())
    {
        // no handle for this zone, return
        return NULL;
    }
    return itr->second;
}

void OutdoorPvPMgr::Update(uint32 diff)
{
    m_UpdateTimer += diff;
    if (m_UpdateTimer > OUTDOORPVP_OBJECTIVE_UPDATE_INTERVAL)
    {
        for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
            (*itr)->Update(m_UpdateTimer);
        m_UpdateTimer = 0;
    }
}

bool OutdoorPvPMgr::HandleCustomSpell(Player* plr, uint32 spellId, GameObject* go)
{
    for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
    {
        if ((*itr)->HandleCustomSpell(plr, spellId, go))
            return true;
    }
    return false;
}

ZoneScript* OutdoorPvPMgr::GetZoneScript(uint32 zoneId)
{
    OutdoorPvPMap::iterator itr = m_OutdoorPvPMap.find(zoneId);
    if (itr != m_OutdoorPvPMap.end())
        return itr->second;
    else
        return NULL;
}

bool OutdoorPvPMgr::HandleOpenGo(Player* plr, ObjectGuid guid)
{
    for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
    {
        if ((*itr)->HandleOpenGo(plr,guid))
            return true;
    }
    return false;
}

void OutdoorPvPMgr::HandleGossipOption(Player* plr, uint64 guid, uint32 gossipid)
{
    for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
    {
        if ((*itr)->HandleGossipOption(plr, guid, gossipid))
            return;
    }
}

bool OutdoorPvPMgr::CanTalkTo(Player* plr, Creature* c, GossipMenuItems gso)
{
    for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
    {
        if ((*itr)->CanTalkTo(plr, c, gso))
            return true;
    }
    return false;
}

void OutdoorPvPMgr::HandleDropFlag(Player* plr, uint32 spellId)
{
    for(OutdoorPvPSet::iterator itr = m_OutdoorPvPSet.begin(); itr != m_OutdoorPvPSet.end(); ++itr)
    {
        if ((*itr)->HandleDropFlag(plr, spellId))
            return;
    }
}
