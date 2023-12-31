﻿#include <new>
#include <string>
#include <cstring>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <algorithm>

#include "ImGui/imgui.h"
#include "ImGui/imgui_stdlib.h"
#include "qrcodegen/cpp/qrcodegen.hpp"
#include "clip/clip.h"

#include "Log.h"
#include "Clang.h"
#include "Image.h"
#include "ImageWriter.h"
#include "RenderWindow.h"
#include "Localization.h"


inline int Utf8CharSize(const char* utf8Char)
{
    // The most significant bits of the first byte determine the number of bytes in the character
    if ((utf8Char[0] & 0b10000000) == 0) {
        return 1; // Single-byte character
    }
    else if ((utf8Char[0] & 0b11100000) == 0b11000000) {
        return 2; // Two-byte character
    }
    else if ((utf8Char[0] & 0b11110000) == 0b11100000) {
        return 3; // Three-byte character
    }
    else if ((utf8Char[0] & 0b11111000) == 0b11110000) {
        return 4; // Four-byte character
    }
    else {
        // Invalid UTF-8 encoding
        return 0;
    }
}


inline size_t Utf8PreviousCodePoint(const char* str, size_t index)
{
    assert(index > 0);
    // Move backward until the start of the current character is found
    while ((str[--index] & 0xC0) == 0x80);

    // The current index is now the start of the previous character
    return index;
}


