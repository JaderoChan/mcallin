#include "modules.hpp"

#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include <opencv2/opencv.hpp>
#include <opencv2/video.hpp>
#include <opencv2/imgproc.hpp>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <nbt.hpp>

#include "datacarrier.hpp"
#include "command.hpp"
#include "file_processing.hpp"

#undef GetObject

static inline double rgbDistance(const Rgb &a, const Rgb &b) {
    return  std::sqrt(square(a.r - b.r) + square(a.g - b.g) + square(a.b - b.b));
}

static inline double rgbSimilarity(const Rgb &a, const Rgb &b, int type) {
    if (type == 0)
        return 1. - (square<double>(a.r - b.r) * 0.32 + square<double>(a.g - b.g) * 0.52 +
                     square<double>(a.b - b.b) * 0.16) / 65025;
    if (type == 1)
        return 1. - (square<double>(a.r - b.r) * 0.299 + square<double>(a.g - b.g) * 0.587 +
                     square<double>(a.b - b.b) * 0.114) / 65025;
    return 0;
}

static inline Rgb bgrToRgb(const cv::Vec3b &cvBgr) {
    return Rgb(cvBgr[2], cvBgr[1], cvBgr[0]);
}

static inline cv::Vec3b rgbToBgr(const Rgb &rgb) {
    return cv::Vec3b(rgb.b, rgb.g, rgb.r);
}

// @brief If the image size greater than specify size zoom out the image by specify interpolation algorithm, else do nothing.
// @note Does not change the aspect ratio of the image.
static void limitScale(cv::Mat &image, int maxWidth, int maxHeight) {
    if (image.empty()) {
        std::cerr << "In line " << __LINE__ << ", " << __FUNCTION__ << " " << "The image is invalid." << std::endl;
        return;
    }
    int width = image.cols;
    int height = image.rows;
    double ratio;
    if (maxWidth == 0 || maxHeight == 0)
        return;
    if (maxWidth == -1 && maxHeight != -1) {
        if (height <= maxHeight)
            return;
        ratio = static_cast<double>(maxHeight) / height;
    } else if (maxWidth != -1 && maxHeight == -1) {
        if (width <= maxWidth)
            return;
        ratio = static_cast<double>(maxWidth) / width;
    } else {
        if (width <= maxWidth && height <= maxHeight)
            return;
        ratio = maxWidth / double(width) < maxHeight / double(height) ?
            maxWidth / double(width) : maxHeight / double(height);
    }
    cv::resize(image, image, cv::Size(int(width * ratio), int(height * ratio)), 0.0, 0.0, cv::INTER_AREA);
}

static std::string domToStr(const rapidjson::Document &dom) {
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    dom.Accept(writer);
    return buffer.GetString();
}

static BlockInfoModified *rgbNearest(const Rgb &rgb, BIModis &modis, int type = 0) {
    assert(!modis.empty());

    if (modis.empty())
        return nullptr;
    BlockInfoModified *result = nullptr;
    double similarity = 0;
    for (auto &var : modis) {
        double sim = rgbSimilarity(rgb, var.color, type);
        if (similarity > sim)
            continue;
        similarity = sim;
        result = &var;
    }
    return result;
}

static rapidjson::Document getDom(std::ifstream &dataFile) {
    if (!dataFile.is_open())
        return rapidjson::Document();
    std::string json;
    std::string line;
    while (dataFile >> line)
        json += line;
    rapidjson::Document dom;
    dom.Parse(json.c_str());
    return dom;
}

static BlockCube getBlocks(cv::Mat &img, BIModis &modis, int maxWidth, int maxHeight,
                           std::unordered_map<std::string, int> *blocksInfo = nullptr)
{
    limitScale(img, maxWidth, maxHeight);
    cv::flip(img, img, 1);
    BlockCube result(img.cols, img.rows, 1);
    for (int row = 0; row < img.rows; ++row) {
        for (int col = 0; col < img.cols; ++col) {
            Rgb rgb = bgrToRgb(img.at<cv::Vec3b>(row, col));
            BlockInfoModified *modi = rgbNearest(rgb, modis);
            result[col][img.rows - 1 - row][0] = modi->blockId;
            if (blocksInfo != nullptr) {
                if (blocksInfo->find(modi->blockId) == blocksInfo->end())
                    blocksInfo->insert({ modi->blockId, 0 });
                else
                    ++blocksInfo->at(modi->blockId);
            }
        }
    }
    return result;
}

