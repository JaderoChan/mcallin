#include "mcpack.hpp"

#include <string.h>

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <cppcodec/base64_rfc4648.hpp>

namespace Mcpack
{

// Generates a UUIDv4 string (with a conjunction symbol)
static inline std::string genUuidV4() {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni(0, 15);
    std::uniform_int_distribution<int> uni2(8, 11);

    const char *hexmap = "0123456789abcdef";
    std::stringstream ss;
    for (int i = 0; i < 32; ++i) {
        if (i == 8 || i == 12 || i == 16 || i == 20)
            ss << "-";
        if (i == 12) {
            ss << '4';
            continue;
        }
        if (i == 16) {
            ss << hexmap[uni2(rng)];
            continue;
        }
        ss << hexmap[uni(rng)];
    }
    return ss.str();
}

std::string getManifestJson(const PackManifest &manifest) {
    using namespace rapidjson;

    Document json;
    json.SetObject();

    // Add some nodes to the root node.
    json.AddMember("format_version", manifest.formatVersion, json.GetAllocator());
    json.AddMember("header", kObjectType, json.GetAllocator());
    json.AddMember("modules", kArrayType, json.GetAllocator());
    json["modules"].GetArray().PushBack(kObjectType, json.GetAllocator());

    // Add some nodes to the header node.
    auto &header = json["header"];
    header.AddMember("name", Value(manifest.name.c_str(), json.GetAllocator()), json.GetAllocator());
    header.AddMember("description", Value(manifest.description.c_str(), json.GetAllocator()), json.GetAllocator());
    header.AddMember("uuid", Value(genUuidV4().c_str(), json.GetAllocator()), json.GetAllocator());
    header.AddMember("version", kArrayType, json.GetAllocator());
    header.AddMember("min_engine_version", kArrayType, json.GetAllocator());
    header["version"].GetArray().PushBack(manifest.packVersion[0], json.GetAllocator());
    header["version"].GetArray().PushBack(manifest.packVersion[1], json.GetAllocator());
    header["version"].GetArray().PushBack(manifest.packVersion[2], json.GetAllocator());
    header["min_engine_version"].GetArray().PushBack(manifest.minVersion[0], json.GetAllocator());
    header["min_engine_version"].GetArray().PushBack(manifest.minVersion[1], json.GetAllocator());
    header["min_engine_version"].GetArray().PushBack(manifest.minVersion[2], json.GetAllocator());

    // Add some nodes to the modules node.
    auto modules = json["modules"].GetArray().begin();
    modules->AddMember("description", Value(manifest.description.c_str(), json.GetAllocator()), json.GetAllocator());
    modules->AddMember("type", Value(manifest.getTypeString().c_str(), json.GetAllocator()), json.GetAllocator());
    modules->AddMember("uuid", Value(genUuidV4().c_str(), json.GetAllocator()), json.GetAllocator());
    modules->AddMember("version", kArrayType, json.GetAllocator());
    modules->GetObject()["version"].GetArray().PushBack(manifest.packVersion[0], json.GetAllocator());
    modules->GetObject()["version"].GetArray().PushBack(manifest.packVersion[1], json.GetAllocator());
    modules->GetObject()["version"].GetArray().PushBack(manifest.packVersion[2], json.GetAllocator());

    // Get the string of the json.
    StringBuffer buffer;
    // Use PrettyWriter for indent.
    PrettyWriter<StringBuffer> writer(buffer);
    json.Accept(writer);
    return buffer.GetString();
}

Bf::Dir getMcpackFrame(const PackManifest &manifest) {
    using namespace Bf;

    // The base framework of the mcpack directory.
    Dir root = Dir(manifest.name);
    File mani = File("manifest.json");
    File icon = File("pack_icon.png");
    Dir funcs = Dir("functions");
    Dir funcsSub = Dir(manifest.prefix);
    File tick = File("tick.json");
    Dir struc = Dir("structures");
    Dir strucSub = Dir(manifest.prefix);

    // Decode the icon data and write it.
    std::vector<unsigned char> data = cppcodec::base64_rfc4648::decode(_PACKICON_BASE64,
                                                                       std::strlen(_PACKICON_BASE64));
    icon = data;

    // Write the manifest json data to the file.
    mani = getManifestJson(manifest);

    // Create the tick.json data and write it.
    rapidjson::Document tickJson;
    rapidjson::Value valueArray;
    tickJson.SetObject();
    valueArray.SetArray();
    tickJson.AddMember("values", valueArray, tickJson.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    tickJson.Accept(writer);
    tick = buffer.GetString();

    // Merge the directories and files to the root directory.
    //funcs << funcsSub.move() << tick.move();
    //struc << strucSub.move();
    //root << mani.move() << icon.move() << funcs.move() << struc.move();
    funcs << funcsSub << tick;
    struc << strucSub;
    root << mani << icon << funcs << struc;
    return root;
}

}
