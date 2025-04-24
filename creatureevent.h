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

#ifndef __CREATUREEVENT__
#define __CREATUREEVENT__
#include "enums.h"

#include "baseevents.h"
#include "tile.h"

enum CreatureEventType_t : uint64_t
{
	CREATURE_EVENT_NONE = 0,
	CREATURE_EVENT_LOGIN = 1,
	CREATURE_EVENT_LOGOUT = 2,
	CREATURE_EVENT_SPAWN_SINGLE = 3,
	CREATURE_EVENT_SPAWN_GLOBAL = 4,
	CREATURE_EVENT_CHANNEL_JOIN = 5,
	CREATURE_EVENT_CHANNEL_LEAVE = 6,
	CREATURE_EVENT_CHANNEL_REQUEST = 7,
	CREATURE_EVENT_ADVANCE = 8,
	CREATURE_EVENT_LOOK = 9,
	CREATURE_EVENT_DIRECTION = 10,
	CREATURE_EVENT_OUTFIT = 11,
	CREATURE_EVENT_MAIL_SEND = 12,
	CREATURE_EVENT_MAIL_RECEIVE = 13,
	CREATURE_EVENT_TRADE_REQUEST = 14,
	CREATURE_EVENT_TRADE_ACCEPT = 15,
	CREATURE_EVENT_TEXTEDIT = 16,
	CREATURE_EVENT_HOUSEEDIT = 17,
	CREATURE_EVENT_REPORTBUG = 18,
	CREATURE_EVENT_REPORTVIOLATION = 19,
	CREATURE_EVENT_THINK = 20,
	CREATURE_EVENT_STATSCHANGE = 21,
	CREATURE_EVENT_COMBAT_AREA = 22,
	CREATURE_EVENT_THROW = 23,
	CREATURE_EVENT_PUSH = 24,
	CREATURE_EVENT_TARGET = 25,
	CREATURE_EVENT_FOLLOW = 26,
	CREATURE_EVENT_COMBAT = 27,
	CREATURE_EVENT_ATTACK = 28,
	CREATURE_EVENT_CAST = 29,
	CREATURE_EVENT_KILL = 30,
	CREATURE_EVENT_DEATH = 31,
	CREATURE_EVENT_PREPAREDEATH = 32,
	CREATURE_EVENT_EXTENDED_OPCODE = 33, // otclient additional network opcodes
	CREATURE_EVENT_INSERT_MARKET_MYOFFERS = 34, // market insert my offers in seller - thalles vitor
	CREATURE_EVENT_INSERT_MARKET_ALLOFFERS = 35, // market insert all offers in buy - thalles vitor
	CREATURE_EVENT_INSERT_MARKETBUYOFFER_UPDATE = 36, // update market window (all offers, my offers) after buy item from market - thalles vitor
	CREATURE_EVENT_INSERT_MARKET_MYHISTORIC = 37, // market insert my historic buyed of items - thalles vitor
	CREATURE_EVENT_INSERT_MARKET_MAKEOFFREMOVEITEM = 38, // market make offer remove the item in pushed to slot - thalles vitor
	CREATURE_EVENT_INSERT_MARKET_UPDATEOFFERS = 39, // market update makered offers - thalles vitor
	CREATURE_EVENT_INSERT_MARKET_OFFERSTOME = 40, // market insert all offers to me - thalles vitor
	CREATURE_EVENT_INSERT_MARKETITEM = 41,
	CREATURE_EVENT_MOVEITEM = 42,
	CREATURE_EVENT_STATSCHANGEEX = 43,
	CREATURE_EVENT_EQUIP_ITEM = 44,
	CREATURE_EVENT_UNEQUIP_ITEM = 45 
};

enum StatsChange_t
{
	STATSCHANGE_HEALTHGAIN,
	STATSCHANGE_HEALTHLOSS,
	STATSCHANGE_MANAGAIN,
	STATSCHANGE_MANALOSS
};

class Monster;
class CreatureEvent;

class CreatureEvents : public BaseEvents
{
	public:
		CreatureEvents();
		virtual ~CreatureEvents();

		// global events
		bool playerLogin(Player* player);
		bool playerLogout(Player* player, bool forceLogout);
		bool monsterSpawn(Monster* monster);

		CreatureEvent* getEventByName(const std::string& name);
		CreatureEventType_t getType(const std::string& type);

	protected:
		virtual std::string getScriptBaseName() const {return "creaturescripts";}
		virtual void clear();

		virtual Event* getEvent(const std::string& nodeName);
		virtual bool registerEvent(Event* event, xmlNodePtr p, bool override);

		virtual LuaInterface& getInterface() {return m_interface;}
		LuaInterface m_interface;

		//creature events
		typedef std::vector<CreatureEvent*> CreatureEventList;
		CreatureEventList m_creatureEvents;
};

struct DeathEntry;
typedef std::vector<DeathEntry> DeathList;

typedef std::map<uint32_t, Player*> UsersMap;
class CreatureEvent : public Event
{
	public:
		CreatureEvent(LuaInterface* _interface);
		CreatureEvent(const CreatureEvent* copy);
		virtual ~CreatureEvent() {}

		virtual bool configureEvent(xmlNodePtr p);

