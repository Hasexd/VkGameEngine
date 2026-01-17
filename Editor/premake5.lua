project "Editor"
    kind "ConsoleApp"
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

    local shaderPath = path.getabsolute("Shaders")

    defines {
        "PATH_TO_SHADERS=\"" .. shaderPath .. "\"",
        "PATH_TO_EDITOR=\"" .. rootPath .. "/Editor" .. "\"",
    }

    links {
        "Core",
        "ImGui",
        "GLFW",
        "VkBootstrap"
    }

    includedirs {
        "Headers",
        "Vendor/ImGui",
        "../Core/Headers",
        "../Core/Vendor/glfw/include",
        "../Core/Vendor/VulkanMemoryAllocator/include",
        "../Core/Vendor/vk-bootstrap/include",
        "../Core/Vendor/glm",
        vkSDK .. "/Include"
    }

    vpaths {
        ["Header Files"] = "Headers/**.h",
        ["Source Files"] = "Sources/**.cpp"
    }

    filter "system:windows"
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