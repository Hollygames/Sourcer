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
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "monsters.h"
#include "tools.h"
#include "monster.h"

#include "luascript.h"
#include "container.h"
#include "weapons.h"

#include "spells.h"
#include "combat.h"

#include "configmanager.h"
#include "game.h"

extern Game g_game;
extern Spells* g_spells;
extern Monsters g_monsters;
extern ConfigManager g_config;

void MonsterType::reset()
{
	canPushItems = canPushCreatures = isSummonable = isIllusionable = isConvinceable = isLureable = isWalkable = hideName = hideHealth = eliminable  = playerblockspawn = canWalkEverywhere = isPuppet = isBoss = isLimbo = false;
	pushable = isAttackable = isHostile = true;

	outfit.lookHead = outfit.lookBody = outfit.lookLegs = outfit.lookFeet = outfit.lookType = outfit.lookTypeEx = outfit.lookAddons = 0;
	runAwayHealth = healthMin = manaCost = lightLevel = lightColor = yellSpeedTicks = yellChance = changeTargetSpeed = changeTargetChance = 0;
	experience = defense = armor = lookCorpse = corpseUnique = corpseAction = conditionImmunities = damageImmunities = 0;

	maxSummons = -1;
	targetDistance = 1;
	staticAttackChance = 95;
	health = healthMax = 100;
	baseSpeed = 200;

	race = RACE_BLOOD;
	skull = SKULL_NONE;
	partyShield = SHIELD_NONE;
	guildEmblem = GUILDEMBLEM_NONE;
	lootMessage = LOOTMSG_IGNORE;
	
	// Wonsr
	canWalkEverywhere = false; 
	saga = missao = 0; // Saga System

	for(SpellList::iterator it = spellAttackList.begin(); it != spellAttackList.end(); ++it)
	{
		if(!it->combatSpell)
			continue;

		delete it->spell;
		it->spell = NULL;
	}

	spellAttackList.clear();
	for(SpellList::iterator it = spellDefenseList.begin(); it != spellDefenseList.end(); ++it)
	{
		if(!it->combatSpell)
			continue;

		delete it->spell;
		it->spell = NULL;
	}

	spellDefenseList.clear();
	summonList.clear();
	scriptList.clear();

	voiceVector.clear();
	lootItems.clear();
	elementMap.clear();
}

uint16_t MonsterType::getLootRare(uint16_t item_id, Player* owner)
{	
if (g_config.getBool(ConfigManager::RARE_SYTEM_ENABLE) && Item::items[item_id].rareSystemR > 0){
	int r = random_range(1, 100000) * (g_config.getDouble(ConfigManager::RATE_LOOT_RARE)+owner->getExtraRareLootRate());
	uint16_t num = ((r < 95000) ? item_id :(r < 98500) ? Item::items[item_id].rareSystemR : (r < 99500) ? Item::items[item_id].rareSystemE : Item::items[item_id].rareSystemL);
	return num;
}
	return item_id;
}

ItemList MonsterType::createLoot(const LootBlock& lootBlock, Player* owner)
{
	uint16_t item = lootBlock.ids[0], random = Monsters::getLootRandom(owner), count = 0;
	if(lootBlock.ids.size() > 1)
		item = getLootRare(lootBlock.ids[random_range((size_t)0, lootBlock.ids.size() - 1)], owner);

	ItemList items;
	bool rarityDropped = false;
	if(random < lootBlock.chance)
		count = random % lootBlock.count + 1;
	

	Item* tmpItem = NULL;
	while (count > 0 && !rarityDropped)
	{
		uint16_t n = 0;
		if(Item::items[item].stackable)
			n = std::min(count, (uint16_t)100);

		if(!(tmpItem = Item::CreateItem(item, n)))
			break;

		count -= (!n ? 1 : n);
		if(lootBlock.subType != -1)
			tmpItem->setSubType(lootBlock.subType);
		
		uint8_t randomAttribute = random_range(1, 5);

        if (lootBlock.ninjutsu != -1)
            tmpItem->setAttribute("magiclevel", randomAttribute);

        if (lootBlock.taijutsu != -1)
            tmpItem->setAttribute("skillfist", randomAttribute);

        if (!lootBlock.raridade.empty())
        {
            std::map<int, std::string> raridade = {{1, "[N] "}, {2, "[R] "}, {3, "[E] "}, {4, "[L] "}, {5, "[M] "}};
            tmpItem->setAttribute("name", raridade[randomAttribute] + Item::items[item].name);

            rarityDropped = true;
        }

		if(lootBlock.actionId != -1)
			tmpItem->setActionId(lootBlock.actionId, false);

		if(lootBlock.uniqueId != -1)
			tmpItem->setUniqueId(lootBlock.uniqueId);

		if(!lootBlock.text.empty())
			tmpItem->setText(lootBlock.text);

		items.push_back(tmpItem);
	}

	return items;
}

bool MonsterType::createChildLoot(Container* parent, const LootBlock& lootBlock, uint32_t& money, std::stringstream& str, Player* player)
{
	LootItems::const_iterator it = lootBlock.childLoot.begin();
	if(it == lootBlock.childLoot.end())
		return true;

	ItemList items;
	for(; it != lootBlock.childLoot.end() && !parent->full(); ++it)
	{
		items = createLoot(*it, player);
		if(items.empty())
			continue;

		for(ItemList::iterator iit = items.begin(); iit != items.end(); ++iit)
		{
			Item* tmpItem = *iit;
			if(Container* container = tmpItem->getContainer())
			{
				if(createChildLoot(container, *it, money, str, player))
					parent->__internalAddThing(tmpItem);
				else
					delete container;
			}
			else {
				bool LootCatch = false;
				if(player && (player->statusAutoLoot() == "On")) {
					LootCatch = player->checkAutoLoot(tmpItem->getID());
				}
				if(LootCatch) {
					if(player->isMoneyAutoLoot(tmpItem, money)) {
						continue;
					}
					g_game.internalPlayerAddItem(NULL, player, tmpItem);
					str << " " << tmpItem->getNameDescription() << ",";
					continue;
				}
					parent->__internalAddThing(tmpItem);
			}
		}
	}

	return !parent->empty();
}

uint16_t Monsters::getLootRandom(Player* owner)
{
	double extraChance = (owner ? owner->getExtraLootChance() : 0);
	return (uint16_t)std::ceil((double)random_range(0, MAX_LOOTCHANCE) / (g_config.getDouble(ConfigManager::RATE_LOOT) + extraChance));
}

void MonsterType::dropLoot(Container* corpse)
{   
    uint32_t ownerId = corpse->getCorpseOwner();
    Player* owner = g_game.getPlayerByGuid(ownerId);
    
    uint32_t money = 0;
    ItemList items;
    std::stringstream str;
    std::stringstream payload;
    payload << "SendItemLoot@@@";
    
    for(LootItems::const_iterator it = lootItems.begin(); it != lootItems.end() && !corpse->full(); ++it)
    {
        items = createLoot(*it, owner);
        if(items.empty())
            continue;

        for(ItemList::iterator iit = items.begin(); iit != items.end(); ++iit)
        {
            Item* tmpItem = *iit;
            if(Container* container = tmpItem->getContainer())
            {
                if(createChildLoot(container, (*it), money, str, owner)) {
                    corpse->__internalAddThing(tmpItem);
                } else {
                    delete container;
                }
            }
            else {
                bool LootCatch = false;
                if(owner) {
                    if(owner->statusAutoLoot() == "On") {
                        LootCatch = owner->checkAutoLoot(tmpItem->getID());
                        if(LootCatch) {
                            if(owner->isMoneyAutoLoot(tmpItem, money)) {
                                continue;
                            }
                            g_game.internalPlayerAddItem(NULL, owner, tmpItem);
                            str << " " << tmpItem->getNameDescription() << ",";
                            
                            // Adicionando item ao payload para envio do opcode 180
                            payload << tmpItem->getClientID() << "@" << tmpItem->getName() << "@" << tmpItem->getItemCount() << "@";
                            continue;
                        }
                    }
                }
                corpse->__internalAddThing(tmpItem);
            }
        }
    }

    corpse->__startDecaying();
    
    if (!ownerId || !owner)
        return;
    
    if (money != 0) {
        if (owner->statusAutoMoneyCollect() == "Bank"){
            owner->balance += money;
        } else {
            g_game.addMoney(owner, money);
        }
        str << " " << money << "x coins.";
    } else {
        str << " nothing coins.";
    }

    LootMessage_t message = lootMessage;
    if (message == LOOTMSG_IGNORE)
        message = (LootMessage_t)g_config.getNumber(ConfigManager::LOOT_MESSAGE);

    if (message < LOOTMSG_PLAYER)
        return;

    std::stringstream ss;
    ss << "Loot of " << nameDescription << ": " << corpse->getContentDescription() << ".";
    if (owner->statusAutoLoot() == "On") {
        ss << "\nAutoLoot Collected:" << str.str();
    }
    if (owner->getParty() && message > LOOTMSG_PLAYER)
        owner->getParty()->broadcastMessage((MessageClasses)g_config.getNumber(ConfigManager::LOOT_MESSAGE_TYPE), ss.str());
    else if (message == LOOTMSG_PLAYER || message == LOOTMSG_BOTH)
    {
        std::string value = "-1";
        owner->getStorage("lootch", value);
        if (value == "-1")
            owner->sendTextMessage((MessageClasses)g_config.getNumber(ConfigManager::LOOT_MESSAGE_TYPE), ss.str());
        else
            owner->sendChannelMessage("", ss.str(), MSG_CHANNEL_MANAGEMENT, CHANNEL_LOOT);
    }
    
    // Enviar o opcode 180 com os itens coletados
    owner->sendExtendedOpcode(180, payload.str());
}