static BlockCube getBlocks(cv::VideoCapture &video, BIModis &modis, int maxWidth, int maxHeight,
                           int maxFrameCount, std::unordered_map<std::string, int> *blocksInfo = nullptr)
{
    BlockCube result(0, 0, 0);
    maxFrameCount = static_cast<int>(video.get(cv::CAP_PROP_FRAME_COUNT)) > maxFrameCount ?
        maxFrameCount : static_cast<int>(video.get(cv::CAP_PROP_FRAME_COUNT));
    cv::Mat frame;
    int z = 0;
    while (video.read(frame)) {
        limitScale(frame, maxWidth, maxHeight);
        cv::flip(frame, frame, 1);
        if (z == 0)
            result = BlockCube(frame.cols, frame.rows, maxFrameCount);
        for (int row = 0; row < frame.rows; ++row) {
            for (int col = 0; col < frame.cols; ++col) {
                Rgb rgb = bgrToRgb(frame.at<cv::Vec3b>(row, col));
                BlockInfoModified *modi = rgbNearest(rgb, modis);
                result[col][frame.rows - 1 - row][z] = modi->blockId;
                if (blocksInfo != nullptr) {
                    if (blocksInfo->find(modi->blockId) == blocksInfo->end())
                        blocksInfo->insert({ modi->blockId, 0 });
                    else
                        ++blocksInfo->at(modi->blockId);
                }
            }
        }
        if (++z == maxFrameCount)
            break;
    }
    return result;
}

static cv::Mat getBlockImage(cv::Mat &img, BIModis &modis, const std::string &texturePath,
                             int maxWidth, int maxHeight, std::unordered_map<std::string, int> *blocksInfo = nullptr)
{
    if (modis.empty() || img.empty() || img.type() != CV_8UC3)
        return cv::Mat();
    if (maxWidth != 0 && maxHeight != 0)
        limitScale(img, maxWidth, maxHeight);
    std::unordered_map<std::string, cv::Mat *> map;
    cv::Mat result(img.rows * 16, img.cols * 16, CV_8UC3);
    for (int row = 0; row < img.rows; ++row) {
        for (int col = 0; col < img.cols; ++col) {
            Rgb rgb = bgrToRgb(img.at<cv::Vec3b>(row, col));
            BlockInfoModified *modi = rgbNearest(rgb, modis);
            if (map.find(modi->textureName) == map.end()) {
                cv::Mat *img = new cv::Mat(cv::imread(texturePath + "/" + modi->textureName));
                map.insert({ modi->textureName, img });
                img->copyTo(result(cv::Range(row * 16, row * 16 + 16), cv::Range(col * 16, col * 16 + 16)));
            } else {
                map[modi->textureName]->copyTo(result(cv::Range(row * 16, row * 16 + 16),
                                                      cv::Range(col * 16, col * 16 + 16)));
            }
            if (blocksInfo != nullptr) {
                if (blocksInfo->find(modi->blockId) == blocksInfo->end())
                    blocksInfo->insert({ modi->blockId, 0 });
                else
                    ++blocksInfo->at(modi->blockId);
            }
        }
    }
    return result;
}

