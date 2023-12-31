project "QR-Generator"
    language "C++"
    cppdialect "C++17"
    characterset "Unicode"

    files {
        "src/**.cpp",
        "src/**.h",
        "vendor/stb/**.h",
        "vendor/stb/**.cpp",
        "vendor/qrcodegen/cpp/qrcodegen.hpp",
        "vendor/qrcodegen/cpp/qrcodegen.cpp",
    }

    includedirs {
        "src",
        "vendor",
        "../Dependencies/clip/include",
        "../Dependencies/GLFW/include",
        "../Dependencies/ImGui/include",
        "../Dependencies/tinyfiledialogs/include",
        "../Dependencies/nativefiledialog-extended/src/include"
    }

    externalincludedirs {
        "vendor",
        "../Dependencies/ImGui/include"
    }

    flags "FatalWarnings"

    links {
        "glfw",
        "ImGui",
        "Clip",
        "tinyfiledialogs",
        "nativefiledialog-extended"
    }

    filter "system:windows"
        defines "_CRT_SECURE_NO_WARNINGS"
        links {
            "gdi32",
            "opengl32",
            "shell32",
            "Shlwapi",
            "ole32",
            "uuid",
            "comdlg32",
            "UxTheme",
            "Dwmapi"
        }

    filter "system:linux"
        links {
            "GL",
            "gdk_pixbuf-2.0",
            "gtk-3",
            "gdk-3",
            "glib-2.0",
            "gobject-2.0",
            "xcb",
            "png",
            "X11",
            "pangocairo-1.0",
            "pango-1.0",
            "harfbuzz",
            "cairo-gobject",
            "cairo",
            "z"
        }

    filter "system:macosx"
        defines "GL_SILENCE_DEPRECATION"
        linkoptions "-framework AppKit -framework iokit -framework OpenGl -framework UniformTypeIdentifiers"
        disablewarnings { "sign-conversion" }

    filter { "toolset:gcc* or toolset:clang*", "platforms:x86", "system:windows" }
        linkoptions "res/iconx86.res"

    filter { "toolset:gcc* or toolset:clang*", "platforms:x64", "system:windows" }
        linkoptions "res/icon.res"

    --gcc* clang* msc*
    filter "toolset:msc*"
        warnings "High"
        externalwarnings "Default"
        files "res/icon.rc"
        buildoptions { "/sdl" }
        disablewarnings "4706"

    filter { "toolset:gcc* or toolset:clang*" }
        enablewarnings {
            --"cast-align",
            --"cast-qual",
            --"ctor-dtor-privacy",
            "disabled-optimization",
            --"format=2",
            "init-self",
            "missing-include-dirs",
            "overloaded-virtual",
            "redundant-decls",
            "shadow",
            "sign-promo",
            --"switch-default",
            --"undef",
            "uninitialized",
            "unreachable-code",
            "unused",
            "alloca",
            --"conversion",
            "deprecated",
            --"format-security",
            --"null-dereference",
            "stack-protector",
            "vla",
            "shift-overflow"
        }
        disablewarnings { "unknown-warning-option", "format-security", "sequence-point", "invalid-source-encoding" }

    filter "toolset:gcc*"
        warnings "Extra"
        externalwarnings "Off"
        filter { "toolset:gcc*", "not system:macosx" }
            linkgroups "on" -- activate position independent linking
        filter "toolset:gcc*"
        enablewarnings {
            "noexcept",
            "strict-null-sentinel",
            "array-bounds=2",
            --"duplicated-branches",
            "duplicated-cond",
            "logical-op",
            "arith-conversion",
            "stringop-overflow=4",
            "implicit-fallthrough=3",
            "trampolines"
        }
        disablewarnings "cast-function-type"

    filter "toolset:clang*"
        warnings "Extra"
        externalwarnings "Everything"
        linkgroups "off"
        enablewarnings {
            "array-bounds",
            "long-long",
            --"implicit-fallthrough", 
        }
        disablewarnings {"cast-align", "sign-conversion" }
    filter {}


    filter { "configurations:Debug" }
        kind "ConsoleApp"
    filter { "configurations:Release" }
        kind "WindowedApp"
        entrypoint "mainCRTStartup"