bool Monsters::loadFromXml(bool reloading /*= false*/)
{
	loaded = false;
	xmlDocPtr doc = xmlParseFile(getFilePath(FILE_TYPE_OTHER, "monster/monsters.xml").c_str());
	if(!doc)
	{
		std::clog << "[Warning - Monsters::loadFromXml] Cannot load monsters file." << std::endl;
		std::clog << getLastXMLError() << std::endl;
		return false;
	}

	xmlNodePtr root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"monsters"))
	{
		std::clog << "[Error - Monsters::loadFromXml] Malformed monsters file." << std::endl;
		xmlFreeDoc(doc);
		return false;
	}

	for(xmlNodePtr p = root->children; p; p = p->next)
	{
		if(p->type != XML_ELEMENT_NODE)
			continue;

		if(xmlStrcmp(p->name, (const xmlChar*)"monster"))
		{
			std::clog << "[Warning - Monsters::loadFromXml] Unknown node name (" << p->name << ")." << std::endl;
			continue;
		}

		std::string file, name;
		if(!readXMLString(p, "file", file) || !readXMLString(p, "name", name))
			continue;

		file = getFilePath(FILE_TYPE_OTHER, "monster/" + file);
		loadMonster(file, name, reloading);
	}

	xmlFreeDoc(doc);
	loaded = true;
	return loaded;
}

ConditionDamage* Monsters::getDamageCondition(ConditionType_t conditionType,
	int32_t maxDamage, int32_t minDamage, int32_t startDamage, uint32_t tickInterval)
{
	if(ConditionDamage* condition = dynamic_cast<ConditionDamage*>(Condition::createCondition(CONDITIONID_COMBAT, conditionType, 0)))
	{
		condition->setParam(CONDITIONPARAM_TICKINTERVAL, tickInterval);
		condition->setParam(CONDITIONPARAM_MINVALUE, minDamage);
		condition->setParam(CONDITIONPARAM_MAXVALUE, maxDamage);
		condition->setParam(CONDITIONPARAM_STARTVALUE, startDamage);
		condition->setParam(CONDITIONPARAM_DELAYED, 1);
		return condition;
	}

	return NULL;
}

