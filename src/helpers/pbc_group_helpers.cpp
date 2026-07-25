#include "pbc_group_helpers.h"
#include "pbc_utils.h"
#include "Player.h"
#include "Group.h"
#include "WorldSession.h"
#include "ObjectAccessor.h"
#include "GridNotifiers.h"
#include "CellImpl.h"

#include <algorithm>
#include <random>

// ---------------------------------------------------------------------------
// PBC_FindGroupBots
// ---------------------------------------------------------------------------

std::vector<Player*> PBC_FindGroupBots(Player* player)
{
    std::vector<Player*> bots;
    if (!PBC_PTR_VALID(player)) return bots;

    Group* grp = player->GetGroup();
    if (!grp) return bots;

    for (GroupReference* ref = grp->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!PBC_PTR_VALID(member) || member == player) continue;
        if (!member->IsInWorld()) continue;
        WorldSession* sess = member->GetSession();
        if (!PBC_PTR_VALID(sess)) continue;
        if (!sess->IsBot()) continue;
        bots.push_back(member);
    }
    return bots;
}

// ---------------------------------------------------------------------------
// PBC_FindRealPlayersInGroup
// ---------------------------------------------------------------------------

std::vector<Player*> PBC_FindRealPlayersInGroup(Player* player)
{
    std::vector<Player*> realPlayers;
    if (!PBC_PTR_VALID(player)) return realPlayers;

    Group* grp = player->GetGroup();
    if (!grp) return realPlayers;

    for (GroupReference* ref = grp->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!PBC_PTR_VALID(member) || member == player) continue;
        if (!member->IsInWorld()) continue;
        WorldSession* sess = member->GetSession();
        if (!PBC_PTR_VALID(sess)) continue;
        if (sess->IsBot()) continue;
        realPlayers.push_back(member);
    }
    return realPlayers;
}

// ---------------------------------------------------------------------------
// PBC_FindRealPlayersInSubGroup
// ---------------------------------------------------------------------------
std::vector<Player*> PBC_FindRealPlayersInSubGroup(Player* player)
{
    std::vector<Player*> realPlayers;
    if (!PBC_PTR_VALID(player)) return realPlayers;

    Group* grp = player->GetGroup();
    if (!grp) return realPlayers;

    uint8 mySubGroup = grp->GetMemberGroup(player->GetGUID());

    for (GroupReference* ref = grp->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!PBC_PTR_VALID(member) || member == player) continue;
        if (!member->IsInWorld()) continue;
        WorldSession* sess = member->GetSession();
        if (!PBC_PTR_VALID(sess) || sess->IsBot()) continue;
        if (grp->GetMemberGroup(member->GetGUID()) != mySubGroup) continue;
        realPlayers.push_back(member);
    }
    return realPlayers;
}

// ---------------------------------------------------------------------------
// PBC_FindSubGroupBots
//
// In a raid group, party chat only reaches the members of the sender's own
// sub-group.  This helper returns exactly those bots so that party-chat
// messages inside a raid are answered only by the characters who actually
// heard them.  For a regular (non-raid) party it behaves like
// PBC_FindGroupBots.
// ---------------------------------------------------------------------------
std::vector<Player*> PBC_FindSubGroupBots(Player* player)
{
    std::vector<Player*> bots;
    if (!PBC_PTR_VALID(player)) return bots;

    Group* grp = player->GetGroup();
    if (!grp) return bots;

    uint8 mySubGroup = grp->GetMemberGroup(player->GetGUID());

    for (GroupReference* ref = grp->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!PBC_PTR_VALID(member) || member == player) continue;
        if (!member->IsInWorld()) continue;
        WorldSession* sess = member->GetSession();
        if (!PBC_PTR_VALID(sess) || !sess->IsBot()) continue;
        if (grp->GetMemberGroup(member->GetGUID()) != mySubGroup) continue;
        bots.push_back(member);
    }
    return bots;
}

// ---------------------------------------------------------------------------
// PBC_FindGroupBotsExcluding
// ---------------------------------------------------------------------------

