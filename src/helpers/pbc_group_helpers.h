#ifndef MOD_PBC_GROUP_HELPERS_H
#define MOD_PBC_GROUP_HELPERS_H

#include <string>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include "SharedDefines.h"

class Player;

// ---------------------------------------------------------------------------
// Group / bot-finding helpers  (main-thread only)
//
// All functions require live Player* objects and must be called on the main
// thread.  They walk group membership lists and return vectors of Player*
// for event dispatch and roll logic.
// ---------------------------------------------------------------------------

// Find all bot players in the same group as 'player' (excluding 'player'
// itself).  Returns empty vector if the player is not in a group.
std::vector<Player*> PBC_FindGroupBots(Player* player);

// Find all real (non-bot) players in the same group as 'player' (excluding
// 'player' itself).  Returns empty vector if the player is not in a group.
std::vector<Player*> PBC_FindRealPlayersInGroup(Player* player);

// Find all real (non-bot) players in the same sub-group (party) as 'player'
// within a raid group, excluding 'player' itself.  In a regular 5-man party
// this is equivalent to PBC_FindRealPlayersInGroup.  Used so that party-chat
// messages sent inside a raid only deliver history to the sender's own
// sub-group.
std::vector<Player*> PBC_FindRealPlayersInSubGroup(Player* player);

// Find all bot players in the same group as 'player', excluding 'player'
// itself AND any GUIDs in the excluded set.  Returns empty vector if the
// player is not in a group.
std::vector<Player*> PBC_FindGroupBotsExcluding(Player* player,
    const std::unordered_set<uint64_t>& excludedGuids);

// Find all bot players in the same sub-group (party) as 'player' within a
// raid group, excluding 'player' itself.  In a regular 5-man party this is
// equivalent to PBC_FindGroupBots.  Used so that party-chat messages sent
// inside a raid only reach — and are answered by — the members of the
// sender's own sub-group.
std::vector<Player*> PBC_FindSubGroupBots(Player* player);

// Find nearby bot players around a WorldObject source within the given range.
// Used for say/yell chat events where proximity matters rather than group.
std::vector<Player*> PBC_FindNearbyBots(Player* source, float range = 60.0f);

// Find bot players eligible to answer a channel (e.g. General) chat message.
// Channel membership itself isn't queryable from outside the core Channel
// class, so this uses the same-zone-as-sender proxy (the scope classic
// WotLK's zone-based General channel already uses), excludes 'sender', and
// caps the result at maxCandidates (after shuffling) to bound worst-case
// fan-out on populated channels.
std::vector<Player*> PBC_FindChannelBots(Player* sender, size_t maxCandidates);

// Returns true if bot is in a group that contains at least one real (non-bot) player.
bool PBC_BotIsGroupedWithRealPlayer(Player* bot);

// Returns true if 'player' is a member of a raid group (i.e. a group with
// more than one sub-group, which is what makes party chat unable to reach
// everyone).  Returns false for a regular 5-man party or when ungrouped.
bool PBC_IsInRaidGroup(Player* player);

// Returns the group chat type that should be used for events targeting
// 'player's group:
//   - CHAT_MSG_RAID  when the player is in a raid group,
//   - CHAT_MSG_PARTY when in a regular party,
//   - CHAT_MSG_SAY   when not grouped.
// This is the single source of truth for "which group channel do event
// replies go to" and should be used by every event dispatcher so that raid
// members in different sub-groups can all see the responses.
ChatMsg PBC_GetGroupChatType(Player* player);

#endif // MOD_PBC_GROUP_HELPERS_H