bool Monsters::deserializeSpell(xmlNodePtr node, spellBlock_t& sb, const std::string& description)
{
	sb.range = sb.minCombatValue = sb.maxCombatValue = 0;
	sb.combatSpell = sb.isMelee = false;
	sb.chance = 100;
	sb.speed = 2000;

	std::string name, scriptName;
	bool isScripted = false;
	if(readXMLString(node, "script", scriptName))
		isScripted = true;
	else if(!readXMLString(node, "name", name))
		return false;

	int32_t intValue;
	std::string strValue;
	if(readXMLInteger(node, "speed", intValue) || readXMLInteger(node, "interval", intValue))
		sb.speed = std::max(1, intValue);

	if(readXMLInteger(node, "chance", intValue))
	{
		if(intValue < 0 || intValue > 100)
			intValue = 100;

		sb.chance = intValue;
	}

	if(readXMLInteger(node, "range", intValue))
	{
		if(intValue < 0)
			intValue = 0;

		if(intValue > Map::maxViewportX << 1)
			intValue = Map::maxViewportX << 1;

		sb.range = intValue;
	}

	if(readXMLInteger(node, "min", intValue))
		sb.minCombatValue = intValue;

	if(readXMLInteger(node, "max", intValue))
		sb.maxCombatValue = intValue;

	//normalize values
	if(std::abs(sb.minCombatValue) > std::abs(sb.maxCombatValue))
		std::swap(sb.minCombatValue, sb.maxCombatValue);

	if((sb.spell = g_spells->getSpellByName(name)))
		return true;

	CombatSpell* combatSpell = NULL;
	bool needTarget = false, needDirection = false;
	if(isScripted)
	{
		if(readXMLString(node, "direction", strValue))
			needDirection = booleanString(strValue);

		if(readXMLString(node, "target", strValue))
			needTarget = booleanString(strValue);

		combatSpell = new CombatSpell(NULL, needTarget, needDirection);
		if(!combatSpell->loadScript(getFilePath(FILE_TYPE_OTHER, g_spells->getScriptBaseName() + "/scripts/" + scriptName), true))
		{
			delete combatSpell;
			return false;
		}

		if(!combatSpell->loadScriptCombat())
		{
			delete combatSpell;
			return false;

		}

		combatSpell->getCombat()->setPlayerCombatValues(FORMULA_VALUE, sb.minCombatValue, 0, sb.maxCombatValue, 0, 0, 0, 0, 0, 0, 0);
	}
	else
	{
		Combat* combat = new Combat;
		sb.combatSpell = true;
		if(readXMLInteger(node, "length", intValue))
		{
			int32_t length = intValue;
			if(length > 0)
			{
				int32_t spread = 3;
				//need direction spell
				if(readXMLInteger(node, "spread", intValue))
					spread = std::max(0, intValue);

				CombatArea* area = new CombatArea();
				area->setupArea(length, spread);

				combat->setArea(area);
				needDirection = true;
			}
		}

		if(readXMLInteger(node, "radius", intValue))
		{
			int32_t radius = intValue;
			//target spell
			if(readXMLInteger(node, "target", intValue))
				needTarget = (intValue != 0);

			CombatArea* area = new CombatArea();
			area->setupArea(radius);
			combat->setArea(area);
		}

		std::string tmpName = asLowerCaseString(name);
		if(tmpName == "melee" || tmpName == "distance")
		{
			int32_t attack = 0, skill = 0;
			if(readXMLInteger(node, "attack", attack) && readXMLInteger(node, "skill", skill))
			{
				sb.minCombatValue = 0;
				sb.maxCombatValue = -Weapons::getMaxMeleeDamage(skill, attack);
			}

			uint32_t tickInterval = 2000;
			ConditionType_t conditionType = CONDITION_NONE;
			if(readXMLInteger(node, "bleed", intValue) || readXMLInteger(node, "physical", intValue))
			{
				conditionType = CONDITION_BLEEDING;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "suiton", intValue))
			{
				conditionType = CONDITION_SUITON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "fuuton", intValue))
			{
				conditionType = CONDITION_FUUTON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "doton", intValue))
			{
				conditionType = CONDITION_DOTON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "raiton", intValue))
			{
				conditionType = CONDITION_RAITON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "mokuton", intValue))
			{
				conditionType = CONDITION_MOKUTON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "shoton", intValue))
			{
				conditionType = CONDITION_SHOTON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "suna", intValue))
			{
				conditionType = CONDITION_SUNA;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "enton", intValue))
			{
				conditionType = CONDITION_ENTON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "hyoton", intValue))
			{
				conditionType = CONDITION_HYOTON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "yoton", intValue))
			{
				conditionType = CONDITION_YOTON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "ranton", intValue))
			{
				conditionType = CONDITION_RANTON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "bakuton", intValue))
			{
				conditionType = CONDITION_BAKUTON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "shakuton", intValue))
			{
				conditionType = CONDITION_SHAKUTON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "futton", intValue))
			{
				conditionType = CONDITION_FUTTON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "jinton", intValue))
			{
				conditionType = CONDITION_JINTON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "satetsu", intValue))
			{
				conditionType = CONDITION_SATETSU;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "taijutsu", intValue))
			{
				conditionType = CONDITION_TAIJUTSU;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "yin", intValue))
			{
				conditionType = CONDITION_YIN;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "kenjutsu", intValue))
			{
				conditionType = CONDITION_KENJUTSU;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "fire", intValue))
			{
				conditionType = CONDITION_FIRE;
				tickInterval = 9000;
			}
			else if(readXMLInteger(node, "energy", intValue))
			{
				conditionType = CONDITION_ENERGY;
				tickInterval = 10000;
			}
			else if(readXMLInteger(node, "poison", intValue) || readXMLInteger(node, "earth", intValue))
			{
				conditionType = CONDITION_POISON;
				tickInterval = 5000;
			}
			else if(readXMLInteger(node, "freeze", intValue) || readXMLInteger(node, "ice", intValue))
			{
				conditionType = CONDITION_FREEZING;
				tickInterval = 8000;
			}
			else if(readXMLInteger(node, "dazzle", intValue) || readXMLInteger(node, "holy", intValue))
			{
				conditionType = CONDITION_DAZZLED;
				tickInterval = 10000;
			}
			else if(readXMLInteger(node, "curse", intValue) || readXMLInteger(node, "death", intValue))
			{
				conditionType = CONDITION_CURSED;
				tickInterval = 4000;
			}
			else if(readXMLInteger(node, "drown", intValue))
			{
				conditionType = CONDITION_DROWN;
				tickInterval = 5000;
			}

			uint32_t damage = std::abs(intValue);
			if(readXMLInteger(node, "tick", intValue) && intValue > 0)
				tickInterval = intValue;

			if(conditionType != CONDITION_NONE)
			{
				Condition* condition = getDamageCondition(conditionType, damage, damage, 0, tickInterval);
				if(condition)
					combat->setCondition(condition);
			}

			sb.isMelee = true;
			sb.range = 1;

			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_PHYSICALDAMAGE);
			combat->setParam(COMBATPARAM_BLOCKEDBYSHIELD, 1);
			combat->setParam(COMBATPARAM_BLOCKEDBYARMOR, 1);
		}
		else if(tmpName == "physical")
		{
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_PHYSICALDAMAGE);
			combat->setParam(COMBATPARAM_BLOCKEDBYARMOR, 1);
		}
		else if(tmpName == "suiton")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_SUITONDAMAGE);
		else if(tmpName == "fuuton")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_FUUTONDAMAGE);
		else if(tmpName == "doton")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_DOTONDAMAGE);
		else if(tmpName == "raiton")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_RAITONDAMAGE);
		else if(tmpName == "mokuton")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_MOKUTONDAMAGE);
		else if(tmpName == "shoton")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_SHOTONDAMAGE);
		else if(tmpName == "suna")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_SUNADAMAGE);
		else if(tmpName == "enton")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_ENTONDAMAGE);
		else if(tmpName == "hyoton")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_HYOTONDAMAGE);
		else if(tmpName == "yoton")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_YOTONDAMAGE);
		else if(tmpName == "ranton")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_RANTONDAMAGE);
		else if(tmpName == "bakuton")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_BAKUTONDAMAGE);
		else if(tmpName == "shakuton")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_SHAKUTONDAMAGE);
		else if(tmpName == "futton")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_FUTTONDAMAGE);
		else if(tmpName == "jinton")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_JINTONDAMAGE);
		else if(tmpName == "satetsu")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_SATETSUDAMAGE);
		else if(tmpName == "taijutsu")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_TAIJUTSUDAMAGE);
		else if(tmpName == "yin")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_YINDAMAGE);
		else if(tmpName == "yang")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_YANGDAMAGE);
		else if(tmpName == "kenjutsu")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_KENJUTSUDAMAGE);
		else if(tmpName == "bleed")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_PHYSICALDAMAGE);
		else if(tmpName == "drown")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_DROWNDAMAGE);
		else if(tmpName == "fire")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_FIREDAMAGE);
		else if(tmpName == "energy")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_ENERGYDAMAGE);
		else if(tmpName == "poison" || tmpName == "earth")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_EARTHDAMAGE);
		else if(tmpName == "ice")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_ICEDAMAGE);
		else if(tmpName == "holy")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_HOLYDAMAGE);
		else if(tmpName == "death")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_DEATHDAMAGE);
		else if(tmpName == "lifedrain")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_LIFEDRAIN);
		else if(tmpName == "manadrain")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_MANADRAIN);
		else if(tmpName == "healing")
		{
			bool aggressive = false;
			if(readXMLInteger(node, "self", intValue))
				aggressive = (intValue != 0);

			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_HEALING);
			combat->setParam(COMBATPARAM_AGGRESSIVE, aggressive);
		}
		else if(tmpName == "undefined")
			combat->setParam(COMBATPARAM_COMBATTYPE, COMBAT_UNDEFINEDDAMAGE);
		else if(tmpName == "speed")
		{
			int32_t speedChange = 0, duration = 10000;
			if(readXMLInteger(node, "duration", intValue))
				duration = intValue;

			enum Aggressive {
				NO,
				YES,
				AUTO
			} aggressive = AUTO;
			if(readXMLInteger(node, "self", intValue))
				aggressive = (Aggressive)intValue;

			if(readXMLInteger(node, "speedchange", intValue))
				speedChange = std::max(-1000, intValue); //cant be slower than 100%

			std::vector<Outfit_t> outfits;
			for(xmlNodePtr tmpNode = node->children; tmpNode; tmpNode = tmpNode->next)
			{
				if(xmlStrcmp(tmpNode->name,(const xmlChar*)"outfit"))
					continue;

				if(readXMLInteger(tmpNode, "type", intValue))
				{
					Outfit_t outfit;
					outfit.lookType = intValue;
					if(readXMLInteger(tmpNode, "head", intValue))
						outfit.lookHead = intValue;

					if(readXMLInteger(tmpNode, "body", intValue))
						outfit.lookBody = intValue;

					if(readXMLInteger(tmpNode, "legs", intValue))
						outfit.lookLegs = intValue;

					if(readXMLInteger(tmpNode, "feet", intValue))
						outfit.lookFeet = intValue;

					if(readXMLInteger(tmpNode, "addons", intValue))
						outfit.lookAddons = intValue;

					if(readXMLInteger(tmpNode, "wings", intValue)) {
						Wing* wing = Wings::getInstance()->getWing(intValue);
						outfit.lookWing = wing ? wing->clientId : 0;
					}

					if(readXMLInteger(tmpNode, "aura", intValue)) {
						Aura* aura = Auras::getInstance()->getAura(intValue);
						outfit.lookAura = aura ? aura->clientId : 0;
					}

					// if(readXMLString(tmpNode, "shader", strValue)) {
						// Shader* shader = Shaders::getInstance()->getShaderByName(strValue);
						// outfit.lookShader = shader ? shader->clientId : 0;
					// }

					outfits.push_back(outfit);
				}

				if(readXMLInteger(tmpNode, "typeex", intValue) || readXMLInteger(tmpNode, "item", intValue))
				{
					Outfit_t outfit;
					outfit.lookTypeEx = intValue;
					outfits.push_back(outfit);
				}

				if(readXMLString(tmpNode, "monster", strValue))
				{
					if(MonsterType* mType = g_monsters.getMonsterType(strValue))
						outfits.push_back(mType->outfit);
				}
			}

			ConditionType_t conditionType = CONDITION_PARALYZE;
			if(speedChange > 0)
			{
				conditionType = CONDITION_HASTE;
				if(aggressive == AUTO)
					aggressive = NO;
			}
			else if(aggressive == AUTO)
				aggressive = YES;

			if(ConditionSpeed* condition = dynamic_cast<ConditionSpeed*>(Condition::createCondition(
				CONDITIONID_COMBAT, conditionType, duration)))
			{
				condition->setFormulaVars((speedChange / 1000.), 0, (speedChange / 1000.), 0);
				if(!outfits.empty())
					condition->setOutfits(outfits);

				combat->setCondition(condition);
				combat->setParam(COMBATPARAM_AGGRESSIVE, aggressive);
			}
		}
		else if(tmpName == "outfit")
		{
			std::vector<Outfit_t> outfits;
			for(xmlNodePtr tmpNode = node->children; tmpNode; tmpNode = tmpNode->next)
			{
				if(xmlStrcmp(tmpNode->name,(const xmlChar*)"outfit"))
					continue;

				if(readXMLInteger(tmpNode, "type", intValue))
				{
					Outfit_t outfit;
					outfit.lookType = intValue;
					if(readXMLInteger(tmpNode, "head", intValue))
						outfit.lookHead = intValue;

					if(readXMLInteger(tmpNode, "body", intValue))
						outfit.lookBody = intValue;

					if(readXMLInteger(tmpNode, "legs", intValue))
						outfit.lookLegs = intValue;

					if(readXMLInteger(tmpNode, "feet", intValue))
						outfit.lookFeet = intValue;

					if(readXMLInteger(tmpNode, "addons", intValue))
						outfit.lookAddons = intValue;

					outfits.push_back(outfit);
				}

				if(readXMLInteger(tmpNode, "typeex", intValue) || readXMLInteger(tmpNode, "item", intValue))
				{
					Outfit_t outfit;
					outfit.lookTypeEx = intValue;
					outfits.push_back(outfit);
				}

				if(readXMLString(tmpNode, "monster", strValue))
				{
					if(MonsterType* mType = g_monsters.getMonsterType(strValue))
						outfits.push_back(mType->outfit);
				}
			}

			if(outfits.empty())
			{
				if(readXMLInteger(node, "type", intValue))
				{
					Outfit_t outfit;
					outfit.lookType = intValue;
					if(readXMLInteger(node, "head", intValue))
						outfit.lookHead = intValue;

					if(readXMLInteger(node, "body", intValue))
						outfit.lookBody = intValue;

					if(readXMLInteger(node, "legs", intValue))
						outfit.lookLegs = intValue;

					if(readXMLInteger(node, "feet", intValue))
						outfit.lookFeet = intValue;

					if(readXMLInteger(node, "addons", intValue))
						outfit.lookAddons = intValue;

					outfits.push_back(outfit);
				}

				if(readXMLInteger(node, "typeex", intValue) || readXMLInteger(node, "item", intValue))
				{
					Outfit_t outfit;
					outfit.lookTypeEx = intValue;
					outfits.push_back(outfit);
				}

				if(readXMLString(node, "monster", strValue))
				{
					if(MonsterType* mType = g_monsters.getMonsterType(strValue))
						outfits.push_back(mType->outfit);
				}
			}

			if(!outfits.empty())
			{
				int32_t duration = 10000;
				if(readXMLInteger(node, "duration", intValue))
					duration = intValue;

				bool aggressive = false;
				if(readXMLInteger(node, "self", intValue))
					aggressive = (intValue != 0);

				if(ConditionOutfit* condition = dynamic_cast<ConditionOutfit*>(Condition::createCondition(
					CONDITIONID_COMBAT, CONDITION_OUTFIT, duration)))
				{
					condition->setOutfits(outfits);
					combat->setCondition(condition);
					combat->setParam(COMBATPARAM_AGGRESSIVE, aggressive);
				}
			}
		}
		else if(tmpName == "invisible")
		{
			int32_t duration = 10000;
			if(readXMLInteger(node, "duration", intValue))
				duration = intValue;

			bool aggressive = false;
			if(readXMLInteger(node, "self", intValue))
				aggressive = (intValue != 0);

			if(Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_INVISIBLE, duration))
			{
				combat->setParam(COMBATPARAM_AGGRESSIVE, aggressive);
				combat->setCondition(condition);
			}
		}
		else if(tmpName == "drunk")
		{
			int32_t duration = 10000, subId = 0;
			if(readXMLInteger(node, "duration", intValue))
				duration = intValue;

			if(readXMLInteger(node, "subid", intValue))
				subId = intValue;

			if(Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_DRUNK, duration, 0, false, subId))
				combat->setCondition(condition);
		}
		else if(tmpName == "skills" || tmpName == "attributes")
		{
			uint32_t duration = 10000, subId = 0;
			if(readXMLInteger(node, "duration", intValue))
				duration = intValue;

			if(readXMLInteger(node, "subid", intValue))
				subId = intValue;

			intValue = 0;
			ConditionParam_t param = CONDITIONPARAM_BUFF; //to know was it loaded
			if(readXMLInteger(node, "melee", intValue))
				param = CONDITIONPARAM_SKILL_MELEE;
			else if(readXMLInteger(node, "fist", intValue))
				param = CONDITIONPARAM_SKILL_FIST;
			else if(readXMLInteger(node, "club", intValue))
				param = CONDITIONPARAM_SKILL_CLUB;
			else if(readXMLInteger(node, "axe", intValue))
				param = CONDITIONPARAM_SKILL_AXE;
			else if(readXMLInteger(node, "sword", intValue))
				param = CONDITIONPARAM_SKILL_SWORD;
			else if(readXMLInteger(node, "distance", intValue) || readXMLInteger(node, "dist", intValue))
				param = CONDITIONPARAM_SKILL_DISTANCE;
			else if(readXMLInteger(node, "shielding", intValue) || readXMLInteger(node, "shield", intValue))
				param = CONDITIONPARAM_SKILL_SHIELD;
			else if(readXMLInteger(node, "fishing", intValue) || readXMLInteger(node, "fish", intValue))
				param = CONDITIONPARAM_SKILL_FISHING;
			else if(readXMLInteger(node, "meleePercent", intValue))
				param = CONDITIONPARAM_SKILL_MELEEPERCENT;
			else if(readXMLInteger(node, "fistPercent", intValue))
				param = CONDITIONPARAM_SKILL_FISTPERCENT;
			else if(readXMLInteger(node, "clubPercent", intValue))
				param = CONDITIONPARAM_SKILL_CLUBPERCENT;
			else if(readXMLInteger(node, "axePercent", intValue))
				param = CONDITIONPARAM_SKILL_AXEPERCENT;
			else if(readXMLInteger(node, "swordPercent", intValue))
				param = CONDITIONPARAM_SKILL_SWORDPERCENT;
			else if(readXMLInteger(node, "distancePercent", intValue) || readXMLInteger(node, "distPercent", intValue))
				param = CONDITIONPARAM_SKILL_DISTANCEPERCENT;
			else if(readXMLInteger(node, "shieldingPercent", intValue) || readXMLInteger(node, "shieldPercent", intValue))
				param = CONDITIONPARAM_SKILL_SHIELDPERCENT;
			else if(readXMLInteger(node, "fishingPercent", intValue) || readXMLInteger(node, "fishPercent", intValue))
				param = CONDITIONPARAM_SKILL_FISHINGPERCENT;
			else if(readXMLInteger(node, "maxhealth", intValue))
				param = CONDITIONPARAM_STAT_MAXHEALTHPERCENT;
			else if(readXMLInteger(node, "maxmana", intValue))
				param = CONDITIONPARAM_STAT_MAXMANAPERCENT;
			else if(readXMLInteger(node, "soul", intValue))
				param = CONDITIONPARAM_STAT_SOULPERCENT;
			else if(readXMLInteger(node, "magiclevel", intValue) || readXMLInteger(node, "maglevel", intValue))
				param = CONDITIONPARAM_STAT_MAGICLEVELPERCENT;
			else if(readXMLInteger(node, "maxhealthPercent", intValue))
				param = CONDITIONPARAM_STAT_MAXHEALTHPERCENT;
			else if(readXMLInteger(node, "maxmanaPercent", intValue))
				param = CONDITIONPARAM_STAT_MAXMANAPERCENT;
			else if(readXMLInteger(node, "soulPercent", intValue))
				param = CONDITIONPARAM_STAT_SOULPERCENT;
			else if(readXMLInteger(node, "magiclevelPercent", intValue) || readXMLInteger(node, "maglevelPercent", intValue))
				param = CONDITIONPARAM_STAT_MAGICLEVELPERCENT;

			if(param != CONDITIONPARAM_BUFF)
			{
				if(ConditionAttributes* condition = dynamic_cast<ConditionAttributes*>(Condition::createCondition(
					CONDITIONID_COMBAT, CONDITION_ATTRIBUTES, duration, 0, false, subId)))
				{
					condition->setParam(param, intValue);
					combat->setCondition(condition);
				}
			}
		}
		else if(tmpName == "firefield")
			combat->setParam(COMBATPARAM_CREATEITEM, ITEM_FIREFIELD);
		else if(tmpName == "poisonfield")
			combat->setParam(COMBATPARAM_CREATEITEM, ITEM_POISONFIELD);
		else if(tmpName == "energyfield")
			combat->setParam(COMBATPARAM_CREATEITEM, ITEM_ENERGYFIELD);
		else if(tmpName == "physicalcondition" || tmpName == "bleedcondition" ||
			tmpName == "firecondition" || tmpName == "energycondition" ||
			tmpName == "earthcondition" || tmpName == "poisoncondition" ||
			tmpName == "icecondition" || tmpName == "freezecondition" ||
			tmpName == "deathcondition" || tmpName == "cursecondition" ||
			tmpName == "holycondition" || tmpName == "dazzlecondition" ||
			tmpName == "drowncondition" || tmpName == "suitoncondition" ||
			tmpName == "fuutoncondition" || tmpName == "raitoncondition" ||
			tmpName == "dotoncondition" || tmpName == "mokutoncondition" ||
			tmpName == "shotoncondition" || tmpName == "sunacondition" ||
			tmpName == "entoncondition" || tmpName == "hyotoncondition" ||
			tmpName == "yotoncondition" || tmpName == "rantoncondition" ||
			tmpName == "bakutoncondition" || tmpName == "shakutoncondition" ||
			tmpName == "futtoncondition" || tmpName == "jintoncondition" ||
			tmpName == "satetsucondition" || tmpName == "taijutsucondition"|| 
			tmpName == "yincondition"|| tmpName == "yangcondition" || 
			tmpName == "invulnerablecondition" || tmpName == "battlelockcondition" || 
			tmpName == "kenjutsucondition" )
		{
			ConditionType_t conditionType = CONDITION_NONE;
			uint32_t tickInterval = 2000;
			if(tmpName == "physicalcondition" || tmpName == "bleedcondition")
			{
				conditionType = CONDITION_BLEEDING;
				tickInterval = 5000;
			}
			else if(tmpName == "suitoncondition")
			{
				conditionType = CONDITION_SUITON;
				tickInterval = 5000;
			}
			else if(tmpName == "fuutoncondition")
			{
				conditionType = CONDITION_FUUTON;
				tickInterval = 5000;
			}
			else if(tmpName == "dotoncondition")
			{
				conditionType = CONDITION_DOTON;
				tickInterval = 5000;
			}
			else if(tmpName == "raitoncondition")
			{
				conditionType = CONDITION_RAITON;
				tickInterval = 5000;
			}
			else if(tmpName == "mokutoncondition")
			{
				conditionType = CONDITION_MOKUTON;
				tickInterval = 5000;
			}
			else if(tmpName == "shotoncondition")
			{
				conditionType = CONDITION_SHOTON;
				tickInterval = 5000;
			}
			else if(tmpName == "sunacondition")
			{
				conditionType = CONDITION_SUNA;
				tickInterval = 5000;
			}
			else if(tmpName == "entoncondition")
			{
				conditionType = CONDITION_ENTON;
				tickInterval = 5000;
			}
			else if(tmpName == "hyotoncondition")
			{
				conditionType = CONDITION_HYOTON;
				tickInterval = 5000;
			}
			else if(tmpName == "yotoncondition")
			{
				conditionType = CONDITION_YOTON;
				tickInterval = 5000;
			}
			else if(tmpName == "rantoncondition")
			{
				conditionType = CONDITION_RANTON;
				tickInterval = 5000;
			}
			else if(tmpName == "bakutoncondition")
			{
				conditionType = CONDITION_BAKUTON;
				tickInterval = 5000;
			}
			else if(tmpName == "shakutoncondition")
			{
				conditionType = CONDITION_SHAKUTON;
				tickInterval = 5000;
			}
			else if(tmpName == "futtoncondition")
			{
				conditionType = CONDITION_FUTTON;
				tickInterval = 5000;
			}
			else if(tmpName == "jintoncondition")
			{
				conditionType = CONDITION_JINTON;
				tickInterval = 5000;
			}
			else if(tmpName == "satetsucondition")
			{
				conditionType = CONDITION_SATETSU;
				tickInterval = 5000;
			}
			else if(tmpName == "taijutsucondition")
			{
				conditionType = CONDITION_TAIJUTSU;
				tickInterval = 5000;
			}
			else if(tmpName == "yincondition")
			{
				conditionType = CONDITION_YIN;
				tickInterval = 5000;
			}
			else if(tmpName == "yangcondition")
			{
				conditionType = CONDITION_YANG;
				tickInterval = 5000;
			}
			else if(tmpName == "kenjutsucondition")
			{
				conditionType = CONDITION_KENJUTSU;
				tickInterval = 5000;
			}
			else if(tmpName == "firecondition")
			{
				conditionType = CONDITION_FIRE;
				tickInterval = 9000;
			}
			else if(tmpName == "energycondition")
			{
				conditionType = CONDITION_ENERGY;
				tickInterval = 10000;
			}
			else if(tmpName == "earthcondition" || tmpName == "poisoncondition")
			{
				conditionType = CONDITION_POISON;
				tickInterval = 5000;
			}
			else if(tmpName == "icecondition" || tmpName == "freezecondition")
			{
				conditionType = CONDITION_FREEZING;
				tickInterval = 8000;
			}
			else if(tmpName == "deathcondition" || tmpName == "cursecondition")
			{
				conditionType = CONDITION_CURSED;
				tickInterval = 4000;
			}
			else if(tmpName == "holycondition" || tmpName == "dazzlecondition")
			{
				conditionType = CONDITION_DAZZLED;
				tickInterval = 10000;
			}
			else if(tmpName == "drowncondition")
			{
				conditionType = CONDITION_DROWN;
				tickInterval = 5000;
			}

			if(readXMLInteger(node, "tick", intValue) && intValue > 0)
				tickInterval = intValue;

			int32_t startDamage = 0, minDamage = std::abs(sb.minCombatValue), maxDamage = std::abs(sb.maxCombatValue);
			if(readXMLInteger(node, "start", intValue))
				startDamage = std::max(std::abs(intValue), minDamage);

			if(Condition* condition = getDamageCondition(conditionType, maxDamage, minDamage, startDamage, tickInterval))
				combat->setCondition(condition);
		}
		else if(tmpName == "strength")
		{
			//TODO: monster extra strength
		}
		else if(tmpName == "effect")
			{/*show some effect and bye bye!*/}
		else
		{
			delete combat;
			std::clog << "[Error - Monsters::deserializeSpell] " << description << " - Unknown spell name: " << name << std::endl;
			return false;
		}

		combat->setPlayerCombatValues(FORMULA_VALUE, sb.minCombatValue, 0, sb.maxCombatValue, 0, 0, 0, 0, 0, 0, 0);
		combatSpell = new CombatSpell(combat, needTarget, needDirection);

		for(xmlNodePtr attributeNode = node->children; attributeNode; attributeNode = attributeNode->next)
		{
			if(!xmlStrcmp(attributeNode->name, (const xmlChar*)"attribute"))
			{
				if(readXMLString(attributeNode, "key", strValue))
				{
					std::string tmpStrValue = asLowerCaseString(strValue);
					if(tmpStrValue == "shooteffect")
					{
						if(readXMLString(attributeNode, "value", strValue))
						{
							ShootEffect_t shoot = getShootType(strValue);
							if(shoot != SHOOT_EFFECT_UNKNOWN)
								combat->setParam(COMBATPARAM_DISTANCEEFFECT, shoot);
							else
								std::clog << "[Warning - Monsters::deserializeSpell] " << description << " - Unknown shootEffect: " << strValue << std::endl;
						}
					}
					else if(tmpStrValue == "areaeffect")
					{
						if(readXMLString(attributeNode, "value", strValue))
						{
							MagicEffect_t effect = getMagicEffect(strValue);
							if(effect != MAGIC_EFFECT_UNKNOWN)
								combat->setParam(COMBATPARAM_EFFECT, effect);
							else
								std::clog << "[Warning - Monsters::deserializeSpell] " << description << " - Unknown areaEffect: " << strValue << std::endl;
						}
					}
					else
						std::clog << "[Warning - Monsters::deserializeSpells] Effect type \"" << strValue << "\" does not exist." << std::endl;
				}
			}
		}
	}

	sb.spell = combatSpell;
	return true;
}