std::vector<Player*> PBC_FindGroupBotsExcluding(Player* player,
    const std::unordered_set<uint64_t>& excludedGuids)
{
    std::vector<Player*> bots;
    if (!PBC_PTR_VALID(player)) return bots;

    Group* grp = player->GetGroup();
    if (!grp) return bots;

    for (GroupReference* ref = grp->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!PBC_PTR_VALID(member) || member == player) continue;
        if (!member->IsInWorld()) continue;
        WorldSession* sess = member->GetSession();
        if (!PBC_PTR_VALID(sess)) continue;
        if (!sess->IsBot()) continue;
        if (excludedGuids.count(member->GetGUID().GetCounter())) continue;
        bots.push_back(member);
    }
    return bots;
}

// ---------------------------------------------------------------------------
// PBC_FindNearbyBots
// ---------------------------------------------------------------------------

std::vector<Player*> PBC_FindNearbyBots(Player* source, float range)
{
    std::vector<Player*> bots;
    if (!PBC_PTR_VALID(source) || !source->GetMap()) return bots;

    auto doWork = [&](Player* p)
    {
        if (!PBC_PTR_VALID(p) || p == source) return;
        if (!p->IsInWorld()) return;
        if (!p->GetSession() || !p->GetSession()->IsBot()) return;
        if (p->IsWithinDist(source, range))
            bots.push_back(p);
    };
    Acore::PlayerDistWorker<decltype(doWork)> worker(source, range, doWork);
    Cell::VisitObjects(source, worker, range);
    return bots;
}

// ---------------------------------------------------------------------------
// PBC_FindChannelBots
// ---------------------------------------------------------------------------

std::vector<Player*> PBC_FindChannelBots(Player* sender, size_t maxCandidates)
{
    std::vector<Player*> bots;
    if (!PBC_PTR_VALID(sender)) return bots;

    uint32 zoneId = sender->GetZoneId();

    for (auto const& [guid, player] : ObjectAccessor::GetPlayers())
    {
        if (!PBC_PTR_VALID(player) || player == sender) continue;
        if (!player->IsInWorld()) continue;
        if (player->GetZoneId() != zoneId) continue;
        WorldSession* sess = player->GetSession();
        if (!PBC_PTR_VALID(sess) || !sess->IsBot()) continue;
        bots.push_back(player);
    }

    if (bots.size() > maxCandidates)
    {
        std::shuffle(bots.begin(), bots.end(), PBC_GetRNG());
        bots.resize(maxCandidates);
    }

    return bots;
}

// ---------------------------------------------------------------------------
// PBC_BotIsGroupedWithRealPlayer
// ---------------------------------------------------------------------------

bool PBC_BotIsGroupedWithRealPlayer(Player* bot)
{
    if (!PBC_PTR_VALID(bot)) return false;
    Group* grp = bot->GetGroup();
    if (!grp) return false;
    for (GroupReference* ref = grp->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!PBC_PTR_VALID(member) || !member->IsInWorld()) continue;
        WorldSession* sess = member->GetSession();
        if (!PBC_PTR_VALID(sess)) continue;
        if (!sess->IsBot()) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// PBC_IsInRaidGroup
//
// A "raid group" is a group that has been converted to a raid (more than one
// sub-group).  In such a group party chat only reaches the members of the
// sender's own sub-group, so event replies must go to raid chat instead.
// ---------------------------------------------------------------------------
bool PBC_IsInRaidGroup(Player* player)
{
    if (!PBC_PTR_VALID(player)) return false;
    Group* grp = player->GetGroup();
    if (!grp) return false;
    return grp->isRaidGroup();
}

// ---------------------------------------------------------------------------
// PBC_GetGroupChatType
//
// Single source of truth for "which group channel do event replies go to".
// Returns CHAT_MSG_RAID for raid groups, CHAT_MSG_PARTY for regular parties,
// and CHAT_MSG_SAY when the player is not grouped.
// ---------------------------------------------------------------------------
ChatMsg PBC_GetGroupChatType(Player* player)
{
    if (!PBC_PTR_VALID(player)) return CHAT_MSG_SAY;
    Group* grp = player->GetGroup();
    if (!grp) return CHAT_MSG_SAY;
    return grp->isRaidGroup() ? CHAT_MSG_RAID : CHAT_MSG_PARTY;
}
