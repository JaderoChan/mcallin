#include "preprocess.hpp"

#include <cmath>
#include <cassert>

#include <vector>
#include <fstream>

#include <betterfiles.hpp>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

#undef GetObject

static inline std::string byteToHexstr(unsigned char num, bool isUppercase = true, bool justify = true) {
    std::stringstream ss;
    if (isUppercase)
        ss << std::uppercase << std::hex << num;
    else
        ss << std::hex << num;
    std::string str = ss.str();
    if (justify && str.size() % 2 != 0)
        str.insert(0, "0");
    return str;
}

static inline Rgb hexstrToRgb(const std::string &hexstr) {
    if (hexstr.size() < 6)
        return Rgb();
    int rgb[3] = { 0, 0, 0 };
    for (int i = 0; i < 6; ++i) {
        char ch = toupper(hexstr[i]);
        if (ch >= '0' && ch <= '9')
            rgb[i / 2] += i % 2 ? (ch - '0') : (ch - '0') * 16;
        else if (ch >= 'A' && ch <= 'F')
            rgb[i / 2] += i % 2 ? (ch - 'A' + 10) : (ch - 'A' + 10) * 16;
        else
            return Rgb();
    }
    return Rgb(rgb[0], rgb[1], rgb[2]);
}

static inline std::string rgbToHexstr(const Rgb &rgb, bool isUppercase = true) {
    std::string str;
    str = byteToHexstr(rgb.r, isUppercase) +
        byteToHexstr(rgb.g, isUppercase) +
        byteToHexstr(rgb.b, isUppercase);
    return str;
}

static inline Rgb bgrToRgb(const cv::Vec3b &cvBgr) {
    return Rgb(cvBgr[2], cvBgr[1], cvBgr[0]);
}

static inline cv::Vec3b rgbToBgr(const Rgb &rgb) {
    return cv::Vec3b(rgb.b, rgb.g, rgb.r);
}

// @brief Gets the average color of the image.
// @return The BGR value.
static inline cv::Vec3b getAverageColor(const cv::Mat &image) {
    if (image.empty()) {
        std::cerr << "In line " << __LINE__ << ", " << __FUNCTION__ << " " << "The image is invalid." << std::endl;
        return cv::Vec3b(0, 0, 0);
    }
    int totalPixel = static_cast<int>(image.total());
    cv::Mat img = image.reshape(0, totalPixel);
    img.convertTo(img, CV_8UC3);
    cv::Vec3d sum(0.0, 0.0, 0.0);
    for (int i = totalPixel - 1; i >= 0; --i)
        sum += img.at<cv::Vec3b>(i, 0);
    return sum / totalPixel;
}

// @brief Gets the theme color of the image by Kmeans algorithm and show.
// @param K Number of clusters, that is how many color what do you want. If K is 1 it equal to Average color.
// @param iter Iterations.
// @param repeat Times of repetition.
static inline void getThemeColor(const cv::Mat &image, int K, int iter, int repeat) {
    if (image.empty()) {
        std::cerr << "In line " << __LINE__ << ", " << __FUNCTION__ << " " << "The image is invalid." << std::endl;
        return;
    }
    cv::Mat src;
    src = image.reshape(1, int(src.total()));
    src.convertTo(src, CV_32F);
    src /= 255.f;

    cv::Mat labels;
    cv::Mat centers;
    cv::TermCriteria criteria = cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, iter, 0.1);
    cv::kmeans(src, K, labels, criteria, repeat, cv::KMEANS_RANDOM_CENTERS, centers);

    centers *= 255.f;
    centers.convertTo(centers, CV_8UC3);

    cv::Mat result(K * 100, K * 100, CV_8UC3);
    for (int i = 0; i < K; ++i) {
        result.rowRange(i * 100, K * 100) = centers.at<cv::Vec3b>(i, 0);
    }
    cv::imshow("imageThemeColor", result);
    cv::waitKey();
}