#define SHOW_XML_WARNING(desc) std::clog << "[Warning - Monsters::loadMonster] " << desc << " (" << file << ")." << std::endl;
#define SHOW_XML_ERROR(desc) std::clog << "[Error - Monsters::loadMonster] " << desc << " (" << file << ")." << std::endl;

bool Monsters::loadMonster(const std::string& file, const std::string& monsterName, bool reloading/* = false*/)
{
	if(getIdByName(monsterName) && !reloading)
	{
		std::clog << "[Warning - Monsters::loadMonster] Duplicate registered monster with name: " << monsterName << std::endl;
		return true;
	}

	bool monsterLoad, new_mType = true;
	MonsterType* mType = NULL;
	if(reloading)
	{
		uint32_t id = getIdByName(monsterName);
		if(id != 0)
		{
			mType = getMonsterType(id);
			if(mType != NULL)
			{
				new_mType = false;
				mType->reset();
			}
		}
	}

	if(new_mType)
		mType = new MonsterType();

	xmlDocPtr doc = xmlParseFile(file.c_str());
	if(!doc)
	{
		std::clog << "[Warning - Monsters::loadMonster] Cannot load monster (" << monsterName << ") file (" << file << ")." << std::endl;
		std::clog << getLastXMLError() << std::endl;
		return false;
	}

	monsterLoad = true;
	xmlNodePtr root = xmlDocGetRootElement(doc);
	if(xmlStrcmp(root->name,(const xmlChar*)"monster"))
	{
		std::clog << "[Error - Monsters::loadMonster] Malformed monster (" << monsterName << ") file (" << file << ")." << std::endl;
		xmlFreeDoc(doc);
		return false;
	}

	int32_t intValue;
	std::string strValue;
	if(readXMLString(root, "name", strValue))
		mType->name = strValue;
	else
		monsterLoad = false;

	mType->file = file;
	if(readXMLString(root, "nameDescription", strValue))
		mType->nameDescription = strValue;
	else
	{
		mType->nameDescription = "a " + mType->name;
		toLowerCaseString(mType->nameDescription);
	}

	if(readXMLString(root, "race", strValue))
	{
		std::string tmpStrValue = asLowerCaseString(strValue);
		if(tmpStrValue == "venom")
			mType->race = RACE_VENOM;
		else if(tmpStrValue == "blood")
			mType->race = RACE_BLOOD;
		else if(tmpStrValue == "undead")
			mType->race = RACE_UNDEAD;
		else if(tmpStrValue == "fire")
			mType->race = RACE_FIRE;
		else if(tmpStrValue == "energy")
			mType->race = RACE_ENERGY;
		else
		{
			int32_t race = atoi(strValue.c_str());
			if(race < RACE_VENOM || race > RACE_ENERGY)
			{
				SHOW_XML_WARNING("Unknown race type " << strValue);
			}
			else
				mType->race = (RaceType_t)race;
		}
	}

	if(readXMLInteger(root, "experience", intValue))
		mType->experience = intValue;
	
	if(readXMLInteger(root, "saga", intValue))
		mType->saga = intValue;
	
	if(readXMLInteger(root, "missao", intValue))
		mType->missao = intValue;

	if(readXMLInteger(root, "speed", intValue))
		mType->baseSpeed = intValue;

	if(readXMLInteger(root, "manacost", intValue))
		mType->manaCost = intValue;

	if(readXMLString(root, "skull", strValue))
		mType->skull = getSkulls(strValue);

	if(readXMLString(root, "shield", strValue))
		mType->partyShield = getShields(strValue);

	if(readXMLString(root, "emblem", strValue))
		mType->guildEmblem = getEmblems(strValue);

	for(xmlNodePtr p = root->children; p; p = p->next)
	{
		if(p->type != XML_ELEMENT_NODE)
			continue;

		if(!xmlStrcmp(p->name, (const xmlChar*)"health"))
		{
			if(readXMLInteger(p, "min", intValue))
				mType->healthMin = intValue;

			if(!readXMLInteger(p, "max", intValue))
			{
				SHOW_XML_ERROR("Missing health.max");
				monsterLoad = false;
				break;
			}

			mType->healthMax = intValue;
			if(!readXMLInteger(p, "now", intValue))
				mType->health = mType->healthMax;
			else
				mType->health = intValue;
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"flags"))
		{
			for(xmlNodePtr tmpNode = p->children; tmpNode; tmpNode = tmpNode->next)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"flag"))
				{
					if(readXMLString(tmpNode, "summonable", strValue))
						mType->isSummonable = booleanString(strValue);
					
					// Wonsr
					if(readXMLString(tmpNode, "canwalkeverywhere", strValue)) // PS
						mType->canWalkEverywhere = booleanString(strValue);
						
					if(readXMLString(tmpNode, "limbo", strValue))
						mType->isLimbo = booleanString(strValue);
					
					if(readXMLString(tmpNode, "puppet", strValue))
						mType->isPuppet = booleanString(strValue);
					
					if(readXMLString(tmpNode, "boss", strValue))
						mType->isBoss = booleanString(strValue);
					
					//

					if(readXMLString(tmpNode, "attackable", strValue))
						mType->isAttackable = booleanString(strValue);

					if(readXMLString(tmpNode, "hostile", strValue))
						mType->isHostile = booleanString(strValue);

					if (readXMLString(tmpNode, "playerblockspawn", strValue))
						mType->playerblockspawn = booleanString(strValue);

					if(readXMLString(tmpNode, "illusionable", strValue))
						mType->isIllusionable = booleanString(strValue);

					if(readXMLString(tmpNode, "convinceable", strValue))
						mType->isConvinceable = booleanString(strValue);

					if(readXMLString(tmpNode, "pushable", strValue))
						mType->pushable = booleanString(strValue);

					if(readXMLString(tmpNode, "canpushitems", strValue))
						mType->canPushItems = booleanString(strValue);

					if(readXMLString(tmpNode, "canpushcreatures", strValue))
						mType->canPushCreatures = booleanString(strValue);

					if(readXMLString(tmpNode, "hidename", strValue))
						mType->hideName = booleanString(strValue);

					if(readXMLString(tmpNode, "hidehealth", strValue))
						mType->hideHealth = booleanString(strValue);

					if(readXMLInteger(tmpNode, "lootmessage", intValue))
						mType->lootMessage = (LootMessage_t)intValue;

					if(readXMLInteger(tmpNode, "staticattack", intValue))
					{
						if(intValue < 0 || intValue > 100)
						{
							SHOW_XML_WARNING("staticattack lower than 0 or greater than 100");
							intValue = 0;
						}

						mType->staticAttackChance = intValue;
					}

					if(readXMLInteger(tmpNode, "lightlevel", intValue))
						mType->lightLevel = intValue;

					if(readXMLInteger(tmpNode, "lightcolor", intValue))
						mType->lightColor = intValue;

					if(readXMLInteger(tmpNode, "targetdistance", intValue))
					{
						if(intValue > Map::maxViewportX)
							SHOW_XML_WARNING("targetdistance greater than maxViewportX");

						mType->targetDistance = std::max(1, intValue);
					}

					if(readXMLInteger(tmpNode, "runonhealth", intValue))
						mType->runAwayHealth = intValue;

					if(readXMLString(tmpNode, "lureable", strValue))
						mType->isLureable = booleanString(strValue);

					if(readXMLString(tmpNode, "walkable", strValue))
						mType->isWalkable = booleanString(strValue);

					if(readXMLString(tmpNode, "skull", strValue))
						mType->skull = getSkulls(strValue);

					if(readXMLString(tmpNode, "shield", strValue))
						mType->partyShield = getShields(strValue);

					if(readXMLString(tmpNode, "emblem", strValue))
						mType->guildEmblem = getEmblems(strValue);

					if(readXMLString(tmpNode, "eliminable", strValue))
						mType->eliminable = booleanString(strValue);
				}
			}

			//if a monster can push creatures, it should not be pushable
			if(mType->canPushCreatures && mType->pushable)
				mType->pushable = false;
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"targetchange"))
		{
			if(readXMLInteger(p, "speed", intValue) || readXMLInteger(p, "interval", intValue))
				mType->changeTargetSpeed = std::max(1, intValue);
			else
				SHOW_XML_WARNING("Missing targetchange.speed");

			if(readXMLInteger(p, "chance", intValue))
				mType->changeTargetChance = intValue;
			else
				SHOW_XML_WARNING("Missing targetchange.chance");
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"strategy"))
		{
			if(readXMLInteger(p, "attack", intValue)) {}
				//mType->attackStrength = intValue;

			if(readXMLInteger(p, "defense", intValue)) {}
				//mType->defenseStrength = intValue;
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"look"))
		{
			if(readXMLInteger(p, "type", intValue))
			{
				mType->outfit.lookType = intValue;
				if(readXMLInteger(p, "head", intValue))
					mType->outfit.lookHead = intValue;

				if(readXMLInteger(p, "body", intValue))
					mType->outfit.lookBody = intValue;

				if(readXMLInteger(p, "legs", intValue))
					mType->outfit.lookLegs = intValue;

				if(readXMLInteger(p, "feet", intValue))
					mType->outfit.lookFeet = intValue;

				if(readXMLInteger(p, "addons", intValue))
					mType->outfit.lookAddons = intValue;
			}
			else if(readXMLInteger(p, "typeex", intValue))
				mType->outfit.lookTypeEx = intValue;
			else
				SHOW_XML_WARNING("Missing look type/typeex");

			if(readXMLInteger(p, "corpse", intValue))
				mType->lookCorpse = intValue;

			if(readXMLInteger(p, "corpseUniqueId", intValue) || readXMLInteger(p, "corpseUid", intValue))
				mType->corpseUnique = intValue;

			if(readXMLInteger(p, "corpseActionId", intValue) || readXMLInteger(p, "corpseAid", intValue))
				mType->corpseAction = intValue;
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"attacks"))
		{
			for(xmlNodePtr tmpNode = p->children; tmpNode; tmpNode = tmpNode->next)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"attack"))
				{
					spellBlock_t sb;
					if(deserializeSpell(tmpNode, sb, monsterName))
						mType->spellAttackList.push_back(sb);
					else
						SHOW_XML_WARNING("Cant load spell");
				}
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"defenses"))
		{
			if(readXMLInteger(p, "defense", intValue))
				mType->defense = intValue;

			if(readXMLInteger(p, "armor", intValue))
				mType->armor = intValue;

			for(xmlNodePtr tmpNode = p->children; tmpNode; tmpNode = tmpNode->next)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"defense"))
				{
					spellBlock_t sb;
					if(deserializeSpell(tmpNode, sb, monsterName))
						mType->spellDefenseList.push_back(sb);
					else
						SHOW_XML_WARNING("Cant load spell");
				}
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"immunities"))
		{
			for(xmlNodePtr tmpNode = p->children; tmpNode; tmpNode = tmpNode->next)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"immunity"))
				{
					if(readXMLString(tmpNode, "name", strValue))
					{
						std::string tmpStrValue = asLowerCaseString(strValue);
						if(tmpStrValue == "physical")
						{
							mType->damageImmunities |= COMBAT_PHYSICALDAMAGE;
							mType->conditionImmunities |= CONDITION_BLEEDING;
						}
						else if(tmpStrValue == "suiton")
						{
							mType->damageImmunities |= COMBAT_SUITONDAMAGE;
							mType->conditionImmunities |= CONDITION_SUITON;
						}
						else if(tmpStrValue == "fuuton")
						{
							mType->damageImmunities |= COMBAT_FUUTONDAMAGE;
							mType->conditionImmunities |= CONDITION_FUUTON;
						}
						else if(tmpStrValue == "doton")
						{
							mType->damageImmunities |= COMBAT_DOTONDAMAGE;
							mType->conditionImmunities |= CONDITION_DOTON;
						}
						else if(tmpStrValue == "raiton")
						{
							mType->damageImmunities |= COMBAT_RAITONDAMAGE;
							mType->conditionImmunities |= CONDITION_RAITON;
						}
						else if(tmpStrValue == "mokuton")
						{
							mType->damageImmunities |= COMBAT_MOKUTONDAMAGE;
							mType->conditionImmunities |= CONDITION_MOKUTON;
						}
						else if(tmpStrValue == "shoton")
						{
							mType->damageImmunities |= COMBAT_SHOTONDAMAGE;
							mType->conditionImmunities |= CONDITION_SHOTON;
						}
						else if(tmpStrValue == "suna")
						{
							mType->damageImmunities |= COMBAT_SUNADAMAGE;
							mType->conditionImmunities |= CONDITION_SUNA;
						}
						else if(tmpStrValue == "enton")
						{
							mType->damageImmunities |= COMBAT_ENTONDAMAGE;
							mType->conditionImmunities |= CONDITION_ENTON;
						}
						else if(tmpStrValue == "hyoton")
						{
							mType->damageImmunities |= COMBAT_HYOTONDAMAGE;
							mType->conditionImmunities |= CONDITION_HYOTON;
						}
						else if(tmpStrValue == "yoton")
						{
							mType->damageImmunities |= COMBAT_YOTONDAMAGE;
							mType->conditionImmunities |= CONDITION_YOTON;
						}
						else if(tmpStrValue == "ranton")
						{
							mType->damageImmunities |= COMBAT_RANTONDAMAGE;
							mType->conditionImmunities |= CONDITION_RANTON;
						}
						else if(tmpStrValue == "bakuton")
						{
							mType->damageImmunities |= COMBAT_BAKUTONDAMAGE;
							mType->conditionImmunities |= CONDITION_BAKUTON;
						}
						else if(tmpStrValue == "shakuton")
						{
							mType->damageImmunities |= COMBAT_SHAKUTONDAMAGE;
							mType->conditionImmunities |= CONDITION_SHAKUTON;
						}
						else if(tmpStrValue == "futton")
						{
							mType->damageImmunities |= COMBAT_FUTTONDAMAGE;
							mType->conditionImmunities |= CONDITION_FUTTON;
						}
						else if(tmpStrValue == "jinton")
						{
							mType->damageImmunities |= COMBAT_JINTONDAMAGE;
							mType->conditionImmunities |= CONDITION_JINTON;
						}
						else if(tmpStrValue == "satetsu")
						{
							mType->damageImmunities |= COMBAT_SATETSUDAMAGE;
							mType->conditionImmunities |= CONDITION_SATETSU;
						}
						else if(tmpStrValue == "taijutsu")
						{
							mType->damageImmunities |= COMBAT_TAIJUTSUDAMAGE;
							mType->conditionImmunities |= CONDITION_TAIJUTSU;
						}
						else if(tmpStrValue == "yin")
						{
							mType->damageImmunities |= COMBAT_YINDAMAGE;
							mType->conditionImmunities |= CONDITION_YIN;
						}
						else if(tmpStrValue == "kenjutsu")
						{
							mType->damageImmunities |= COMBAT_KENJUTSUDAMAGE;
							mType->conditionImmunities |= CONDITION_KENJUTSU;
						}
						else if(tmpStrValue == "energy")
						{
							mType->damageImmunities |= COMBAT_ENERGYDAMAGE;
							mType->conditionImmunities |= CONDITION_ENERGY;
						}
						else if(tmpStrValue == "fire")
						{
							mType->damageImmunities |= COMBAT_FIREDAMAGE;
							mType->conditionImmunities |= CONDITION_FIRE;
						}
						else if(tmpStrValue == "poison" || tmpStrValue == "earth")
						{
							mType->damageImmunities |= COMBAT_EARTHDAMAGE;
							mType->conditionImmunities |= CONDITION_POISON;
						}
						else if(tmpStrValue == "ice")
						{
							mType->damageImmunities |= COMBAT_ICEDAMAGE;
							mType->conditionImmunities |= CONDITION_FREEZING;
						}
						else if(tmpStrValue == "holy")
						{
							mType->damageImmunities |= COMBAT_HOLYDAMAGE;
							mType->conditionImmunities |= CONDITION_DAZZLED;
						}
						else if(tmpStrValue == "death")
						{
							mType->damageImmunities |= COMBAT_DEATHDAMAGE;
							mType->conditionImmunities |= CONDITION_CURSED;
						}
						else if(tmpStrValue == "drown")
						{
							mType->damageImmunities |= COMBAT_DROWNDAMAGE;
							mType->conditionImmunities |= CONDITION_DROWN;
						}
						else if(tmpStrValue == "lifedrain")
							mType->damageImmunities |= COMBAT_LIFEDRAIN;
						else if(tmpStrValue == "manadrain")
							mType->damageImmunities |= COMBAT_MANADRAIN;
						else if(tmpStrValue == "paralyze")
							mType->conditionImmunities |= CONDITION_PARALYZE;
						else if(tmpStrValue == "outfit")
							mType->conditionImmunities |= CONDITION_OUTFIT;
						else if(tmpStrValue == "drunk")
							mType->conditionImmunities |= CONDITION_DRUNK;
						else if(tmpStrValue == "invisible")
							mType->conditionImmunities |= CONDITION_INVISIBLE;
						else if(tmpStrValue == "bleed")
							mType->conditionImmunities |= CONDITION_BLEEDING;
						else
							SHOW_XML_WARNING("Unknown immunity name " << strValue);
					}
					else if(readXMLString(tmpNode, "physical", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_PHYSICALDAMAGE;
						mType->conditionImmunities |= CONDITION_BLEEDING;
					}
					else if(readXMLString(tmpNode, "suiton", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_SUITONDAMAGE;
						mType->conditionImmunities |= CONDITION_SUITON;
					}
					else if(readXMLString(tmpNode, "fuuton", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_FUUTONDAMAGE;
						mType->conditionImmunities |= CONDITION_FUUTON;
					}
					else if(readXMLString(tmpNode, "doton", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_DOTONDAMAGE;
						mType->conditionImmunities |= CONDITION_DOTON;
					}
					else if(readXMLString(tmpNode, "raiton", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_RAITONDAMAGE;
						mType->conditionImmunities |= CONDITION_RAITON;
					}
					else if(readXMLString(tmpNode, "mokuton", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_MOKUTONDAMAGE;
						mType->conditionImmunities |= CONDITION_MOKUTON;
					}
					else if(readXMLString(tmpNode, "shoton", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_SHOTONDAMAGE;
						mType->conditionImmunities |= CONDITION_SHOTON;
					}
					else if(readXMLString(tmpNode, "suna", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_SUNADAMAGE;
						mType->conditionImmunities |= CONDITION_SUNA;
					}
					else if(readXMLString(tmpNode, "enton", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_ENTONDAMAGE;
						mType->conditionImmunities |= CONDITION_ENTON;
					}
					else if(readXMLString(tmpNode, "hyoton", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_HYOTONDAMAGE;
						mType->conditionImmunities |= CONDITION_HYOTON;
					}
					else if(readXMLString(tmpNode, "yoton", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_YOTONDAMAGE;
						mType->conditionImmunities |= CONDITION_YOTON;
					}
					else if(readXMLString(tmpNode, "ranton", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_RANTONDAMAGE;
						mType->conditionImmunities |= CONDITION_RANTON;
					}
					else if(readXMLString(tmpNode, "bakuton", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_BAKUTONDAMAGE;
						mType->conditionImmunities |= CONDITION_BAKUTON;
					}
					else if(readXMLString(tmpNode, "shakuton", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_SHAKUTONDAMAGE;
						mType->conditionImmunities |= CONDITION_SHAKUTON;
					}
					else if(readXMLString(tmpNode, "futton", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_FUTTONDAMAGE;
						mType->conditionImmunities |= CONDITION_FUTTON;
					}
					else if(readXMLString(tmpNode, "jinton", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_JINTONDAMAGE;
						mType->conditionImmunities |= CONDITION_JINTON;
					}
					else if(readXMLString(tmpNode, "satetsu", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_SATETSUDAMAGE;
						mType->conditionImmunities |= CONDITION_SATETSU;
					}
					else if(readXMLString(tmpNode, "taijutsu", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_TAIJUTSUDAMAGE;
						mType->conditionImmunities |= CONDITION_TAIJUTSU;
					}
					else if(readXMLString(tmpNode, "yin", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_YINDAMAGE;
						mType->conditionImmunities |= CONDITION_YIN;
					}
					else if(readXMLString(tmpNode, "yang", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_YANGDAMAGE;
						mType->conditionImmunities |= CONDITION_YANG;
					}
					else if(readXMLString(tmpNode, "kenjutsu", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_KENJUTSUDAMAGE;
						mType->conditionImmunities |= CONDITION_KENJUTSU;
					}
					else if(readXMLString(tmpNode, "energy", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_ENERGYDAMAGE;
						mType->conditionImmunities |= CONDITION_ENERGY;
					}
					else if(readXMLString(tmpNode, "fire", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_FIREDAMAGE;
						mType->conditionImmunities |= CONDITION_FIRE;
					}
					else if((readXMLString(tmpNode, "poison", strValue) || readXMLString(tmpNode, "earth", strValue))
						&& booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_EARTHDAMAGE;
						mType->conditionImmunities |= CONDITION_POISON;
					}
					else if(readXMLString(tmpNode, "drown", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_DROWNDAMAGE;
						mType->conditionImmunities |= CONDITION_DROWN;
					}
					else if(readXMLString(tmpNode, "ice", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_ICEDAMAGE;
						mType->conditionImmunities |= CONDITION_FREEZING;
					}
					else if(readXMLString(tmpNode, "holy", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_HOLYDAMAGE;
						mType->conditionImmunities |= CONDITION_DAZZLED;
					}
					else if(readXMLString(tmpNode, "death", strValue) && booleanString(strValue))
					{
						mType->damageImmunities |= COMBAT_DEATHDAMAGE;
						mType->conditionImmunities |= CONDITION_CURSED;
					}
					else if(readXMLString(tmpNode, "lifedrain", strValue) && booleanString(strValue))
						mType->damageImmunities |= COMBAT_LIFEDRAIN;
					else if(readXMLString(tmpNode, "manadrain", strValue) && booleanString(strValue))
						mType->damageImmunities |= COMBAT_LIFEDRAIN;
					else if(readXMLString(tmpNode, "paralyze", strValue) && booleanString(strValue))
						mType->conditionImmunities |= CONDITION_PARALYZE;
					else if(readXMLString(tmpNode, "outfit", strValue) && booleanString(strValue))
						mType->conditionImmunities |= CONDITION_OUTFIT;
					else if(readXMLString(tmpNode, "drunk", strValue) && booleanString(strValue))
						mType->conditionImmunities |= CONDITION_DRUNK;
					else if(readXMLString(tmpNode, "invisible", strValue) && booleanString(strValue))
						mType->conditionImmunities |= CONDITION_INVISIBLE;
					else if(readXMLString(tmpNode, "bleed", strValue) && booleanString(strValue))
						mType->conditionImmunities |= CONDITION_BLEEDING;
				}
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"voices"))
		{
			if(readXMLInteger(p, "speed", intValue) || readXMLInteger(p, "interval", intValue))
				mType->yellSpeedTicks = intValue;
			else
				SHOW_XML_WARNING("Missing voices.speed");

			if(readXMLInteger(p, "chance", intValue))
				mType->yellChance = intValue;
			else
				SHOW_XML_WARNING("Missing voices.chance");

			for(xmlNodePtr tmpNode = p->children; tmpNode; tmpNode = tmpNode->next)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"voice"))
				{
					voiceBlock_t vb;
					vb.text = "";
					vb.yellText = false;

					if(readXMLString(tmpNode, "sentence", strValue))
						vb.text = strValue;
					else
						SHOW_XML_WARNING("Missing voice.sentence");

					if(readXMLString(tmpNode, "yell", strValue))
						vb.yellText = booleanString(strValue);

					mType->voiceVector.push_back(vb);
				}
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"loot"))
		{
			for(xmlNodePtr tmpNode = p->children; tmpNode; tmpNode = tmpNode->next)
			{
				if(tmpNode->type != XML_ELEMENT_NODE)
					continue;

				LootBlock rootBlock;
				if(loadLoot(tmpNode, rootBlock))
					mType->lootItems.push_back(rootBlock);
				else
					SHOW_XML_WARNING("Cant load loot");
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"elements"))
		{
			for(xmlNodePtr tmpNode = p->children; tmpNode; tmpNode = tmpNode->next)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"element"))
				{
					if(readXMLInteger(tmpNode, "firePercent", intValue))
						mType->elementMap[COMBAT_FIREDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "energyPercent", intValue))
						mType->elementMap[COMBAT_ENERGYDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "icePercent", intValue))
						mType->elementMap[COMBAT_ICEDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "poisonPercent", intValue) || readXMLInteger(tmpNode, "earthPercent", intValue))
						mType->elementMap[COMBAT_EARTHDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "holyPercent", intValue))
						mType->elementMap[COMBAT_HOLYDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "deathPercent", intValue))
						mType->elementMap[COMBAT_DEATHDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "drownPercent", intValue))
						mType->elementMap[COMBAT_DROWNDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "physicalPercent", intValue))
						mType->elementMap[COMBAT_PHYSICALDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "lifeDrainPercent", intValue))
						mType->elementMap[COMBAT_LIFEDRAIN] = intValue;
					else if(readXMLInteger(tmpNode, "manaDrainPercent", intValue))
						mType->elementMap[COMBAT_MANADRAIN] = intValue;
					else if(readXMLInteger(tmpNode, "healingPercent", intValue))
						mType->elementMap[COMBAT_HEALING] = intValue;
					else if(readXMLInteger(tmpNode, "undefinedPercent", intValue))
						mType->elementMap[COMBAT_UNDEFINEDDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "suitonPercent", intValue))
						mType->elementMap[COMBAT_SUITONDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "fuutonPercent", intValue))
						mType->elementMap[COMBAT_FUUTONDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "dotonPercent", intValue))
						mType->elementMap[COMBAT_DOTONDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "raitonPercent", intValue))
						mType->elementMap[COMBAT_RAITONDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "mokutonPercent", intValue))
						mType->elementMap[COMBAT_MOKUTONDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "shotonPercent", intValue))
						mType->elementMap[COMBAT_SHOTONDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "sunaPercent", intValue))
						mType->elementMap[COMBAT_SUNADAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "entonPercent", intValue))
						mType->elementMap[COMBAT_ENTONDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "hyotonPercent", intValue))
						mType->elementMap[COMBAT_HYOTONDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "yotonPercent", intValue))
						mType->elementMap[COMBAT_YOTONDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "rantonPercent", intValue))
						mType->elementMap[COMBAT_RANTONDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "bakutonPercent", intValue))
						mType->elementMap[COMBAT_BAKUTONDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "shakutonPercent", intValue))
						mType->elementMap[COMBAT_SHAKUTONDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "futtonPercent", intValue))
						mType->elementMap[COMBAT_FUTTONDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "jintonPercent", intValue))
						mType->elementMap[COMBAT_JINTONDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "satetsuPercent", intValue))
						mType->elementMap[COMBAT_SATETSUDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "taijutsuPercent", intValue))
						mType->elementMap[COMBAT_TAIJUTSUDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "yinPercent", intValue))
						mType->elementMap[COMBAT_YINDAMAGE] = intValue;
					else if(readXMLInteger(tmpNode, "yangPercent", intValue))
						mType->elementMap[COMBAT_YANGDAMAGE] = intValue; 
					else if(readXMLInteger(tmpNode, "kenjutsuPercent", intValue))
						mType->elementMap[COMBAT_KENJUTSUDAMAGE] = intValue;
				}
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"summons"))
		{
			if(readXMLInteger(p, "maxSummons", intValue) || readXMLInteger(p, "max", intValue))
				mType->maxSummons = intValue;

			for(xmlNodePtr tmpNode = p->children; tmpNode; tmpNode = tmpNode->next)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"summon"))
				{
					uint32_t chance = 100, interval = 1000, amount = 1;
					if(readXMLInteger(tmpNode, "speed", intValue) || readXMLInteger(tmpNode, "interval", intValue))
						interval = intValue;

					if(readXMLInteger(tmpNode, "chance", intValue))
						chance = intValue;

					if(readXMLInteger(tmpNode, "amount", intValue) || readXMLInteger(tmpNode, "max", intValue))
						amount = intValue;

					if(readXMLString(tmpNode, "name", strValue))
					{
						summonBlock_t sb;
						sb.name = strValue;
						sb.interval = interval;
						sb.chance = chance;
						sb.amount = amount;

						mType->summonList.push_back(sb);
					}
					else
						SHOW_XML_WARNING("Missing summon.name");
				}
			}
		}
		else if(!xmlStrcmp(p->name, (const xmlChar*)"script"))
		{
			for(xmlNodePtr tmpNode = p->children; tmpNode; tmpNode = tmpNode->next)
			{
				if(!xmlStrcmp(tmpNode->name, (const xmlChar*)"event"))
				{
					if(readXMLString(tmpNode, "name", strValue))
						mType->scriptList.push_back(strValue);
					else
						SHOW_XML_WARNING("Missing name for script event");
				}
			}
		}
		else
			SHOW_XML_WARNING("Unknown attribute type - " << p->name);
	}

	xmlFreeDoc(doc);
	if(!monsterLoad)
	{
		if(new_mType)
			delete mType;

		return false;
	}

	static uint32_t id = 0;
	if(new_mType)
	{
		monsterNames[asLowerCaseString(monsterName)] = ++id;
		monsters[id] = mType;
	}

	return true;
}

