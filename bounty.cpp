#include "otpch.h"
#include "bounty.h"
#include "tools.h"
#include "database.h"
#include "player.h" // Inclua o cabeçalho completo de Player
#include "const.h"  // Inclua o cabeçalho que define os tipos de mensagem

#define MESSAGE_STATUS_CONSOLE_BLUE 10 // Substitua 10 pelo valor correto
#define MESSAGE_INFO_DESCR 19  // Substitua pelo valor correto do seu servidor

extern Database g_database;

// Remova a declaração da variável global g_bountySystem
// BountySystem g_bountySystem; // Isso está incorreto para um Singleton

void BountySystem::addBounty(const std::string& issuer, const std::string& target, uint32_t value, uint8_t type)
{
    Bounty bounty;
    bounty.id = nextBountyId++;
    bounty.issuerName = issuer;
    bounty.targetName = target;
    bounty.value = value;
    bounty.type = type;
    bounty.timestamp = time(nullptr);

    bounties.push_back(bounty);
}

void BountySystem::acceptBounty(Player* player, uint32_t bountyId)
{
    for (auto& bounty : bounties) {
        if (bounty.id == bountyId) {
            break;
        }
    }
}

std::string BountySystem::getBountyListForPlayer(Player* player)
{
    std::string bountyList;
    for (const auto& bounty : bounties) {
        bountyList += std::to_string(bounty.id) + "," + bounty.targetName + "," + std::to_string(bounty.value) + "," + std::to_string(bounty.type) + ";";
    }
    return bountyList;
}

void BountySystem::loadBounties()
{
    DBResult_ptr result = g_database.storeQuery("SELECT * FROM bounties");
    if (result) {
        do {
            Bounty bounty;
            bounty.id = result->getNumber<uint32_t>("id");
            bounty.issuerName = result->getString("issuer_name");
            bounty.targetName = result->getString("target_name");
            bounty.value = result->getNumber<uint32_t>("value");
            bounty.type = result->getNumber<uint8_t>("type");
            bounty.timestamp = result->getNumber<time_t>("timestamp");
            bounties.push_back(bounty);
        } while (result->next());
    }
}

void BountySystem::saveBounty(const Bounty& bounty)
{
    std::stringstream query;
    query << "INSERT INTO bounties (issuer_name, target_name, value, type) VALUES ("
          << g_database.escapeString(bounty.issuerName) << ", "
          << g_database.escapeString(bounty.targetName) << ", "
          << bounty.value << ", "
          << static_cast<int>(bounty.type) << ")";
    g_database.executeQuery(query.str());
}