static std::vector<std::string> getCommands(const BlockCube &blocks, Plane plane,
                                            bool useNewExecute = true, const Posli &offset = Posli(0, 0, 1))
{
    using namespace Command;
    std::vector<std::string> commands;

    for (int z = 0; z < blocks.z; ++z) {
        for (int y = 0; y < blocks.y; ++y) {
            int x = 0;
            BlockCluster cluster;
            cluster.blockId = blocks[x][y][z];
            cluster.posFrom = Posi(x, y, z);
            for (; x < blocks.x; ++x) {
                if (blocks[x][y][z] != cluster.blockId || x == blocks.x - 1) {
                    int count = x == blocks.x - 1 ? 2 : 1;
                    for (int i = 0; i < count; ++i) {
                        switch (plane) {
                            case XY_Z:
                                break;
                            case ZY_X:
                                std::swap(cluster.posFrom.x, cluster.posFrom.z);
                                std::swap(cluster.posTo.x, cluster.posTo.z);
                                break;
                            case XZ_Y:
                                std::swap(cluster.posFrom.z, cluster.posFrom.y);
                                std::swap(cluster.posTo.y, cluster.posTo.y);
                                break;
                            default:
                                break;
                        }
                        commands.push_back(execute(Selector::Nearest, Selector::Own,
                                                   fill(cluster.blockId,
                                                        { cluster.posFrom.x, cluster.posFrom.y, cluster.posFrom.z },
                                                        {cluster.posTo.x, cluster.posTo.y, cluster.posTo.z },
                                                        PosMode::Relative)));
                        cluster.blockId = blocks[x][y][z];
                        cluster.posFrom = Posi(x, y, z);
                        cluster.posTo = Posi(x, y, z);
                    }
                }
                cluster.posTo = Posi(x, y, z);
            }
        }
    }

    return commands;
}

static Nbt::Tag getMcstructure(const BlockCube &blocks, Plane plane) {
    using namespace Nbt;

    int xs = 0, ys = 0, zs = 0;
    switch (plane) {
        case XY_Z:
            xs = blocks.x;
            ys = blocks.y;
            zs = blocks.z;
            break;
        case ZY_X:
            xs = blocks.z;
            ys = blocks.y;
            zs = blocks.x;
            break;
        case XZ_Y:
            xs = blocks.x;
            ys = blocks.z;
            zs = blocks.y;
            break;
        default:
            break;
    }

    Tag formatVersion = gInt("format_version", 1);
    Tag size = gList("size", Int);
    size << gpInt(xs) << gpInt(ys) << gpInt(zs);
    Tag swo = gList("structure_world_origin", Int);
    swo << gpInt(0) << gpInt(0) << gpInt(0);
    Tag data1 = gpList(Int);
    Tag data2 = gpList(Int);
    Tag blockPalette = gList("block_palette", Compound);

    std::unordered_map<std::string, int> map;
    int index = 0;
    for (int x = 0; x < xs; ++x) {
        for (int y = 0; y < ys; ++y) {
            for (int z = 0; z < zs; ++z) {
                std::string blockId;
                switch (plane) {
                    case XY_Z:
                        blockId = blocks[x][y][z];
                        break;
                    case ZY_X:
                        blockId = blocks[z][y][x];
                        break;
                    case XZ_Y:
                        blockId = blocks[x][z][y];
                        break;
                    default:
                        break;
                }
                data2 << gpInt(-1);
                if (map.find(blockId) == map.end()) {
                    map.insert({ blockId, index });
                    data1.addMember(gpInt(index));
                    Tag block = gCompound();
                    block << gCompound("states") << gInt("version", 18103297) << gString("name", blockId);
                    blockPalette << block;
                    ++index;
                    continue;
                }
                data1.addMember(gpInt(map[blockId]));
            }
        }
    }
    Tag s2 = gCompound("structure");
    Tag s3 = gCompound("palette");
    Tag s4 = gCompound("default");
    s4 << gCompound("block_position_data") << blockPalette;
    Tag s5 = gList("block_indices", List);
    s5 << data1 << data2;
    s3 << s4;;
    s2 << s5 << gList("entities", End) << s3;
    Tag root = gCompound();
    root << formatVersion << size << s2 << swo;
    return root;
}

static Nbt::Tag getAirStructure(int x, int y, int z, Plane plane) {
    using namespace Nbt;

    int xs = 0, ys = 0, zs = 0;
    switch (plane) {
        case XY_Z:
            xs = x;
            ys = y;
            zs = z;
            break;
        case ZY_X:
            xs = z;
            ys = y;
            zs = x;
            break;
        case XZ_Y:
            xs = x;
            ys = z;
            zs = y;
            break;
        default:
            break;
    }

    Tag formatVersion = gInt("format_version", 1);
    Tag size = gList("size", Int);
    size << gpInt(xs) << gpInt(ys) << gpInt(zs);
    Tag swo = gList("structure_world_origin", Int);
    swo << gpInt(0) << gpInt(0) << gpInt(0);
    Tag data1 = gpList(Int);
    Tag data2 = gpList(Int);
    Tag blockPalette = gList("block_palette", Compound);
    Tag block = gCompound();
    block << gCompound("states") << gInt("version", 18103297) << gString("name", "minecraft:air");
    blockPalette << block;

    int all = x * y * z;
    for (int i = 0; i < all; ++i) {
        data1.addMember(gpInt(0));
        data2 << gpInt(-1);
    }
    Tag s2 = gCompound("structure");
    Tag s3 = gCompound("palette");
    Tag s4 = gCompound("default");
    s4 << gCompound("block_position_data") << blockPalette;
    Tag s5 = gList("block_indices", List);
    s5 << data1 << data2;
    s3 << s4;;
    s2 << s5 << gList("entities", End) << s3;
    Tag root = gCompound();
    root << formatVersion << size << s2 << swo;
    return root;
}