bool Monsters::loadLoot(xmlNodePtr node, LootBlock& lootBlock)
{
	std::string strValue;
	if(readXMLString(node, "id", strValue) || readXMLString(node, "ids", strValue))
	{
		IntegerVec idsVec;
		parseIntegerVec(strValue, idsVec);
		for(IntegerVec::iterator it = idsVec.begin(); it != idsVec.end(); ++it)
		{
			lootBlock.ids.push_back(*it);
			if(Item::items[(*it)].isContainer())
				loadChildLoot(node, lootBlock);
		}
	}
	else if(readXMLString(node, "name", strValue) || readXMLString(node, "names", strValue))
	{
		StringVec names = explodeString(strValue, ";");
		for(StringVec::iterator it = names.begin(); it != names.end(); ++it)
		{
			uint16_t tmp = Item::items.getItemIdByName(strValue);
			if(!tmp)
				continue;

			lootBlock.ids.push_back(tmp);
			if(Item::items[tmp].isContainer())
				loadChildLoot(node, lootBlock);
		}
	}

	if(lootBlock.ids.empty())
		return false;

	int32_t intValue;
	if(readXMLInteger(node, "count", intValue) || readXMLInteger(node, "countmax", intValue))
		lootBlock.count = std::max((int32_t)1, intValue);
	else
		lootBlock.count = 1;

	if(readXMLInteger(node, "chance", intValue) || readXMLInteger(node, "chance1", intValue))
		lootBlock.chance = std::min(MAX_LOOTCHANCE, std::abs(intValue));
	else
		lootBlock.chance = MAX_LOOTCHANCE;

	if(readXMLInteger(node, "subtype", intValue) || readXMLInteger(node, "subType", intValue))
		lootBlock.subType = intValue;
	
	if(readXMLString(node, "raridade", strValue))
		lootBlock.raridade = strValue;
		
	if(readXMLInteger(node, "ninjutsu", intValue) || readXMLInteger(node, "ninjutsu", intValue)
		|| readXMLInteger(node, "uid", intValue))
		lootBlock.ninjutsu = intValue;

	if(readXMLInteger(node, "taijutsu", intValue) || readXMLInteger(node, "taijutsu", intValue)
		|| readXMLInteger(node, "uid", intValue))
		lootBlock.taijutsu = intValue;
	
	if(readXMLInteger(node, "actionId", intValue) || readXMLInteger(node, "actionid", intValue)
		|| readXMLInteger(node, "aid", intValue))
		lootBlock.actionId = intValue;

	if(readXMLInteger(node, "uniqueId", intValue) || readXMLInteger(node, "uniqueid", intValue)
		|| readXMLInteger(node, "uid", intValue))
		lootBlock.uniqueId = intValue;

	if(readXMLString(node, "text", strValue))
		lootBlock.text = strValue;

	return true;
}