static inline BlockFlag::Faces _getFaceByStr(const std::string &str) {
    if (str == PREPROC_KW_FRONT)
        return BlockFlag::Front;
    if (str == PREPROC_KW_BACK)
        return BlockFlag::Back;
    if (str == PREPROC_KW_RIGHT)
        return BlockFlag::Right;
    if (str == PREPROC_KW_LEFT)
        return BlockFlag::Left;
    if (str == PREPROC_KW_TOP)
        return BlockFlag::Top;
    if (str == PREPROC_KW_BOTTOM)
        return BlockFlag::Bottom;
    if (str == PREPROC_KW_SIDE)
        return BlockFlag::Side;
    return BlockFlag::Side;
}

static inline BlockInfoRaw _getBIRawByJsonObj(const rapidjson::Value &object) {
    if (!object.IsObject())
        std::terminate();
    assert(object.HasMember(PREPROC_KW_IdS) &&
           object.HasMember(PREPROC_KW_TEXS) &&
           object.HasMember(PREPROC_KW_COLORS) &&
           object.HasMember(PREPROC_KW_DEBUTVERS) &&
           object.HasMember(PREPROC_KW_ALIGNMENT));

    auto &ids = object[PREPROC_KW_IdS];
    auto &facesTexturePath = object[PREPROC_KW_TEXS];
    auto &facesColor = object[PREPROC_KW_COLORS];
    auto &debutVersion = object[PREPROC_KW_DEBUTVERS];
    auto &alignments = object[PREPROC_KW_ALIGNMENT];
    assert(ids.IsArray() &&
           facesTexturePath.IsObject() &&
           facesColor.IsObject() &&
           debutVersion.IsArray() &&
           alignments.IsArray());

    std::vector<std::string> attrStrs;
    attrStrs.push_back(PREPROC_KW_ISLIGHTING);
    attrStrs.push_back(PREPROC_KW_ISTIMEVRAYING);
    attrStrs.push_back(PREPROC_KW_BURNABLE);
    attrStrs.push_back(PREPROC_KW_PICKABLE);
    attrStrs.push_back(PREPROC_KW_HASGRAVITY);
    attrStrs.push_back(PREPROC_KW_HASENERGY);
    attrStrs.push_back(PREPROC_KW_ISTRANSPARENCY);
    attrStrs.push_back(PREPROC_KW_ISCOMMANDFORMATId);

    for (auto it = attrStrs.begin(); it != attrStrs.end();) {
        if (object.HasMember((*it).c_str())) {
            assert(object[(*it).c_str()].IsBool());
            ++it;
            continue;
        }
        it = attrStrs.erase(it);
    }

    BlockInfoRaw result;
    // Get the debut version of the block.
    result.debutVersion = Version(debutVersion.GetArray()[0].GetInt(),
                                  debutVersion.GetArray()[1].GetInt(),
                                  debutVersion.GetArray()[2].GetInt());
    // Get the block id in different versions.
    for (auto &obj : ids.GetArray()) {
        assert(obj.HasMember(PREPROC_KW_Id) && obj.HasMember(PREPROC_KW_VERS));
        assert(obj[PREPROC_KW_Id].IsString() && obj[PREPROC_KW_VERS].IsArray());
        std::string id = obj[PREPROC_KW_Id].GetString();
        Version version = Version(obj[PREPROC_KW_VERS].GetArray()[0].GetInt(),
                                  obj[PREPROC_KW_VERS].GetArray()[1].GetInt(),
                                  obj[PREPROC_KW_VERS].GetArray()[2].GetInt());
        result.ids.insert({ version, id });
    }
    // Get the aligmnet info of the block.
    for (auto &alignment : alignments.GetArray()) {
        assert(alignment.IsString());
        if (std::string(alignment.GetString()) == PREPROC_KW_X)
            result.alignment |= BlockFlag::Horizontal;
        if (std::string(alignment.GetString()) == PREPROC_KW_Y)
            result.alignment |= BlockFlag::Vertical;
    }
    // Get the texture path of the block different faces.
    for (auto &textureName : facesTexturePath.GetObject()) {
        if (!textureName.value.IsString())
            continue;
        std::string key = textureName.name.GetString();
        std::string value = textureName.value.GetString();
        result.facesTexturePath.insert({ _getFaceByStr(key), value });
    }
    // Get the main color of the block different faces.
    for (auto &color : facesColor.GetObject()) {
        if (!color.value.IsString())
            continue;
        std::string key = color.name.GetString();
        std::string value = color.value.GetString();
        result.facesColor.insert({ _getFaceByStr(key), hexstrToRgb(value) });
    }
    // Get the attribute of the block.
    for (std::size_t i = 0; i < attrStrs.size(); ++i) {
        if (object[attrStrs[i].c_str()].GetBool()) {
            if (attrStrs[i] == PREPROC_KW_ISLIGHTING)
                result.attribute |= BlockFlag::IsLighting;
            if (attrStrs[i] == PREPROC_KW_ISTIMEVRAYING)
                result.attribute |= BlockFlag::IsTimeVarying;
            if (attrStrs[i] == PREPROC_KW_BURNABLE)
                result.attribute |= BlockFlag::Burnable;
            if (attrStrs[i] == PREPROC_KW_PICKABLE)
                result.attribute |= BlockFlag::EndermanPickable;
            if (attrStrs[i] == PREPROC_KW_HASGRAVITY)
                result.attribute |= BlockFlag::HasGravity;
            if (attrStrs[i] == PREPROC_KW_HASENERGY)
                result.attribute |= BlockFlag::HasEnergy;
            if (attrStrs[i] == PREPROC_KW_ISTRANSPARENCY)
                result.attribute |= BlockFlag::IsTransparency;
            if (attrStrs[i] == PREPROC_KW_ISCOMMANDFORMATId)
                result.attribute |= BlockFlag::IsCommandFormatId;
        }
    }
    return result;
}