static Bf::Dir makeFunctionPack(cv::Mat &img, BIModis &modis, const Mcpack::PackManifest &manifest,
                                Plane plane = XY_Z, int maxWidth = 480, int maxHeight = 270,
                                int maxCommandCount = 9000, bool useNewExecute = true)
{
    BlockCube blocks = getBlocks(img, modis, maxWidth, maxHeight);
    std::vector<std::string> commands = getCommands(blocks, plane, useNewExecute);
    Bf::Dir root = getMcpackFrame(manifest);

    // Write command data.
    int count = 0;
    int index = 0;
    for (auto &var : commands) {
        if (++count > maxCommandCount) {
            ++index;
            count = 0;
        }
        root["functions"][manifest.prefix]["data"]("d" + std::to_string(index) + ".mcfunction") <<
            var << "\n";
    }

    // Write AUX control data.
    Bf::File &control = root["functions"][manifest.prefix]["aux"]("control.mcfunction");
    std::string scoreboardObj = manifest.prefix + "_Control";
    std::string scoreboardPly = manifest.prefix + "_Dummy";
    for (int i = 0; i <= index; ++i) {
        control << "execute if score " << scoreboardPly << " " << scoreboardObj <<
            " matches " << std::to_string(i) << " run function " << manifest.prefix <<
            "/data/d" << std::to_string(i) << "\n";
    }
    control << "execute if score " << scoreboardPly << " " << scoreboardObj << " matches 0.. run " <<
        "scoreboard players add " << scoreboardPly << " " << scoreboardObj << " 1\n";
    control << "execute if score " << scoreboardPly << " " << scoreboardObj << " matches " <<
        std::to_string(index + 1) << " run " << "tickingarea remove " << manifest.prefix + "_Tickarea\n";
    control << "execute if score " << scoreboardPly << " " << scoreboardObj << " matches " <<
        std::to_string(index + 1) << " run " << "scoreboard objectives remove " << scoreboardObj;

    // Get area size.
    int xs = 0, ys = 0, zs = 0;
    switch (plane) {
        case XY_Z:
            xs = blocks.x;
            ys = blocks.y;
            zs = blocks.z;
            break;
        case ZY_X:
            xs = blocks.z;
            ys = blocks.y;
            zs = blocks.x;
            break;
        case XZ_Y:
            xs = blocks.x;
            ys = blocks.z;
            zs = blocks.y;
            break;
        default:
            break;
    }

    // Write start control.
    Bf::File &start = root["functions"][manifest.prefix]("start.mcfunction");
    start << "scoreboard objectives add " << scoreboardObj << " dummy\n";
    start << "tickingarea add ~~~ ~" + std::to_string(xs - 1) + " ~" + std::to_string(ys - 1) + " ~" +
        std::to_string(zs - 1) + " " + manifest.prefix + "_Tickarea\n";
    start << "execute unless score " << scoreboardPly << " " << scoreboardObj <<
        " matches 0.. run scoreboard players set " << scoreboardPly + " " << scoreboardObj << " 0";

    // Write tick json.
    rapidjson::Document dom;
    dom.Parse(root["functions"]("tick.json").data().c_str());
    rapidjson::Value controlPath((manifest.prefix + "/aux/control").c_str(), dom.GetAllocator());
    dom["values"].GetArray().PushBack(controlPath, dom.GetAllocator());
    root["functions"]("tick.json") = domToStr(dom);

    // Save all.
    return root;
}

