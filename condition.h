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

#ifndef __CONDITION__
#define __CONDITION__
#include "otsystem.h"

#include "const.h"
#include "enums.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "fileloader.h"

class Creature;
class Player;
class PropStream;

enum ConditionType_t
{
	CONDITION_NONE = 0,
	CONDITION_POISON,
	CONDITION_FIRE ,
	CONDITION_ENERGY,
	CONDITION_BLEEDING,
	CONDITION_HASTE,
	CONDITION_PARALYZE,
	CONDITION_OUTFIT,
	CONDITION_INVISIBLE,
	CONDITION_LIGHT,
	CONDITION_MANASHIELD,
	CONDITION_INFIGHT,
	CONDITION_DRUNK,
	CONDITION_EXHAUST,
	CONDITION_REGENERATION,
	CONDITION_SOUL,
	CONDITION_DROWN,
	CONDITION_MUTED,
	CONDITION_ATTRIBUTES,
	CONDITION_FREEZING,
	CONDITION_DAZZLED,
	CONDITION_CURSED,
	CONDITION_PACIFIED,
	CONDITION_GAMEMASTER,
	CONDITION_HUNTING,
	CONDITION_SPELLCOOLDOWN,
	CONDITION_LOGINPROTECTION,
	CONDITION_SUITON,
	CONDITION_FUUTON,
	CONDITION_DOTON,
	CONDITION_RAITON,
	CONDITION_MOKUTON,
	CONDITION_SHOTON,
	CONDITION_SUNA,
	CONDITION_ENTON,
	CONDITION_HYOTON,
	CONDITION_YOTON,
	CONDITION_RANTON,
	CONDITION_BAKUTON,
	CONDITION_SHAKUTON,
	CONDITION_FUTTON,
	CONDITION_JINTON,
	CONDITION_SATETSU,
	CONDITION_TAIJUTSU,
	CONDITION_YIN,
	CONDITION_YANG,
	CONDITION_KENJUTSU
};

enum ConditionEnd_t
{
	CONDITIONEND_CLEANUP,
	CONDITIONEND_DEATH,
	CONDITIONEND_TICKS,
	CONDITIONEND_ABORT
};

enum ConditionAttr_t
{
	CONDITIONATTR_TYPE = 1,
	CONDITIONATTR_ID = 2,
	CONDITIONATTR_TICKS = 3,
	CONDITIONATTR_HEALTHTICKS = 4,
	CONDITIONATTR_HEALTHGAIN = 5,
	CONDITIONATTR_MANATICKS = 6,
	CONDITIONATTR_MANAGAIN = 7,
	CONDITIONATTR_DELAYED = 8,
	CONDITIONATTR_OWNER = 9,
	CONDITIONATTR_INTERVALDATA = 10,
	CONDITIONATTR_SPEEDDELTA = 11,
	CONDITIONATTR_FORMULA_MINA = 12,
	CONDITIONATTR_FORMULA_MINB = 13,
	CONDITIONATTR_FORMULA_MAXA = 14,
	CONDITIONATTR_FORMULA_MAXB = 15,
	CONDITIONATTR_LIGHTCOLOR = 16,
	CONDITIONATTR_LIGHTLEVEL = 17,
	CONDITIONATTR_LIGHTTICKS = 18,
	CONDITIONATTR_LIGHTINTERVAL = 19,
	CONDITIONATTR_SOULTICKS = 20,
	CONDITIONATTR_SOULGAIN = 21,
	CONDITIONATTR_SKILLS = 22,
	CONDITIONATTR_STATS = 23,
	CONDITIONATTR_OUTFIT = 24,
	CONDITIONATTR_PERIODDAMAGE = 25,
	CONDITIONATTR_BUFF = 26,
	CONDITIONATTR_SUBID = 27,

	//reserved for serialization
	CONDITIONATTR_END = 254
};

struct IntervalInfo
{
	int32_t timeLeft, value, interval;
};

class Condition
{
	public:
		Condition(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, bool _buff, uint32_t _subId);
		virtual ~Condition() {}

		virtual bool startCondition(Creature* creature);
		virtual bool executeCondition(Creature* creature, int32_t interval);
		virtual void endCondition(Creature*, ConditionEnd_t) {}
		virtual void addCondition(Creature*, const Condition*) {}