BIRaws getBIRawsByDomFile(const std::string &filepath) noexcept {
    std::ifstream file(filepath);
    std::string json;
    std::string line;
    assert(file.is_open());
    while (file >> line)
        json += line;
    file.close();

    rapidjson::Document dom;
    dom.Parse(json.c_str());
    assert(dom.IsObject());
    assert(dom.HasMember(PREPROC_KW_ROOT));

    auto &objs = dom[PREPROC_KW_ROOT];
    assert(objs.IsArray());

    BIRaws result;
    for (auto &obj : objs.GetArray())
        result.emplace_back(BlockInfoRaw(_getBIRawByJsonObj(obj)));
    return result;
}

BIModis rawsToModis(const BIRaws &raws, int face, int alignment, int attribute,
                    Version debutVersion) {
    assert(!raws.empty());
    BIModis result;
    for (auto &raw : raws) {
        int faceFlag = 0;
        for (auto &var : raw.facesColor)
            faceFlag |= var.first;
        if (!(raw.alignment & alignment))
            continue;
        if (raw.debutVersion < debutVersion)
            continue;
        if((raw.attribute & attribute) != raw.attribute)
            continue;
        if ((face & faceFlag) == 0)
            continue;
        std::string blockId;
        std::string textureName;
        Rgb color;
        for (auto &var : raw.ids) {
            if (debutVersion > var.first)
                continue;
            blockId = var.second;
            break;
        }
        if (raw.alignment == (BlockFlag::Vertical | BlockFlag::Horizontal) &&
            raw.facesColor.size() == 1) {
            textureName = (*raw.facesTexturePath.begin()).second;
            color = (*raw.facesColor.begin()).second;
        } else {
            if (raw.facesTexturePath.find(static_cast<BlockFlag::Faces>(face)) == raw.facesTexturePath.end())
                continue;
            textureName = raw.facesTexturePath.at(static_cast<BlockFlag::Faces>(face));
            if (raw.facesColor.find(static_cast<BlockFlag::Faces>(face)) == raw.facesColor.end())
                continue;
            color = raw.facesColor.at(static_cast<BlockFlag::Faces>(face));
        }
        result.emplace_back(BlockInfoModified(blockId, textureName, color));
    }
    return result;
}

