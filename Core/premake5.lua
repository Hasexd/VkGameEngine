project "Core"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    staticruntime "on"

    targetdir("../bin/" .. outputdir .. "/%{prj.name}")
    objdir("../bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "Headers/**.h",
        "Sources/**.cpp",
        "Shaders/**.frag",
        "Shaders/**.vert",
        "Shaders/**.comp",
        "Shaders/**.geom"
    }

    links {
        "GLFW",
        "VkBootstrap"
    }

    local shaderPath = path.getabsolute("Shaders")
    local objPath = path.getabsolute("Objs");

    defines {
        "PATH_TO_SHADERS=\"" .. shaderPath .. "\"",
    }

    includedirs {
        "Headers",
        "Vendor/glfw/include",
        "Vendor/VulkanMemoryAllocator/include",
        "Vendor/vk-bootstrap/include",
        "Vendor/glm",
        vkSDK .. "/Include"
    }

    vpaths {
        ["Header Files"] = "Headers/**.h",
        ["Source Files"] = "Sources/**.cpp",
    }

    filter "system:windows"
        links { vkSDK .. "/Lib/vulkan-1.lib" }
        prebuildcommands {
            "cd %{prj.location} && call Scripts/compile_shaders.bat"
        }
    
    filter "system:linux"
        links { "vulkan" }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        defines { "DEBUG" }

    filter "configurations:Release"
        runtime "Release"
        optimize "on"