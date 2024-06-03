#ifndef PREPROCESS_HPP
#define PREPROCESS_HPP

#include <string>
#include <unordered_map>

#ifndef PREPROCESS_MACRO
#define PREPROCESS_MACRO

#define PREPROC_KW_ROOT "blocks"
#define PREPROC_KW_IdS "ids"
#define PREPROC_KW_Id "id"
#define PREPROC_KW_VERS "version"
#define PREPROC_KW_TEXS "texture"
#define PREPROC_KW_COLORS "rgbColor"
#define PREPROC_KW_DEBUTVERS "debutVersion"
#define PREPROC_KW_ALIGNMENT "direction"

#define PREPROC_KW_FRONT "front"
#define PREPROC_KW_BACK "back"
#define PREPROC_KW_RIGHT "right"
#define PREPROC_KW_LEFT "left"
#define PREPROC_KW_TOP "top"
#define PREPROC_KW_BOTTOM "bottom"
#define PREPROC_KW_SIDE "side"

#define PREPROC_KW_X "x"
#define PREPROC_KW_Y "y"

#define PREPROC_KW_ISLIGHTING "isLighting"
#define PREPROC_KW_ISTIMEVRAYING "isTimeVarying"
#define PREPROC_KW_BURNABLE "burnable"
#define PREPROC_KW_PICKABLE "endermanPickable"
#define PREPROC_KW_HASGRAVITY "hasGravity"
#define PREPROC_KW_HASENERGY "hasEnergy"
#define PREPROC_KW_ISTRANSPARENCY "isTransparency"
#define PREPROC_KW_ISCOMMANDFORMATId "isCommandFormatId"

#endif // !PREPROCESS_MACRO

template <typename T>
inline T square(T value) {
    return value * value;
}

struct Version
{
    Version() {}
    Version(unsigned char major, unsigned char minor, unsigned char patch) :
        major(major), minor(minor), patch(patch) {}

    int data() const {
        return (major << 24) ^ (minor << 16) ^ patch;
    }
    bool operator==(const Version &rhs) const {
        return data() == rhs.data();
    }
    bool operator>(const Version &rhs) const {
        return data() > rhs.data();
    }
    bool operator<(const Version &rhs) const {
        return data() < rhs.data();
    }
    bool operator>=(const Version &rhs) const {
        return *this > rhs || *this == rhs;
    }
    bool operator<=(const Version &rhs) const {
        return *this < rhs || *this == rhs;
    }

    unsigned char major = 0;
    unsigned char minor = 0;
    unsigned char patch = 0;
};

struct Rgb
{
    Rgb() {}
    Rgb(unsigned char r, unsigned char g, unsigned char b) :
        r(r), g(g), b(b) {}

    bool operator==(const Rgb &rhs) {
        return r == rhs.r && g == rhs.g && b == rhs.b;
    }

    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
};

namespace std
{

template<>
struct hash<Rgb>
{
    size_t operator()(const Rgb &rgb) const {
        size_t h1 = std::hash<unsigned char>()(rgb.r);
        size_t h2 = std::hash<unsigned char>()(rgb.g);
        size_t h3 = std::hash<unsigned char>()(rgb.b);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

template<>
struct hash<Version>
{
    size_t operator()(const Version &version) const {
        return version.data();
    }
};

}

namespace BlockFlag
{

// The usable faces of a block.
enum Faces : int
{
    Front = 0x01,
    Back = 0x02,
    Right = 0x04,
    Left = 0x08,
    Top = 0x10,
    Bottom = 0x20,
    Side = 0x1F
};

// The usable alignment of a block.
enum Alignment : int
{
    // The block can be placed along the XZ plane.
    Horizontal = 1,
    // The block can be placed along the XY or ZY plane.
    Vertical
};

// The usable attribute of a block.
enum Attribute : int
{
    // The block can give out light.
    IsLighting = 0x01,
    // The block will change over time.
    IsTimeVarying = 0x02,
    // The block can be burn.
    Burnable = 0x04,
    // The block can be picked up by endman.
    EndermanPickable = 0x08,
    // The block has gravity.
    HasGravity = 0x10,
    // The block has redstone energy.
    HasEnergy = 0x20,
    // The block has transparency.
    IsTransparency = 0x40,
    // The block id is can be use in command.
    IsCommandFormatId = 0x80
};

}

struct BlockInfoRaw
{
    BlockInfoRaw() {}

    std::unordered_map<Version, std::string> ids;
    std::unordered_map<BlockFlag::Faces, std::string> facesTexturePath;
    std::unordered_map<BlockFlag::Faces, Rgb> facesColor;
    Version debutVersion;
    char alignment = 0;
    int attribute = 0;
};
using BIRaws = std::vector<BlockInfoRaw>;

struct BlockInfoModified
{
    BlockInfoModified() {}
    BlockInfoModified(std::string blockId, std::string textureName, Rgb color) :
        blockId(blockId), textureName(textureName), color(color) {}
    std::string blockId;
    std::string textureName;
    Rgb color;
};
using BIModis = std::vector<BlockInfoModified>;

BIRaws getBIRawsByDomFile(const std::string &filepath) noexcept;

BIModis rawsToModis(const BIRaws &raws, int face, int alignment, int attribute,
                    Version debutVersion);

bool textureAndblockIdHandle(const std::string &textureDirPath, const std::string &blockInfosFilePath);

#endif // !PREPROCESS_HPP