		bool isLoaded() const {return m_loaded;}
		const std::string& getName() const {return m_eventName;}
		CreatureEventType_t getEventType() const {return m_type;}

		void copyEvent(CreatureEvent* creatureEvent);
		void clearEvent();

		//scripting
		uint32_t executePlayer(Player* player);
		uint32_t executeLogout(Player* player, bool forceLogout);
		uint32_t executeSpawn(Monster* monster);
		uint32_t executeChannel(Player* player, uint16_t channelId, UsersMap usersMap);
		uint32_t executeChannelRequest(Player* player, const std::string& channel, bool isPrivate, bool custom);
		uint32_t executeAdvance(Player* player, skills_t skill, uint32_t oldLevel, uint32_t newLevel);
		uint32_t executeLook(Player* player, Thing* thing, const Position& position, int16_t stackpos, int32_t lookDistance);
		uint32_t executeMail(Player* player, Player* target, Item* item, bool openBox);
		uint32_t executeTradeRequest(Player* player, Player* target, Item* item);
		uint32_t executeTradeAccept(Player* player, Player* target, Item* item, Item* targetItem);
		uint32_t executeTextEdit(Player* player, Item* item, const std::string& newText);
		uint32_t executeHouseEdit(Player* player, uint32_t houseId, uint32_t listId, const std::string& text);
		uint32_t executeReportBug(Player* player, const std::string& comment);
		uint32_t executeReportViolation(Player* player, ReportType_t type, uint8_t reason, const std::string& name,
			const std::string& comment, const std::string& translation, uint32_t statementId);
		uint32_t executeThink(Creature* creature, uint32_t interval);
		uint32_t executeDirection(Creature* creature, Direction old, Direction current);
		uint32_t executeOutfit(Creature* creature, const Outfit_t& old, const Outfit_t& current);
		uint32_t executeStatsChange(Creature* creature, Creature* attacker, StatsChange_t type, CombatType_t combat, int32_t value);
		uint32_t executeCombatArea(Creature* creature, Tile* tile, bool isAggressive);
		uint32_t executePush(Player* player, Creature* target, Tile* tile);
		uint32_t executeThrow(Player* player, Item* item, const Position& fromPosition, const Position& toPosition, bool fromPlayer, bool toPlayer);
		uint32_t executeCombat(Creature* creature, Creature* target, bool aggressive);
		uint32_t executeAction(Creature* creature, Creature* target);
		uint32_t executeCast(Creature* creature, Creature* target = NULL);
		uint32_t executeKill(Creature* creature, Creature* target, const DeathEntry& entry);
		uint32_t executeDeath(Creature* creature, Item* corpse, DeathList deathList);
		uint32_t executePrepareDeath(Creature* creature, DeathList deathList);
		uint32_t executeExtendedOpcode(Creature* creature, uint8_t opcode, const std::string& buffer);
		uint32_t executeMoveItem(Player* player, Item* item, uint8_t count, const Position& fromPos, const Position& toPos, Item* toContainer, Item* fromContainer, int16_t fstack);
		
		// slot
		void executeStatsChangeEx(Creature* creature, Creature* attacker, StatsChange_t statType, CombatType_t combat, int32_t& value, Origin_t origin, bool inherited);
		void executeEquipItemEvent(Player* player, Item* item, uint8_t slot);
		
		// Thalles Vitor - Market System
		uint32_t executeMarketInsert(Player* player, Item* item);
		uint32_t executeInsertMarketMyOffers(Player* player, uint16_t item_id, const std::string& item_name,
		uint64_t item_time, uint16_t amount, uint64_t price, uint16_t gender, uint16_t level, const std::string& ispokemon, 
		const std::string& attributes, const std::string& description, uint16_t row_count, uint16_t row_count_id, const std::string& type, uint64_t transaction_id, bool onlyoffers);
		uint32_t executeInsertMarketAllOffers(Player* player, uint16_t item_id, const std::string& item_name,
		const std::string& item_seller, uint16_t amount, uint64_t price, uint16_t gender, uint16_t level, const std::string& ispokemon, 
		const std::string& attributes, const std::string& description, uint16_t row_count, uint16_t row_count_id, uint64_t item_time, uint64_t transaction_id, bool onlyoffers, uint64_t page_numeration);
		uint32_t executeBuyOfferUpdateMarketWindow(Player* player);
		uint32_t executeInsertMarketHistoric(Player* player, const std::string& item_name, uint32_t amount, uint64_t date);
		uint32_t executeMarketOfferRemoveItem(Player* player, uint16_t item_id, const std::string& description, uint16_t amount);
		uint32_t executeMakeOffers(Player* player, uint16_t item_id, const std::string& item_name, const std::string& item_seller, uint16_t count_row, const std::string& description, uint64_t transaction_id);
		uint32_t executeMarketViewOffersToMe(Player* player, uint16_t item_id, const std::string& item_name, const std::string& description, uint64_t tempo, uint16_t count_row, uint64_t transaction_id);
		//

	protected:
		virtual std::string getScriptEventName() const;
		virtual std::string getScriptEventParams() const;

		bool m_loaded;
		std::string m_eventName;
		CreatureEventType_t m_type;
};
#endif
