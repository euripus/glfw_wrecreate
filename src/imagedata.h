#ifndef IMAGEDATA_H
#define IMAGEDATA_H

#include <cstdint>
#include <memory>
#include <string>

namespace tex
{
struct ImageData
{
    // origin is the lower-left corner
    enum class PixelType
    {
        pt_rgb,
        pt_rgba,
        pt_none
    };

    uint32_t                   width  = 0;
    uint32_t                   height = 0;
    PixelType                  type   = PixelType::pt_none;
    std::unique_ptr<uint8_t[]> data;
};

bool ReadBMP(std::string const & file_name, ImageData & id);
bool ReadTGA(std::string const & file_name, ImageData & id);

bool WriteTGA(std::string file_name, ImageData const & id);
}   // namespace evnt
#endif   // IMAGEDATA_H