bool Monsters::loadChildLoot(xmlNodePtr node, LootBlock& parentBlock)
{
	if(!node)
		return false;

	for(xmlNodePtr p = node->children; p; p = p->next)
	{
		if(xmlStrcmp(p->name, (const xmlChar*)"inside"))
		{
			LootBlock childBlock;
			if(loadLoot(p, childBlock))
				parentBlock.childLoot.push_back(childBlock);

			continue;
		}

		for(xmlNodePtr insideNode = p->children; insideNode; insideNode = insideNode->next)
		{
			LootBlock childBlock;
			if(loadLoot(insideNode, childBlock))
				parentBlock.childLoot.push_back(childBlock);
		}
	}

	return true;
}

MonsterType* Monsters::getMonsterType(const std::string& name)
{
	uint32_t mid = getIdByName(name);
	if(mid != 0)
		return getMonsterType(mid);

	return NULL;
}

MonsterType* Monsters::getMonsterType(uint32_t mid)
{
	MonsterMap::iterator it = monsters.find(mid);
	if(it != monsters.end())
		return it->second;

	return NULL;
}

uint32_t Monsters::getIdByName(const std::string& name)
{
	std::string tmp = name;
	MonsterNameMap::iterator it = monsterNames.find(asLowerCaseString(tmp));
	if(it != monsterNames.end())
		return it->second;

	return 0;
}

Monsters::~Monsters()
{
	loaded = false;
	for(MonsterMap::iterator it = monsters.begin(); it != monsters.end(); ++it)
		delete it->second;

	monsters.clear();
}