static Bf::Dir makeStructurePack(cv::Mat &img, BIModis &modis, const Mcpack::PackManifest &manifest,
                                 Plane plane = XY_Z, int maxWidth = 480, int maxHeight = 270)
{
    BlockCube blocks = getBlocks(img, modis, maxWidth, maxHeight);
    Nbt::Tag tag = getMcstructure(blocks, plane);
    Bf::Dir root = getMcpackFrame(manifest);

    std::stringstream ss;
    tag.write(ss);
    root["structures"][manifest.prefix]("data.mcstructure") = ss.str();

    return root;
}

static Bf::Dir makeStructurePack(cv::VideoCapture &video, BIModis &modis, const Mcpack::PackManifest &manifest,
                                 Plane plane = XY_Z, int maxWidth = 480, int maxHeight = 270,
                                 int maxFrameCount = 200, bool detachFrame = true)
{
    if (detachFrame) {
        int totalFrame = static_cast<int>(video.get(cv::CAP_PROP_FRAME_COUNT));
        totalFrame = totalFrame < maxFrameCount ? totalFrame : maxFrameCount;
        Bf::Dir root = getMcpackFrame(manifest);
        cv::Mat frame;
        for (int i = 0; i < totalFrame; ++i) {
            video.read(frame);
            BlockCube blocks = getBlocks(frame, modis, maxWidth, maxHeight);
            Nbt::Tag tag = getMcstructure(blocks, plane);
            std::stringstream ss;
            tag.write(ss);
            root["structures"][manifest.prefix]("d" + std::to_string(i) + ".mcstructure") = ss.str();
        }

        // Write AUX control data.
        Bf::File &control = root["functions"][manifest.prefix]["aux"]("control.mcfunction");
        std::string scoreboardObj = manifest.prefix + "_Control";
        std::string scoreboardPly = manifest.prefix + "_Dummy";
        for (int i = 0; i < totalFrame; ++i) {
            control << "execute as @e[name=" << "__" + manifest.prefix << ",c=1] at @s if score " << scoreboardPly <<
                " " << scoreboardObj << " matches " << std::to_string(i) << " run structure load " <<
                manifest.prefix << ":d" << std::to_string(i) << " ~~~\n";
        }
        control << "execute if score " << scoreboardPly << " " << scoreboardObj << " matches 0.. run " <<
            "scoreboard players add " << scoreboardPly << " " << scoreboardObj << " 1\n";
        control << "execute if score " << scoreboardPly << " " << scoreboardObj << " matches " <<
            std::to_string(totalFrame) << " run " << "tickingarea remove " << manifest.prefix + "_Tickarea\n";
        control << "execute if score " << scoreboardPly << " " << scoreboardObj << " matches " <<
            std::to_string(totalFrame) << " run kill @e[type=armor_stand,name=__" + manifest.prefix + "]\n";
        control << "execute if score " << scoreboardPly << " " << scoreboardObj << " matches " <<
            std::to_string(totalFrame) << " run " << "scoreboard objectives remove " << scoreboardObj;

        // Get area size.
        int width = frame.cols;
        int height = frame.rows;
        int xs = 0, ys = 0, zs = 0;
        switch (plane) {
            case XY_Z:
                xs = width;
                ys = height;
                zs = 1;
                break;
            case ZY_X:
                xs = 1;
                ys = height;
                zs = width;
                break;
            case XZ_Y:
                xs = width;
                ys = 1;
                zs = height;
                break;
            default:
                break;
        }

        // Write setO control.
        Bf::File &setO = root["functions"][manifest.prefix]("setO.mcfunction");
        setO << "execute as @p at @s run summon minecraft:armor_stand __" + manifest.prefix << "\n";
        setO << "execute as @e[type=minecraft:armor_stand,name=__" + manifest.prefix + "] at @s run effect @s invisibility 999999 0 true";

        // Write play control.
        Bf::File &play = root["functions"][manifest.prefix]("play.mcfunction");
        play << "scoreboard objectives add " << scoreboardObj << " dummy\n";
        play << "execute as @e[name=" << "__" + manifest.prefix << ",c=1] at @s run tickingarea add ~~~ ~" +
            std::to_string(xs - 1) + " ~" + std::to_string(ys - 1) + " ~" +
            std::to_string(zs - 1) + " " + manifest.prefix + "_Tickarea\n";
        play << "execute unless score " << scoreboardPly << " " << scoreboardObj <<
            " matches 0.. run scoreboard players set " << scoreboardPly + " " << scoreboardObj << " 0";

        // Write tick json.
        rapidjson::Document dom;
        dom.Parse(root["functions"]("tick.json").data().c_str());
        rapidjson::Value controlPath((manifest.prefix + "/aux/control").c_str(), dom.GetAllocator());
        dom["values"].GetArray().PushBack(controlPath, dom.GetAllocator());
        root["functions"]("tick.json") = domToStr(dom);

        return root;
    }

    BlockCube blocks = getBlocks(video, modis, maxFrameCount, maxWidth, maxHeight);
    Nbt::Tag tag = getMcstructure(blocks, plane);
    Bf::Dir root = getMcpackFrame(manifest);
    std::stringstream ss;
    tag.write(ss);
    root["structures"][manifest.prefix]("data.mcstructure") = ss.str();

    return root;
}

