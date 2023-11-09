#pragma once
#include <cstdlib>
#include <exception>
#include <algorithm>
#include <filesystem>

#include "nfd/nfd.h"
#include "stb/stb_image_write.h"

#include "Log.h"
#include "Image.h"


inline std::filesystem::path SaveFileDialog()
{
    std::filesystem::path path;
    nfdchar_t* savePath = NULL;
    nfdresult_t result = NFD_SaveDialog("png;jpg;jpeg;bmp;tga", NULL, &savePath);
    if (result == NFD_OKAY)
    {
        path = savePath;
        free(savePath);
    }
    else if (result == NFD_CANCEL)
        Log("[NFD] User pressed cancel");
    else // error opening the file
        Err("Dateidialog konnte nicht ge�ffnet werden:\n%s", NFD_GetError());
    return path;
}


inline void ImageWriterThrow(bool cond)
{
    if (cond) throw std::runtime_error("");
}


inline void SaveImage(const Image& img)
{
    std::filesystem::path path = SaveFileDialog();
    if (path.empty()) return;
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });

    try
    {
        if (extension == ".jpg" || extension == ".jpeg")
            ImageWriterThrow(!stbi_write_jpg(path.string().c_str(), img.Width(), img.Height(), Image::NumOfChannels, img.Data(), 100));
        else if (extension == ".bmp")
            ImageWriterThrow(!stbi_write_bmp(path.string().c_str(), img.Width(), img.Height(), Image::NumOfChannels, img.Data()));
        else if (extension == ".tga")
            ImageWriterThrow(!stbi_write_tga(path.string().c_str(), img.Width(), img.Height(), Image::NumOfChannels, img.Data()));
        else if (extension == ".png")
            ImageWriterThrow(!stbi_write_png(path.string().c_str(), img.Width(), img.Height(), Image::NumOfChannels, img.Data(), img.Width() * Image::NumOfChannels));
        else
        {
            path += ".png";
            ImageWriterThrow(!stbi_write_png(path.string().c_str(), img.Width(), img.Height(), Image::NumOfChannels, img.Data(), img.Width() * Image::NumOfChannels));
        }
    }
    catch (const std::runtime_error&)
    {
        Err("Bild konnte nicht gespeichert werden.\nPfad: %s\nBreite: %d\nH�he: %d\n", path.string().c_str(), img.Width(), img.Height());
    }
}