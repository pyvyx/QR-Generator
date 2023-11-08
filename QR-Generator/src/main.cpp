﻿#include <iostream>
#include <algorithm>

#include "ImGui/imgui.h"
#include "ImGui/imgui_stdlib.h"
#include "qrcodegen/cpp/qrcodegen.hpp"

#include "Log.h"
#include "Clang.h"
#include "Image.h"
#include "RenderWindow.h"


int main()
{
    Image img;
    RenderWindow window;
    
    while (window.IsOpen())
    {
        window.StartFrame();
        ImVec2 newPos = window.Pos();
        const float windowWidth = window.Size().x;
        const float windowHeight = window.Size().y;

        {
            ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
            ImGui::SetNextWindowSize({ windowWidth/2, windowHeight });
            ImGui::SetNextWindowPos(newPos);

            ImGui::Begin("InputWindow", nullptr, IMGUI_WINDOW_FLAGS);
            static bool rerender = true;
            static bool qrContentChanged = true;

            static std::string s;
            qrContentChanged = ImGui::InputTextWithHint("##ContentInputText", (const char*)u8"Text einfügen", &s) || qrContentChanged;

            static int eccLevel = 0;
            qrContentChanged = ImGui::Combo("Fehlerkorrektur", &eccLevel, "Niedrig\0Mittel\0Quartil\0Hoch\0") || qrContentChanged;

            static bool boostEccl = true;
            qrContentChanged = ImGui::Checkbox((const char*)u8"Erhöhe das Fehlerkorrekturlevel automatisch", &boostEccl) || qrContentChanged;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip((const char*)u8"Es wird automatisch die kleinstmögliche QR-Code-Version innerhalb des angegebenen Bereichs verwendet.\nWenn aktiviert wird das Fehlerkorrekturlevel erhöht sofern dies ohne Erhöhung der Version möglich ist");

            static int borderSize = 3;
            rerender = ImGui::InputInt("Rand", &borderSize, 1, 10, ImGuiInputTextFlags_CharsDecimal) || rerender;
            borderSize = std::clamp(borderSize, 0, 30);

            static int scale = 30;
            rerender = ImGui::InputInt("Skalierung", &scale, 1, 10, ImGuiInputTextFlags_CharsDecimal) || rerender;
            scale = std::clamp(scale, 1, 300);

            ImGui::SameLine();
            ImGui::Text("(%ux%u)", img.Width(), img.Height());

            static float colorPrimary[3] = { 0 };
            rerender = ImGui::ColorEdit3((const char*)u8"Primärfarbe", colorPrimary, ImGuiColorEditFlags_DisplayHex) || rerender;

            static float colorSecondary[3] = { 1, 1, 1 };
            rerender = ImGui::ColorEdit3((const char*)u8"Sekundärfarbe", colorSecondary, ImGuiColorEditFlags_DisplayHex) || rerender;

            static int minVersion = 1;
            static int maxVersion = 40;
            qrContentChanged = ImGui::InputInt("Mindest Version", &minVersion, 1, 10, ImGuiInputTextFlags_CharsDecimal) || qrContentChanged;
            qrContentChanged = ImGui::InputInt("Maximal Version", &maxVersion, 1, 10, ImGuiInputTextFlags_CharsDecimal) || qrContentChanged;
            minVersion = std::clamp(minVersion, 1, maxVersion);
            maxVersion = std::clamp(maxVersion, minVersion, 40);

            static int maskPattern = -1;
            qrContentChanged = ImGui::InputInt("Maske", &maskPattern, 1, 10, ImGuiInputTextFlags_CharsDecimal) || qrContentChanged;
            maskPattern = std::clamp(maskPattern, -1, 7);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("(-1 automatisch, 0 bis 7 manuell)");

            if (rerender || qrContentChanged)
            {
                static qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(s.c_str(), (qrcodegen::QrCode::Ecc)eccLevel);
                if (qrContentChanged)
                {
                    const std::vector<qrcodegen::QrSegment> qrSegments = qrcodegen::QrSegment::makeSegments(s.c_str());
                    qr = qrcodegen::QrCode::encodeSegments(qrSegments, (qrcodegen::QrCode::Ecc)eccLevel, minVersion, maxVersion, maskPattern, boostEccl);
                }
                img.Assign(qr, borderSize, scale, colorPrimary, colorSecondary);
                rerender = false;
                qrContentChanged = false;
            }

            newPos.x += ImGui::GetWindowWidth();
            ImGui::End();
        }

        {
            ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
            ImGui::SetNextWindowSize({ windowWidth / 2, windowHeight });
            ImGui::SetNextWindowPos(newPos);
            ImGui::Begin("QRCodeImage", nullptr, IMGUI_WINDOW_FLAGS);
        
            const float imgSize = std::min(ImGui::GetWindowHeight(), ImGui::GetWindowWidth()) - 2*std::max(ImGui::GetStyle().WindowPadding.x, ImGui::GetStyle().WindowPadding.y);
            const float xPos = (ImGui::GetWindowWidth() - imgSize) / 2.f;
            const float yPos = (ImGui::GetWindowHeight() - imgSize) / 2.f;
            const ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::SetCursorScreenPos(ImVec2(p.x + xPos, p.y + yPos));
            ImGui::Image((void*)(intptr_t)img.GetGpuImage(), { imgSize, imgSize });
            ImGui::End();
        }

        window.EndFrame();
    }
    return 0;
}