		virtual Icons_t getIcons() const;
		ConditionId_t getId() const {return id;}
		uint32_t getSubId() const {return subId;}

		virtual Condition* clone() const {return NULL;}

		ConditionType_t getType() const {return conditionType;}
		int64_t getEndTime() const {return ticks == -1 ? 0 : endTime;}
		int32_t getTicks() const {return ticks;}
		void setTicks(int32_t newTicks);

		static Condition* createCondition(ConditionId_t _id, ConditionType_t _type, int32_t ticks, int32_t param = 0, bool _buff = false, uint32_t _subId = 0);
		static Condition* createCondition(PropStream& propStream);

		virtual bool setParam(ConditionParam_t param, int32_t value);

		//serialization
		bool unserialize(PropStream& propStream);
		virtual bool serialize(PropWriteStream& propWriteStream);
		virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

		bool isPersistent() const {return (ticks > 0 && (id == CONDITIONID_DEFAULT || id != CONDITIONID_COMBAT));}

	protected:
		virtual bool updateCondition(const Condition* addCondition);

		ConditionId_t id;
		uint32_t subId;

		int32_t ticks;
		int64_t endTime;

		ConditionType_t conditionType;
		bool buff;
};

class ConditionGeneric: public Condition
{
	public:
		ConditionGeneric(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, bool _buff, uint32_t _subId);
		virtual ~ConditionGeneric() {}

		virtual void addCondition(Creature* creature, const Condition* condition);

		virtual Icons_t getIcons() const;

		virtual ConditionGeneric* clone() const {return new ConditionGeneric(*this);}
};

class ConditionManaShield : public ConditionGeneric
{
	public:
		ConditionManaShield(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, bool _buff, uint32_t _subId);
		virtual ~ConditionManaShield() {}

		virtual Icons_t getIcons() const;

		virtual ConditionManaShield* clone() const {return new ConditionManaShield(*this);}
};

class ConditionAttributes : public ConditionGeneric
{
	public:
		ConditionAttributes(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, bool _buff, uint32_t _subId);
		virtual ~ConditionAttributes() {}

		virtual bool startCondition(Creature* creature);
		virtual bool executeCondition(Creature* creature, int32_t interval);
		virtual void endCondition(Creature* creature, ConditionEnd_t reason);
		virtual void addCondition(Creature* creature, const Condition* condition);

		virtual ConditionAttributes* clone() const {return new ConditionAttributes(*this);}

		virtual bool setParam(ConditionParam_t param, int32_t value);

		//serialization
		virtual bool serialize(PropWriteStream& propWriteStream);
		virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

	protected:
		int32_t skills[SKILL_LAST + 1], stats[STAT_LAST + 1], skillsPercent[SKILL_LAST + 1],
			statsPercent[STAT_LAST + 1], currentSkill, currentStat;

		void updatePercentSkills(Player* player);
		void updatePercentStats(Player* player);
		void updateSkills(Player* player);
		void updateStats(Player* player);
};

class ConditionRegeneration : public ConditionGeneric
{
	public:
		ConditionRegeneration(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, bool _buff, uint32_t _subId);
		virtual ~ConditionRegeneration() {}

		virtual void addCondition(Creature* creature, const Condition* addCondition);
		virtual bool executeCondition(Creature* creature, int32_t interval);

		virtual ConditionRegeneration* clone() const {return new ConditionRegeneration(*this);}

		virtual bool setParam(ConditionParam_t param, int32_t value);

		//serialization
		virtual bool serialize(PropWriteStream& propWriteStream);
		virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

	protected:
		uint32_t internalHealthTicks, internalManaTicks, healthTicks, manaTicks, healthGain, manaGain;
};

class ConditionSoul : public ConditionGeneric
{
	public:
		ConditionSoul(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, bool _buff, uint32_t _subId);
		virtual ~ConditionSoul() {}

		virtual void addCondition(Creature* creature, const Condition* addCondition);
		virtual bool executeCondition(Creature* creature, int32_t interval);

		virtual ConditionSoul* clone() const {return new ConditionSoul(*this);}

		virtual bool setParam(ConditionParam_t param, int32_t value);

		//serialization
		virtual bool serialize(PropWriteStream& propWriteStream);
		virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

