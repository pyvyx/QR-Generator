#pragma once
#include <new>
#include <cmath>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

#include "GLFW/glfw3.h"
#include "qrcodegen/cpp/qrcodegen.hpp"

#include "Log.h"
#include "Localization.h"

class Image
{
public:
    static constexpr GLuint NumOfChannels = 3;
private:
    GLubyte* m_Pixel = nullptr;
    GLsizei m_Width = 0;
    GLsizei m_Height = 0;
    GLuint m_GpuImage = 0;
private:
    inline void Delete()
    {
        if(m_GpuImage != 0)
            glDeleteTextures(1, &m_GpuImage);
        delete[] m_Pixel;
        m_Width = 0;
        m_Height = 0;
        m_GpuImage = 0;
        m_Pixel = nullptr;
    }

    inline void CreateGpuImage()
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // Create a OpenGL texture identifier
        glGenTextures(1, &m_GpuImage);
        glBindTexture(GL_TEXTURE_2D, m_GpuImage);

        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, m_Pixel);
    }

    inline void UpdateGpu() const
    {
        glBindTexture(GL_TEXTURE_2D, m_GpuImage);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, GL_RGB, GL_UNSIGNED_BYTE, m_Pixel);
    }
public:
    inline ~Image()
    {
        Delete();
    }

    inline void Assign(const qrcodegen::QrCode& qr, int32_t border, size_t scale, float colorPrimary[3], float colorSecondary[3])
    {
        assert(scale != 0 && "Image::Assign scale can't be less than 1");

        bool resized = false;
        const GLsizei wh = (GLsizei)((qr.getSize() + border * 2) * scale);
        if (wh != m_Width)
        {
            GLubyte* tmp = new (std::nothrow) GLubyte[NumOfChannels * wh * wh];
            if (tmp == nullptr)
            {
                Err(Local::Get(Local::Item::ErrImageOutOfMemory), (unsigned int)(NumOfChannels * wh * wh), (unsigned int)(NumOfChannels * wh * wh) / std::pow(10, 9));
                return;
            }

            Delete();
            m_Width = wh;
            m_Height = wh;
            m_Pixel = tmp;
            resized = true;
        }


        size_t idx = 0;
        for (int32_t y = -border; y < qr.getSize() + border; y++)
        {
            for (int32_t x = -border; x < qr.getSize() + border; x++)
            {
                const bool dark = qr.getModule(x, y);
                for (size_t i = 0; i < scale; ++i)
                {
                    m_Pixel[idx++] = dark ? static_cast<GLubyte>(colorPrimary[0] * 255.0) : static_cast<GLubyte>(colorSecondary[0] * 255.0);
                    m_Pixel[idx++] = dark ? static_cast<GLubyte>(colorPrimary[1] * 255.0) : static_cast<GLubyte>(colorSecondary[1] * 255.0);
                    m_Pixel[idx++] = dark ? static_cast<GLubyte>(colorPrimary[2] * 255.0) : static_cast<GLubyte>(colorSecondary[2] * 255.0);
                }
            }
            
            // copy row scale-1 times
            for (size_t i = 0; i < scale - 1; ++i)
            {
                std::memcpy(&m_Pixel[idx], &m_Pixel[idx - m_Width * NumOfChannels], m_Width * NumOfChannels);
                idx += NumOfChannels * m_Width;
            }
        }
        if (resized)
            CreateGpuImage();
        else
            UpdateGpu();
    }

    inline std::vector<uint32_t>& Data32() const
    {
        static std::vector<uint32_t> pixel;
        pixel.clear();
        pixel.reserve(m_Width * m_Height);

        for (GLuint i = 0; i < m_Width * m_Height * NumOfChannels;)
        {
            pixel.push_back(((uint32_t)0xFF << 24) | ((uint32_t)m_Pixel[i++] << 16) | ((uint32_t)m_Pixel[i++] << 8) | ((uint32_t)m_Pixel[i++] << 0));
        }
        return pixel;
    }

    inline GLuint GetGpuImage() const { return m_GpuImage; }
    inline int Height() const { return m_Height; }
    inline int Width() const { return m_Width; }
    inline unsigned char* Data() const { return m_Pixel; }
};