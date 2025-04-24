#ifndef __BOUNTY_H__
#define __BOUNTY_H__

#include <vector>
#include <string>
#include <ctime>

// Declaração avançada para evitar incluir o cabeçalho completo de Player
class Player;

struct Bounty {
    uint32_t id;
    std::string targetName;
    std::string issuerName;
    uint32_t value;
    uint8_t type;
    time_t timestamp;
};

class BountySystem {
public:
    // Método estático para obter a instância única (Singleton)
    static BountySystem& getInstance() {
        static BountySystem instance;
        return instance;
    }

    // Métodos públicos
    void addBounty(const std::string& issuer, const std::string& target, uint32_t value, uint8_t type);
    void acceptBounty(Player* player, uint32_t bountyId);
    std::string getBountyListForPlayer(Player* player);
    void loadBounties();
    void saveBounty(const Bounty& bounty);

private:
    // Construtor privado para garantir que a classe seja Singleton
    BountySystem() = default; // Usando 'default' para definir o construtor

    // Lista de recompensas
    std::vector<Bounty> bounties;

    // Próximo ID de recompensa
    uint32_t nextBountyId = 1;
};

// Declaração da variável global g_bountySystem
extern BountySystem g_bountySystem;

#endif