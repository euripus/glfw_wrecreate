#include "imagedata.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#pragma pack(push, 1)
struct BITMAPFILEHEADER
{
    uint16_t bfType;   // bmp file signature
    uint32_t bfSize;   // file size
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct BITMAPINFO12   // CORE version
{
    uint32_t biSize;
    uint16_t biWidth;
    uint16_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
};

struct BITMAPINFO   // Standart version
{
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};

struct TGAHEADER
{
    uint8_t  idlength;
    uint8_t  colourmaptype;
    uint8_t  datatypecode;
    uint16_t colourmaporigin;
    uint16_t colourmaplength;
    uint8_t  colourmapdepth;
    uint16_t x_origin;
    uint16_t y_origin;
    uint16_t width;
    uint16_t height;
    uint8_t  bitsperpixel;
    uint8_t  imagedescriptor;
};
#pragma pack(pop)

namespace tex
{
//==============================================================================
//         Read BMP section
//==============================================================================
bool ReadBMP(std::string const & file_name, ImageData & id)
{
    bool   res         = false;
    bool   compressed  = false;
    bool   flip        = false;
    size_t file_length = 0;

    id.width  = 0;
    id.height = 0;
    id.type   = ImageData::PixelType::pt_none;
    if(id.data)
        id.data.reset(nullptr);

    std::fstream s(file_name, s.binary | s.in);
    if(s)
    {
        s.seekg(0, std::ios::end);
        file_length = s.tellp();
        s.seekg(0, std::ios::beg);
    }

    if(file_length == 0)
        return res;

    auto buffer_storage = std::make_unique<int8_t[]>(file_length);
    s.read(reinterpret_cast<char *>(buffer_storage.get()), file_length);
    s.close();

    auto buffer = buffer_storage.get();

    auto               pPtr    = buffer;
    BITMAPFILEHEADER * pHeader = reinterpret_cast<BITMAPFILEHEADER *>(pPtr);
    pPtr += sizeof(BITMAPFILEHEADER);
    if(pHeader->bfSize != file_length || pHeader->bfType != 0x4D42)   // little-endian
        return res;

    if(reinterpret_cast<uint32_t *>(pPtr)[0] == 12)
    {
        BITMAPINFO12 * pInfo = reinterpret_cast<BITMAPINFO12 *>(pPtr);
        pPtr += pInfo->biSize;

        if(pInfo->biBitCount != 24 && pInfo->biBitCount != 32)
            return res;

        if(pInfo->biBitCount == 24)
            id.type = ImageData::PixelType::pt_rgb;
        else
            id.type = ImageData::PixelType::pt_rgba;

        id.width  = pInfo->biWidth;
        id.height = pInfo->biHeight;
    }
    else
    {
        BITMAPINFO * pInfo = reinterpret_cast<BITMAPINFO *>(pPtr);
        pPtr += pInfo->biSize;

        if(pInfo->biBitCount != 24 && pInfo->biBitCount != 32)
            return res;

        if(pInfo->biCompression != 3 && pInfo->biCompression != 6 && pInfo->biCompression != 0)
        {
            return res;
        }

        if(pInfo->biCompression == 3 || pInfo->biCompression == 6)
        {
            compressed = true;
        }

        if(pInfo->biBitCount == 24)
            id.type = ImageData::PixelType::pt_rgb;
        else
            id.type = ImageData::PixelType::pt_rgba;

        id.width = pInfo->biWidth;

        if(pInfo->biHeight < 0)
            flip = true;
        id.height = std::abs(pInfo->biHeight);
    }

    // read data:
    pPtr                     = buffer + pHeader->bfOffBits;
    uint32_t lineLength      = 0;
    uint32_t bytes_per_pixel = (id.type == ImageData::PixelType::pt_rgb ? 3 : 4);
    auto     image           = std::make_unique<uint8_t[]>(id.width * id.height * bytes_per_pixel);
    uint8_t  red, green, blue, alpha;
    uint32_t w_ind(0), h_ind(0);

    if(id.type == ImageData::PixelType::pt_rgb)
        lineLength = id.width * bytes_per_pixel + id.width % 4;
    else
        lineLength = id.width * bytes_per_pixel;

    for(uint32_t i = 0; i < id.height; ++i)
    {
        w_ind = 0;
        for(uint32_t j = 0; j < lineLength; j += bytes_per_pixel)
        {
            if(j > id.width * bytes_per_pixel)
                continue;

            if(compressed)
            {
                uint32_t count = 0;
                if(id.type == ImageData::PixelType::pt_rgba)
                {
                    alpha = pPtr[i * lineLength + j + count];
                    count++;
                }
                blue = pPtr[i * lineLength + j + count];
                count++;
                green = pPtr[i * lineLength + j + count];
                count++;
                red = pPtr[i * lineLength + j + count];
            }
            else
            {
                blue  = pPtr[i * lineLength + j + 0];
                green = pPtr[i * lineLength + j + 1];
                red   = pPtr[i * lineLength + j + 2];
                if(id.type == ImageData::PixelType::pt_rgba)   // !!!Not supported - the high byte in each
                                                               // DWORD is not used
                    alpha = pPtr
                        [i * lineLength + j
                         + 3];   // https://msdn.microsoft.com/en-us/library/windows/desktop/dd183376(v=vs.85).aspx
            }

            image[h_ind * id.width * bytes_per_pixel + w_ind * bytes_per_pixel + 0] = red;
            image[h_ind * id.width * bytes_per_pixel + w_ind * bytes_per_pixel + 1] = green;
            image[h_ind * id.width * bytes_per_pixel + w_ind * bytes_per_pixel + 2] = blue;
            if(id.type == ImageData::PixelType::pt_rgba)
                image[h_ind * id.width * bytes_per_pixel + w_ind * bytes_per_pixel + 3] = alpha;
            w_ind++;
        }
        h_ind++;
    }

    // flip image if necessary
    if(flip)
    {
        auto temp_buf = std::make_unique<uint8_t[]>(id.width * id.height * bytes_per_pixel);

        for(uint32_t i = 0; i < id.height; i++)
        {
            std::memcpy(temp_buf.get() + i * id.width * bytes_per_pixel,
                        image.get() + (id.height - 1 - i) * id.width * bytes_per_pixel,
                        id.width * bytes_per_pixel);
        }

        image = std::move(temp_buf);
    }

    id.data = std::move(image);
    res     = true;

    return res;
}

//==============================================================================
//         TGA section
//==============================================================================
bool WriteTGA(std::string fname, const ImageData & id)
{
    TGAHEADER tga;
    std::memset(&tga, 0, sizeof(tga));
    uint8_t bytes_per_pixel = (id.type == ImageData::PixelType::pt_rgb ? 3 : 4);

    std::vector<std::byte> buffer;

    tga.datatypecode = 2;
    tga.width        = static_cast<uint16_t>(id.width);
    tga.height       = static_cast<uint16_t>(id.height);
    tga.bitsperpixel = static_cast<uint8_t>(bytes_per_pixel * 8);
    if(id.type == ImageData::PixelType::pt_rgb)
        tga.imagedescriptor = 0x10;
    else
        tga.imagedescriptor = 0x18;

    buffer.reserve(buffer.size() + sizeof(tga));
    buffer.insert(std::end(buffer), reinterpret_cast<std::byte *>(&tga),
                  reinterpret_cast<std::byte *>(&tga) + sizeof(tga));

    std::byte * data_ptr = reinterpret_cast<std::byte *>(id.data.get());
    std::byte   red, green, blue, alpha;
    for(uint32_t i = 0; i < id.width * id.height * bytes_per_pixel; i += bytes_per_pixel)
    {
        red   = data_ptr[i + 0];
        green = data_ptr[i + 1];
        blue  = data_ptr[i + 2];
        if(bytes_per_pixel == 4)
            alpha = data_ptr[i + 3];

        buffer.push_back(blue);
        buffer.push_back(green);
        buffer.push_back(red);
        if(id.type == ImageData::PixelType::pt_rgba)
            buffer.push_back(alpha);
    }

    std::ofstream outfile(fname, std::ios::out | std::ios::binary);
    if(outfile)
        outfile.write(reinterpret_cast<char *>(buffer.data()), buffer.size());
    else
        return false;

    return true;
}

bool ReadUncompressedTGA(ImageData & id, char * data);
bool ReadCompressedTGA(ImageData & id, char * data);

bool ReadTGA(std::string const & file_name, ImageData & id)
{
    size_t file_length = 0;

    std::fstream s(file_name, s.binary | s.in);
    if(s)
    {
        s.seekg(0, std::ios::end);
        file_length = s.tellp();
        s.seekg(0, std::ios::beg);
    }

    if(file_length == 0)
        return false;

    auto buffer_storage = std::make_unique<int8_t[]>(file_length);
    s.read(reinterpret_cast<char *>(buffer_storage.get()), file_length);
    s.close();

    auto buffer = reinterpret_cast<char *>(buffer_storage.get());

    auto        pPtr    = buffer;
    TGAHEADER * pHeader = reinterpret_cast<TGAHEADER *>(pPtr);
    pPtr += sizeof(TGAHEADER);

    if(pHeader->datatypecode == 2)
    {
        return ReadUncompressedTGA(id, buffer);
    }
    else if(pHeader->datatypecode == 10)
    {
        return ReadCompressedTGA(id, buffer);
    }

    return false;
}

bool ReadUncompressedTGA(ImageData & id, char * data)
{
    char *      pPtr    = data;
    TGAHEADER * pHeader = reinterpret_cast<TGAHEADER *>(pPtr);
    pPtr += sizeof(TGAHEADER);

    if((pHeader->width <= 0) || (pHeader->height <= 0)
       || ((pHeader->bitsperpixel != 24)
           && (pHeader->bitsperpixel != 32)))   // Make sure all information is valid
    {
        return false;
    }

    id.width  = pHeader->width;
    id.height = pHeader->height;
    id.type   = pHeader->bitsperpixel == 24 ? ImageData::PixelType::pt_rgb : ImageData::PixelType::pt_rgba;
    bool flip_horizontal = (pHeader->imagedescriptor & 0x10);
    bool flip_vertical   = (pHeader->imagedescriptor & 0x20);

    uint32_t bytes_per_pixel = pHeader->bitsperpixel / 8;
    uint32_t image_size      = id.width * id.height * bytes_per_pixel;

    auto img = std::make_unique<uint8_t[]>(image_size);

    char red, green, blue, alpha;
    for(uint32_t i = 0; i < id.width * id.height; ++i)
    {
        red   = pPtr[i * bytes_per_pixel + 2];
        green = pPtr[i * bytes_per_pixel + 1];
        blue  = pPtr[i * bytes_per_pixel + 0];
        if(id.type == ImageData::PixelType::pt_rgba)
            alpha = pPtr[i * bytes_per_pixel + 3];

        img[i * bytes_per_pixel + 0] = red;
        img[i * bytes_per_pixel + 1] = green;
        img[i * bytes_per_pixel + 2] = blue;
        if(id.type == ImageData::PixelType::pt_rgba)
            img[i * bytes_per_pixel + 3] = alpha;
    }

    if(flip_vertical)
    {
        auto flipped_img = std::make_unique<uint8_t[]>(image_size);

        for(uint32_t i = 0; i < id.height; i++)
        {
            std::memcpy(flipped_img.get() + i * id.width * bytes_per_pixel,
                        img.get() + (id.height - 1 - i) * id.width * bytes_per_pixel,
                        id.width * bytes_per_pixel);
        }

        img = std::move(flipped_img);
    }

    if(flip_horizontal)
    {
        auto flipped_img = std::make_unique<uint8_t[]>(image_size);

        for(uint32_t i = 0; i < id.height; i++)
        {
            for(uint32_t j = 0; j < id.width; j++)
            {
                flipped_img[id.width * bytes_per_pixel * i + j * bytes_per_pixel + 0] =
                    img[id.width * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 0];
                flipped_img[id.width * bytes_per_pixel * i + j * bytes_per_pixel + 1] =
                    img[id.width * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 1];
                flipped_img[id.width * bytes_per_pixel * i + j * bytes_per_pixel + 2] =
                    img[id.width * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 2];
                if(id.type == ImageData::PixelType::pt_rgba)
                    flipped_img[id.width * bytes_per_pixel * i + j * bytes_per_pixel + 3] =
                        img[id.width * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 3];
            }
        }

        img = std::move(flipped_img);
    }

    id.data = std::move(img);
    return true;
}

bool ReadCompressedTGA(ImageData & id, char * data)
{
    char *      pPtr    = data;
    TGAHEADER * pHeader = reinterpret_cast<TGAHEADER *>(pPtr);
    pPtr += sizeof(TGAHEADER);

    if((pHeader->width <= 0) || (pHeader->height <= 0)
       || ((pHeader->bitsperpixel != 24)
           && (pHeader->bitsperpixel != 32)))   // Make sure all information is valid
    {
        return false;
    }

    id.width  = pHeader->width;
    id.height = pHeader->height;
    id.type   = pHeader->bitsperpixel == 24 ? ImageData::PixelType::pt_rgb : ImageData::PixelType::pt_rgba;
    bool flip_horizontal = (pHeader->imagedescriptor & 0x10);
    bool flip_vertical   = (pHeader->imagedescriptor & 0x20);

    uint32_t bytes_per_pixel = pHeader->bitsperpixel / 8;
    uint32_t image_size      = id.width * id.height * bytes_per_pixel;

    auto     img          = std::make_unique<uint8_t[]>(image_size);
    uint32_t pixelcount   = id.height * id.width;
    uint32_t currentpixel = 0;
    uint32_t currentbyte  = 0;

    do
    {
        int32_t chunk = static_cast<int32_t>(pPtr[0]);
        pPtr++;

        if(chunk & 128)
        {
            chunk -= 127;
            for(int32_t counter = 0; counter < chunk; counter++)
            {
                img[currentbyte + 0] = pPtr[2];
                img[currentbyte + 1] = pPtr[1];
                img[currentbyte + 2] = pPtr[0];
                if(id.type == ImageData::PixelType::pt_rgba)
                    img[currentbyte + 3] = pPtr[3];

                currentbyte += bytes_per_pixel;
                currentpixel++;

                if(currentpixel > pixelcount)   // Make sure we havent written too many pixels
                {
                    return false;
                }
            }
            pPtr += bytes_per_pixel;
        }
        else
        {
            chunk++;
            for(short counter = 0; counter < chunk; counter++)
            {
                img[currentbyte + 0] = pPtr[2];
                img[currentbyte + 1] = pPtr[1];
                img[currentbyte + 2] = pPtr[0];
                if(id.type == ImageData::PixelType::pt_rgba)
                    img[currentbyte + 3] = pPtr[3];

                currentbyte += bytes_per_pixel;
                currentpixel++;
                pPtr += bytes_per_pixel;

                if(currentpixel > pixelcount)   // Make sure we havent read too many pixels
                {
                    return false;
                }
            }
        }
    } while(currentpixel < pixelcount);

    if(flip_vertical)
    {
        auto flipped_img = std::make_unique<uint8_t[]>(image_size);

        for(uint32_t i = 0; i < id.height; i++)
        {
            std::memcpy(flipped_img.get() + i * id.width * bytes_per_pixel,
                        img.get() + (id.height - 1 - i) * id.width * bytes_per_pixel,
                        id.width * bytes_per_pixel);
        }

        img = std::move(flipped_img);
    }

    if(flip_horizontal)
    {
        auto flipped_img = std::make_unique<uint8_t[]>(image_size);

        for(uint32_t i = 0; i < id.height; i++)
        {
            for(uint32_t j = 0; j < id.width; j++)
            {
                flipped_img[id.width * bytes_per_pixel * i + j * bytes_per_pixel + 0] =
                    img[id.width * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 0];
                flipped_img[id.width * bytes_per_pixel * i + j * bytes_per_pixel + 1] =
                    img[id.width * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 1];
                flipped_img[id.width * bytes_per_pixel * i + j * bytes_per_pixel + 2] =
                    img[id.width * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 2];
                if(id.type == ImageData::PixelType::pt_rgba)
                    flipped_img[id.width * bytes_per_pixel * i + j * bytes_per_pixel + 3] =
                        img[id.width * bytes_per_pixel * i + (id.width - j - 1) * bytes_per_pixel + 3];
            }
        }

        img = std::move(flipped_img);
    }

    id.data = std::move(img);
    return true;
}
}   // namespace evnt
