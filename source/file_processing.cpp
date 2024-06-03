#include "file_processing.hpp"

#include <miniz/miniz.h>
#include <betterfiles.hpp>

void compressFolder(const std::string &srcPath, const std::string &destPath)
{
    mz_zip_archive zipArchive;
    memset(&zipArchive, 0, sizeof(zipArchive));

    mz_zip_writer_init_file(&zipArchive, destPath.c_str(), 0);

    std::vector<std::string> files = Bf::getAllFiles(srcPath);
    for (auto &file : files) {
        mz_zip_writer_add_file(&zipArchive,
                               (Bf::getPathSuffix(srcPath) + "/" + file.substr(srcPath.size() + 1)).c_str(),
                               file.c_str(), NULL, 0, MZ_BEST_COMPRESSION);
    }

    mz_zip_writer_finalize_archive(&zipArchive);
    mz_zip_writer_end(&zipArchive);
}
