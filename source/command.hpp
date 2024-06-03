#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>
#include <array>

namespace Command
{

constexpr const char *_Selector[] = {
    "@e", "@a", "@s", "@p", "@r"
};

enum Selector : char
{
    // @e
    All,
    // @a
    Player,
    // @s
    Own,
    // @p
    Nearest,
    // @r
    Rand
};

constexpr const char *_PosMode[] = {
    "", "~", "^"
};

enum class PosMode : char
{
    // Take the center of the world as the origin.
    Absolute,
    // ~ Take the entity position as the origin.
    Relative,
    // ^ Take the entity position as the origin, but the axis affected by entity sight, and the front sight is positive Z axis.
    Locatlity
};

constexpr const char *_SetblockMode[] = {
    "destroy", "keep", "replace"
};

enum class SetblockMode : char
{
    // The replaced blocks will drop like it be destory by pickaxe.
    Destroy,
    // Keep original when target area exist block unless air block.
    Keep,
    // Replace the original block.
    Replace
};

constexpr const char *_FillMode[] = {
    "destroy", "hollow", "keep", "outline", "replace"
};

enum class FillMode : char
{
    // The replaced blocks will drop like it be destory by pickaxe.
    Destroy,
    // Replace outline blocks and replace inline blocks with air block.
    Hollow,
    // Keep original when target area exist block unless air block.
    Keep,
    // Only replace outline blocks.
    Outline,
    // Replace all blocks (include air block), and able to specify a block for only replace it.
    Replace
};

}

namespace Command
{

inline std::string setblock(const std::string &blockId,
                            const std::array<int, 3> &pos,
                            PosMode posMode = PosMode::Absolute,
                            SetblockMode mode = SetblockMode::Replace,
                            bool hasSlash = false)
{
    std::string command;
    if (hasSlash)
        command = '/';
    command = command + "setblock" + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(pos[0]) + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(pos[1]) + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(pos[2]) + ' ' +
        blockId + ' ' + _SetblockMode[static_cast<int>(mode)];
    return command;
}

inline std::string fill(const std::string &blockId,
                        const std::array<int, 3> &posFrom,
                        const std::array<int, 3> &posTo,
                        PosMode posMode = PosMode::Absolute,
                        FillMode mode = FillMode::Replace,
                        const std::string &replacedBlockId = std::string(),
                        bool hasSlash = false)
{
    std::string command;
    if (hasSlash)
        command = '/';
    command = command + "fill" + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(posFrom[0]) + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(posFrom[1]) + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(posFrom[2]) + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(posTo[0]) + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(posTo[1]) + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(posTo[2]) + ' ' +
        blockId + ' ' + _FillMode[static_cast<int>(mode)];
    if (replacedBlockId.empty() || mode != FillMode::Replace)
        return command;
    return command + ' ' + replacedBlockId;
}

inline std::string particle(const std::string &particleId,
                            const std::array<int, 3> &pos,
                            PosMode posMode = PosMode::Absolute,
                            bool hasSlash = false)
{
    std::string command;
    if (hasSlash)
        command = '/';
    command = command + "particle" + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(pos[0]) + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(pos[1]) + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(pos[2]) + ' ' +
        particleId;
    return command;
}

inline std::string oldExecute(const std::string &targetEntityId,
                              const std::array<int, 3> &pos,
                              const std::string &subCommand,
                              PosMode posMode = PosMode::Absolute,
                              bool hasSlash = false)
{
    std::string command;
    if (hasSlash)
        command = '/';
    command = command + "execute" + ' ' +
        targetEntityId + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(pos[0]) + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(pos[1]) + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(pos[2]) + ' ' +
        subCommand;
    return command;
}

inline std::string oldExecute(const std::string &targetEntityId,
                              const std::array<int, 3> &executePos,
                              const std::array<int, 3> &detectPos,
                              const std::string &blockId, int data,
                              const std::string &subCommand,
                              PosMode executePosMode = PosMode::Absolute,
                              PosMode detectPosMode = PosMode::Absolute,
                              bool hasSlash = false)
{
    std::string command;
    if (hasSlash)
        command = '/';
    command = command + "execute" + ' ' +
        targetEntityId + ' ' +
        _PosMode[static_cast<int>(executePosMode)] + std::to_string(executePos[0]) + ' ' +
        _PosMode[static_cast<int>(executePosMode)] + std::to_string(executePos[1]) + ' ' +
        _PosMode[static_cast<int>(executePosMode)] + std::to_string(executePos[2]) + ' ' +
        "detect" + ' ' +
        _PosMode[static_cast<int>(detectPosMode)] + std::to_string(detectPos[0]) + ' ' +
        _PosMode[static_cast<int>(detectPosMode)] + std::to_string(detectPos[1]) + ' ' +
        _PosMode[static_cast<int>(detectPosMode)] + std::to_string(detectPos[2]) + ' ' +
        blockId + ' ' + std::to_string(data) + ' ' +
        subCommand;
    return command;
}

inline std::string execute(const std::string &decorate, const std::string &condition,
                           const std::string &subCommand,
                           bool hasSlash = false)
{
    std::string command;
    if (hasSlash)
        command = '/';
    command = command + "execute" + ' ' +
        decorate + ' ' +
        condition + ' ' +
        "run" + ' ' +
        subCommand;
    return command;
}

inline std::string execute(Selector as,
                           const std::string &subCommand,
                           bool hasSlash = false)
{
    std::string decorate;
    decorate = decorate + "as" + ' ' +
        _Selector[static_cast<int>(as)];
    std::string condition;
    return execute(decorate, condition, subCommand, hasSlash);
}

inline std::string execute(Selector as, Selector at,
                           const std::string &subCommand,
                           bool hasSlash = false)
{
    std::string decorate;
    decorate = decorate + "as" + ' ' +
        _Selector[static_cast<int>(as)] + ' ' +
        "at" + ' ' +
        _Selector[static_cast<int>(at)];
    std::string condition;
    return execute(decorate, condition, subCommand, hasSlash);
}

inline std::string execute(Selector as, Selector at,
                           const std::array<int, 3> &pos,
                           const std::string &subCommand,
                           PosMode posMode = PosMode::Absolute,
                           bool hasSlash = false)
{
    std::string decorate;
    decorate = decorate + "as" + ' ' +
        _Selector[static_cast<int>(as)] + ' ' +
        "at" + ' ' +
        _Selector[static_cast<int>(at)] + ' ' +
        "positioned" + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(pos[0]) + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(pos[1]) + ' ' +
        _PosMode[static_cast<int>(posMode)] + std::to_string(pos[2]);
    std::string condition;
    return execute(decorate, condition, subCommand, hasSlash);
}

}

#endif // !COMMAND_HPP
