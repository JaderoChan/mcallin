#ifndef MODULES_HPP
#define MODULES_HPP

#include <string>

#include "preprocess.hpp"
#include "mcpack.hpp"

enum Plane
{
    XY_Z,
    ZY_X,
    XZ_Y
};

BIModis filterBIRaws(const BIRaws &raws, Plane plane, int attribute, Version version);

void makeBlockImage(const std::string &imgPath, const std::string &outputPath,
                           BIModis &modis, const std::string &texturePath, int maxWidth, int maxHeight,
                           std::unordered_map<std::string, int> *blocksInfo = nullptr);

void makeImageFunctionPack(const std::string &imgPath, const std::string &outputPath,
                                  BIModis &modis, const Mcpack::PackManifest &manifest, Plane plane,
                                  int maxWidth, int maxHeight, int maxCommandCount, bool useNewExecute,
                                  bool isCompress);

void makeImageStructurePack(const std::string &imgPath, const std::string &outputPath,
                                   BIModis &modis, const Mcpack::PackManifest &manifest, Plane plane,
                                   int maxWidth, int maxHeight, bool isCompress);

void makeVideoStructurePack(const std::string &imgPath, const std::string &outputPath,
                                   BIModis &modis, const Mcpack::PackManifest &manifest, Plane plane,
                                   int maxWidth, int maxHeight, int maxFrameCount, bool detachFrame,
                                   bool isCompress);

#endif // !MOUDLES_HPP