bool textureAndblockIdHandle(const std::string &textureDirPath, const std::string &blockInfosFilePath)
{
    std::ifstream blockInfosFileIn;
    std::vector<std::string> files = Bf::getAllFiles(textureDirPath);
    blockInfosFileIn.open(blockInfosFilePath);
    if (!blockInfosFileIn.is_open()) {
        std::cerr << "In line " << __LINE__ << ", " << __FUNCTION__ << " " << "Failed to open file." << std::endl;
        return false;
    }
    std::string json;
    std::string line;
    while (blockInfosFileIn >> line)
        json += line;
    blockInfosFileIn.close();

    rapidjson::Document dom;
    dom.Parse(json.c_str());

    assert(dom.IsObject());
    assert(dom.HasMember(PREPROC_KW_ROOT));

    if (!dom.IsObject())
        return false;
    if (!dom.HasMember(PREPROC_KW_ROOT))
        return false;

    auto &objs = dom[PREPROC_KW_ROOT];

    assert(objs.IsArray());

    if (!objs.IsArray())
        return false;

    for (auto &obj : objs.GetArray()) {
        auto textures = obj.FindMember(PREPROC_KW_TEXS);
        auto rgbs = obj.FindMember(PREPROC_KW_COLORS);
        if (textures == obj.MemberEnd())
            continue;
        if (rgbs == obj.MemberEnd()) {
            rapidjson::Value _rgbs(rapidjson::kObjectType);
            obj.AddMember(PREPROC_KW_COLORS, _rgbs, dom.GetAllocator());
        }
        auto textureSide = textures->value.FindMember(PREPROC_KW_SIDE);
        if (textureSide != textures->value.MemberEnd() &&
            textureSide->value.IsString()) {
            std::string path = textureDirPath + '\\' + textureSide->value.GetString();
            cv::Mat img = cv::imread(path);
            cv::Vec3b bgr = getAverageColor(img);
            Rgb rgb = bgrToRgb(bgr);
            std::string color = rgbToHexstr(rgb);
            auto rgbSide = rgbs->value.FindMember(PREPROC_KW_SIDE);
            if (rgbSide == rgbs->value.MemberEnd()) {
                rapidjson::Value _rgbSide(rapidjson::kStringType);
                rgbs->value.AddMember(PREPROC_KW_SIDE, _rgbSide, dom.GetAllocator());
                rgbSide = rgbs->value.FindMember(PREPROC_KW_SIDE);
            }
            if (rgbSide->value.IsString()) {
                rgbSide->value.SetString(color.c_str(), dom.GetAllocator());
            }
        }
        auto textureTop = textures->value.FindMember(PREPROC_KW_TOP);
        if (textureTop != textures->value.MemberEnd() &&
            textureTop->value.IsString()) {
            std::string path = textureDirPath + '\\' + textureTop->value.GetString();
            cv::Mat img = cv::imread(path);
            cv::Vec3b bgr = getAverageColor(img);
            Rgb rgb = bgrToRgb(bgr);
            std::string color = rgbToHexstr(rgb);
            auto rgbTop = rgbs->value.FindMember(PREPROC_KW_TOP);
            if (rgbTop == rgbs->value.MemberEnd()) {
                rapidjson::Value _rgbTop(rapidjson::kStringType);
                rgbs->value.AddMember(PREPROC_KW_TOP, _rgbTop, dom.GetAllocator());
                rgbTop = rgbs->value.FindMember(PREPROC_KW_TOP);
            }
            if (rgbTop->value.IsString()) {
                rgbTop->value.SetString(color.c_str(), dom.GetAllocator());
            }
        }
    }

    std::ofstream blocksInfoFileOut;
    blocksInfoFileOut.open(blockInfosFilePath);
    if (!blocksInfoFileOut.is_open())
        return false;
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    dom.Accept(writer);
    blocksInfoFileOut << buffer.GetString();
    blocksInfoFileOut.close();
    return true;
}