	protected:
		uint32_t internalSoulTicks, soulTicks, soulGain;
};

class ConditionDamage: public Condition
{
	public:
		ConditionDamage(ConditionId_t _id, ConditionType_t _type, bool _buff, uint32_t _subId);
		virtual ~ConditionDamage() {}

		static void generateDamageList(int32_t amount, int32_t start, std::list<int32_t>& list);

		virtual bool startCondition(Creature* creature);
		virtual bool executeCondition(Creature* creature, int32_t interval);
		virtual void addCondition(Creature* creature, const Condition* condition);

		virtual Icons_t getIcons() const;

		virtual ConditionDamage* clone() const {return new ConditionDamage(*this);}

		virtual bool setParam(ConditionParam_t param, int32_t value);

		int32_t getTotalDamage() const;
		bool doForceUpdate() const {return forceUpdate;}
		bool addDamage(int32_t rounds, int32_t time, int32_t value);

		//serialization
		virtual bool serialize(PropWriteStream& propWriteStream);
		virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

	protected:
		bool updateCondition(const ConditionDamage* addCondition);

		bool init();
		bool getNextDamage(int32_t& damage);
		bool doDamage(Creature* creature, int32_t damage);

		bool delayed, forceUpdate, field;
		int32_t maxDamage, minDamage, startDamage, periodDamage, periodDamageTick, tickInterval;
		uint32_t owner;

		typedef std::list<IntervalInfo> DamageList;
		DamageList damageList;
};

class ConditionOutfit: public Condition
{
	public:
		ConditionOutfit(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, bool _buff, uint32_t _subId);
		virtual ~ConditionOutfit() {}

		virtual bool startCondition(Creature* creature);
		virtual void endCondition(Creature* creature, ConditionEnd_t reason);
		virtual void addCondition(Creature* creature, const Condition* condition);

		virtual ConditionOutfit* clone() const {return new ConditionOutfit(*this);}

		void addOutfit(Outfit_t _outfit) {outfits.push_back(_outfit);}
		void setOutfits(std::vector<Outfit_t> _outfits) {outfits = _outfits;}

		//serialization
		virtual bool serialize(PropWriteStream& propWriteStream);
		virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

	protected:
		void changeOutfit(Creature* creature, int32_t index = -1);
		std::vector<Outfit_t> outfits;
};

class ConditionSpeed: public ConditionOutfit
{
	public:
		ConditionSpeed(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, bool _buff, uint32_t _subId, int32_t changeSpeed);
		virtual ~ConditionSpeed() {}

		virtual bool startCondition(Creature* creature);
		virtual void endCondition(Creature* creature, ConditionEnd_t reason);
		virtual void addCondition(Creature* creature, const Condition* condition);

		virtual Icons_t getIcons() const;

		virtual ConditionSpeed* clone() const {return new ConditionSpeed(*this);}

		virtual bool setParam(ConditionParam_t param, int32_t value);

		void setFormulaVars(float _mina, float _minb, float _maxa, float _maxb);

		//serialization
		virtual bool serialize(PropWriteStream& propWriteStream);
		virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

	protected:
		void getFormulaValues(int32_t var, int32_t& min, int32_t& max) const;

		int32_t speedDelta;
		float mina, minb, maxa, maxb;
};

class ConditionLight: public Condition
{
	public:
		ConditionLight(ConditionId_t _id, ConditionType_t _type, int32_t _ticks, bool _buff, uint32_t _subId, int32_t lightLevel, int32_t lightColor);
		virtual ~ConditionLight() {}

		virtual bool startCondition(Creature* creature);
		virtual bool executeCondition(Creature* creature, int32_t interval);
		virtual void endCondition(Creature* creature, ConditionEnd_t reason);
		virtual void addCondition(Creature* creature, const Condition* addCondition);

		virtual ConditionLight* clone() const {return new ConditionLight(*this);}

		virtual bool setParam(ConditionParam_t param, int32_t value);

		//serialization
		virtual bool serialize(PropWriteStream& propWriteStream);
		virtual bool unserializeProp(ConditionAttr_t attr, PropStream& propStream);

	protected:
		LightInfo lightInfo;
		uint32_t internalLightTicks, lightChangeInterval;
};
#endif