BIModis filterBIRaws(const BIRaws &raws, Plane plane, int attribute, Version version) {
    int face = 0;
    int alignment = 0;
    if (plane == XY_Z || plane == ZY_X) {
        face = BlockFlag::Side;
        alignment = BlockFlag::Vertical;
    } else {
        face = BlockFlag::Top;
        alignment = BlockFlag::Horizontal;
    }
    return rawsToModis(raws, face, alignment, attribute, version);
}

void makeBlockImage(const std::string &imgPath, const std::string &outputPath,
                    BIModis &modis, const std::string &texturePath, int maxWidth, int maxHeight,
                    std::unordered_map<std::string, int> *blocksInfo)
{
    cv::Mat img = cv::imread(imgPath);
    cv::Mat result = getBlockImage(img, modis, texturePath, maxWidth, maxHeight, blocksInfo);
    cv::imwrite(outputPath + "/" + Bf::getFileName(imgPath) + "_BlockImage.jpg", result);
}

void makeImageFunctionPack(const std::string &imgPath, const std::string &outputPath,
                           BIModis &modis, const Mcpack::PackManifest &manifest, Plane plane,
                           int maxWidth, int maxHeight, int maxCommandCount, bool useNewExecute,
                           bool isCompress)
{
    cv::Mat img = cv::imread(imgPath);
    Bf::Dir dir = makeFunctionPack(img, modis, manifest, plane, maxWidth, maxHeight, maxCommandCount,
                                   useNewExecute);
    dir.write(outputPath, Bf::Override);
    if (isCompress) {
        compressFolder(outputPath + "/" + dir.name(), outputPath + "/" + dir.name() + ".mcpack");
        Bf::deleteDirectory(outputPath + "/" + dir.name());
    }
}

void makeImageStructurePack(const std::string &imgPath, const std::string &outputPath,
                            BIModis &modis, const Mcpack::PackManifest &manifest, Plane plane,
                            int maxWidth, int maxHeight, bool isCompress)
{
    cv::Mat img = cv::imread(imgPath);
    Bf::Dir dir = makeStructurePack(img, modis, manifest, plane, maxWidth, maxHeight);
    dir.write(outputPath, Bf::Override);
    if (isCompress) {
        compressFolder(outputPath + "/" + dir.name(), outputPath + "/" + dir.name() + ".mcpack");
        Bf::deleteDirectory(outputPath + "/" + dir.name());
    }
}

void makeVideoStructurePack(const std::string &videoPath, const std::string &outputPath,
                            BIModis &modis, const Mcpack::PackManifest &manifest, Plane plane,
                            int maxWidth, int maxHeight, int maxFrameCount, bool detachFrame,
                            bool isCompress)
{
    cv::VideoCapture video(videoPath);
    Bf::Dir dir = makeStructurePack(video, modis, manifest, plane, maxWidth, maxHeight,
                                    maxFrameCount, detachFrame);
    dir.write(outputPath, Bf::Override);
    if (isCompress) {
        compressFolder(outputPath + "/" + dir.name(), outputPath + "/" + dir.name() + ".mcpack");
        Bf::deleteDirectory(outputPath + "/" + dir.name());
    }
}