inline void Application()
{
    Image img;
    bool rerender = true;
    bool qrContentChanged = true;
    Local::SetLanguage(Local::Language::English);
    RenderWindow window(1200, 760, "QR-Code-Generator");
    float yPosCursor = ImGui::GetStyle().WindowPadding.y;

    while (window.IsOpen())
    {
        window.StartFrame();
        ImVec2 newPos = window.Pos();
        const float windowWidth = window.Size().x;
        const float windowHeight = window.Size().y;

        if (g_WindowResized)
        {
            yPosCursor = ImGui::GetStyle().WindowPadding.y;
            g_WindowResized = false;
        }

        {
            ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
            ImGui::SetNextWindowSize(ImVec2(windowWidth, -1));
            ImGui::SetNextWindowPos(newPos);
            ImGui::Begin("##MenuBarWindow", nullptr, ImGuiWindowFlags_MenuBar | IMGUI_WINDOW_FLAGS);

            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu(Local::Get(Local::Item::LanguageSelectionMenu)))
                {
                    if (ImGui::MenuItem(Local::NameAsString(Local::Language::English), nullptr, Local::GetLanguage() == Local::Language::English)) Local::SetLanguage(Local::Language::English);
                    if (ImGui::MenuItem(Local::NameAsString(Local::Language::German), nullptr, Local::GetLanguage() == Local::Language::German)) Local::SetLanguage(Local::Language::German);
                    if (ImGui::MenuItem(Local::NameAsString(Local::Language::Hebrew), nullptr, Local::GetLanguage() == Local::Language::Hebrew)) Local::SetLanguage(Local::Language::Hebrew);
                    if (ImGui::MenuItem(Local::NameAsString(Local::Language::Arabic), nullptr, Local::GetLanguage() == Local::Language::Arabic)) Local::SetLanguage(Local::Language::Arabic);
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            newPos.y += ImGui::GetWindowHeight();
            ImGui::End();
        }

        {
            ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
            ImGui::SetNextWindowSize({ windowWidth / 2, windowHeight });
            ImGui::SetNextWindowPos(newPos);
            ImGui::Begin("InputWindow", nullptr, IMGUI_WINDOW_FLAGS);

            static const auto rightBoundTextCallback = [](ImGuiInputTextCallbackData* data)
            {
                static int prevBufLen = 0;
                static int prevCursorPos = 0;
                static int prevSelectionRange = data->SelectionEnd - data->SelectionStart;
                if (prevBufLen != data->BufTextLen && prevSelectionRange == 0)
                {
                    data->CursorPos = prevCursorPos;
                }
                prevCursorPos = data->CursorPos;
                prevBufLen = data->BufTextLen;

                static std::string prevContent(data->Buf, data->BufTextLen);

                if (ImGui::IsKeyPressed(ImGuiKey_Backspace) && prevSelectionRange == 0)
                {
                    const int charSize = Utf8CharSize(prevContent.data());
                    prevContent.erase(data->CursorPos, charSize);
                    std::memcpy(data->Buf, prevContent.data(), data->BufSize - charSize);
                    data->BufTextLen = (int)prevContent.size();
                    data->BufDirty = true;
                }

                if (ImGui::IsKeyPressed(ImGuiKey_Delete) && prevSelectionRange == 0)
                {
                    int charSize = 0;
                    size_t prevChar = 0;
                    if (data->CursorPos > 0)
                    {
                        prevChar = Utf8PreviousCodePoint(prevContent.c_str(), data->CursorPos);
                        charSize = Utf8CharSize(&prevContent.c_str()[prevChar]);
                        prevContent.erase(prevChar, charSize);
                    }
                    std::memcpy(data->Buf, prevContent.data(), data->BufSize - charSize);
                    data->BufTextLen = (int)prevContent.size();
                    data->BufDirty = true;
                    data->CursorPos = (int)prevChar;
                }
                prevSelectionRange = data->SelectionEnd - data->SelectionStart;
                prevContent = std::string(data->Buf, data->BufTextLen);
                return 0;
            };

            static std::string s;
            ImGui::SetCursorPosY(yPosCursor);
            qrContentChanged = ImGui::InputTextWithHint("##ContentInputText", Local::Get(Local::Item::QRInputField), &s, Local::IsFromLeftToRight() ? 0 : ImGuiInputTextFlags_CallbackAlways, rightBoundTextCallback) || qrContentChanged;

            static int eccLevel = 0;
            qrContentChanged = ImGui::Combo(Local::Get(Local::Item::QRErrorCorrection), &eccLevel, Local::Get(Local::Item::QRErrorCorrectionCombo)) || qrContentChanged;
        
            static bool boostEccl = true;
            qrContentChanged = ImGui::Checkbox(Local::Get(Local::Item::QRIncreaseErrorCorrection), &boostEccl) || qrContentChanged;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(Local::Get(Local::Item::QRIncreaseErrorCorrectionTooltip));
        
            static int borderSize = 3;
            rerender = ImGui::InputInt(Local::Get(Local::Item::QRBorder), &borderSize, 1, 10, ImGuiInputTextFlags_CharsDecimal) || rerender;
            borderSize = std::max(0, borderSize);
        
            static int scale = 30;
            rerender = ImGui::InputInt(Local::Get(Local::Item::QRResolution), &scale, 1, 10, ImGuiInputTextFlags_CharsDecimal) || rerender;
            scale = std::max(1, scale);
        
            ImGui::SameLine();
            ImGui::Text("(%ux%u)", img.Width(), img.Height());
        
            static float colorPrimary[3] = { 0 };
            rerender = ImGui::ColorEdit3(Local::Get(Local::Item::QRPrimaryColor), colorPrimary, ImGuiColorEditFlags_DisplayHex) || rerender;
        
            static float colorSecondary[3] = { 1, 1, 1 };
            rerender = ImGui::ColorEdit3(Local::Get(Local::Item::QRSecondaryColor), colorSecondary, ImGuiColorEditFlags_DisplayHex) || rerender;
        
            static int minVersion = qrcodegen::QrCode::MIN_VERSION;
            static int maxVersion = qrcodegen::QrCode::MAX_VERSION;
            qrContentChanged = ImGui::InputInt(Local::Get(Local::Item::QRMinVersion), &minVersion, 1, 10, ImGuiInputTextFlags_CharsDecimal) || qrContentChanged;
            qrContentChanged = ImGui::InputInt(Local::Get(Local::Item::QRMaxVersion), &maxVersion, 1, 10, ImGuiInputTextFlags_CharsDecimal) || qrContentChanged;
            minVersion = std::clamp(minVersion, qrcodegen::QrCode::MIN_VERSION, std::max(maxVersion, qrcodegen::QrCode::MIN_VERSION));
            maxVersion = std::clamp(maxVersion, minVersion, qrcodegen::QrCode::MAX_VERSION);
        
            static int maskPattern = -1;
            qrContentChanged = ImGui::InputInt(Local::Get(Local::Item::QRMaskPattern), &maskPattern, 1, 10, ImGuiInputTextFlags_CharsDecimal) || qrContentChanged;
            maskPattern = std::clamp(maskPattern, -1, 7);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(Local::Get(Local::Item::QRMaskPatternTooltip));
        
            if (rerender || qrContentChanged)
            {
                static qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(s.c_str(), (qrcodegen::QrCode::Ecc)eccLevel);
                if (qrContentChanged)
                {
                    try
                    {
                        const std::vector<qrcodegen::QrSegment> qrSegments = qrcodegen::QrSegment::makeSegments(s.c_str());
                        qr = qrcodegen::QrCode::encodeSegments(qrSegments, (qrcodegen::QrCode::Ecc)eccLevel, minVersion, maxVersion, maskPattern, boostEccl);
                    }
                    catch (const std::length_error& e)
                    {
                        Err(Local::Get(Local::Item::ErrFailedToGenerateQRCode), e.what());
                    }
                }
                img.Assign(qr, borderSize, scale, colorPrimary, colorSecondary);
                rerender = false;
                qrContentChanged = false;
            }
        
            newPos.x += ImGui::GetWindowWidth();
            if (yPosCursor == ImGui::GetStyle().WindowPadding.y)
                yPosCursor = (windowHeight / 2 - ImGui::GetCursorPosY() / 2);
            ImGui::End();
        }
        
        {
            ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
            ImGui::SetNextWindowSize({ windowWidth / 2, windowHeight });
            ImGui::SetNextWindowPos(newPos);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
            ImGui::Begin("QRCodeImage", nullptr, IMGUI_WINDOW_FLAGS);
        
            const float imgSize = std::min(ImGui::GetWindowHeight(), ImGui::GetWindowWidth()) * 0.65f;
            const float xPos = (ImGui::GetWindowWidth() - imgSize) / 2.f;
            const float yPos = (ImGui::GetWindowHeight() - imgSize) / 2.f;
            ImGui::SetCursorPos(ImVec2(xPos, yPos));
            if (ImGui::ImageButton((void*)(intptr_t)img.GetGpuImage(), { imgSize, imgSize }))
            {
                clip::image_spec spec;
                spec.width = img.Width();
                spec.height = img.Height();
                spec.bits_per_pixel = 8 * 4;
                spec.bytes_per_row = spec.width * 4;
                spec.red_mask = 0xff;
                spec.green_mask = 0xff00;
                spec.blue_mask = 0xff0000;
                spec.alpha_mask = 0xff000000;
                spec.red_shift = 0;
                spec.green_shift = 8;
                spec.blue_shift = 16;
                spec.alpha_shift = 24;
                try { clip::image cimg(img.Data32().data(), spec);  clip::set_image(cimg); }
                catch (const std::bad_alloc& e) { Err(Local::Get(Local::Item::ErrFailedToCopyImageToClipboard), e.what()); }
                catch (const std::runtime_error& e) { Err(Local::Get(Local::Item::ErrFailedToCopyImageToClipboard), e.what()); }
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip(Local::Get(Local::Item::QRButtonTooltip));
            ImGui::SetCursorPosX(xPos);
            if (ImGui::Button(Local::Get(Local::Item::QRSaveButton), ImVec2(ImGui::GetItemRectSize().x, 28)) || (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyDown(ImGuiKey_S))) SaveImage(img);
            ImGui::End();
            ImGui::PopStyleVar();
        }

        window.EndFrame();
    }
}


int main()
{
    bool keepRunning = true;
    while (keepRunning)
    {
        try
        {
            Application();
            keepRunning = false;
        }
        catch (const std::bad_alloc&)
        {
            keepRunning = tinyfd_messageBox("Error", Local::Get(Local::Item::ErrApplicationBadAllocException), "yesno", "error", 1);
        }
        catch (const std::exception& e)
        {
            const std::string m = Local::Get(Local::Item::ErrApplicationUnhandledStdException) + std::string(e.what());
            keepRunning = tinyfd_messageBox("Error", m.c_str(), "yesno", "error", 1);
        }
        catch (...)
        {
            keepRunning = tinyfd_messageBox("Error", Local::Get(Local::Item::ErrApplicationUnhandledException), "yesno", "error", 1);
        }
    }
    return 0;
}