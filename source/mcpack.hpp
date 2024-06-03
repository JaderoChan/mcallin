#ifndef MCPACK_HPP
#define MCPACK_HPP

#include <array>
#include <string>
#include <sstream>
#include <random>

#include <betterfiles.hpp>

namespace Mcpack
{

// The Base64 data of pack icon.
constexpr const char * _PACKICON_BASE64 = "iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAYAAACqaXHeAAAABGdBTUEAALGPC/xhBQAAAgRJREFUeNrtmrlKQ0EUhvM0goWlC8G4LxgtFA0uKMYoxuWKilGwUUQkIAhBwTSCC+IuxhUCYtDCxoe6tvc/QobDJI3zF19zZ3LmzDdwmCWh1EvUD7L21gOsPHcB0xf1RVl8aAfGj6sB764F2PgYAOT4czdNQPIyAsxeNwITp7VAb6YCWLhvBUIUQAGOC1h97faDyAR38yPAVq4P2H7qB4YjlcDMVQOQOKsDpKCDr0kgU4gDckHkhGX+Utj8bTNAARTguoC99zE/iJzA0Y8HTJ2HAVmE5O+Xch2AbJdFeLMQA/Y/E0D2OwnIeIPZKiB+UgPI/hRAAa4LkB88zwNku2T5sRMw9ZfIIijb5cZFtq/HwoBpPNmfAijAdQGHXtQPIjvI9vzOKCAHMMXTJmxaIFO+FEABFFBcgCkhGVArTDtBrTBTf4lcQAqgANcFyA/agOl0GtAmYBpf21+7QBRAAa4LsE1YWyS1RbPU8SmAAigABdgG1LabNjK2E9aORwEU4LoA2wsF20tJW2w3ShRAARSgK2K2hxmTYFN87UbJdFijAApwXUCpDxfaIiQfOrQbIe3DzJ8/SFAABTguwDagrZByYyqqFEABFKDbCElSQ22A7QWHaWNkOlxpL20pgAJcF2CbkPYSUnv4sV0gCqAACrATYPsYarqQKPfjp/VpkAIo4H8L+AVfB6pPaEnxqAAAAABJRU5ErkJggg==";

struct PackManifest
{
    enum Type : char
    {
        Data,
        Resource
    };

    PackManifest() {}
    PackManifest(const std::string &packName, const std::string &packDescription,
                 const std::array<int, 3> &packVersion, const std::string &packPrefix = "",
                 Type packType = Data, int formatVersion = 2,
                 const std::array<int, 3> &minVersion = {1, 19, 70}) :
        name(packName), description(packDescription), packVersion(packVersion),
        prefix(packPrefix.empty() ? packName : packPrefix), type(packType),
        formatVersion(formatVersion), minVersion(minVersion) {}

    std::string getTypeString() const {
        switch (type) {
            case Data:
                return std::string("data");
            case Resource:
                return std::string("resources");
            default:
                return std::string();
        }
    }

    std::string name;
    std::string description;
    std::string prefix;
    std::array<int, 3> packVersion = { 0, 0, 0 };
    std::array<int, 3> minVersion = { 1, 19, 70 };
    Type type = Data;
    int formatVersion = 2;
};

std::string getManifestJson(const PackManifest &manifest);

Bf::Dir getMcpackFrame(const PackManifest &manifest);

}

#endif // !MCPACK_HPP
