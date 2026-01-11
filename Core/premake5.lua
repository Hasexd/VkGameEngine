project "Core"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    staticruntime "on"

    targetdir("../bin/" .. outputdir .. "/%{prj.name}")
    objdir("../bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "Headers/**.h",
        "Sources/**.cpp"
    }

    links {
        "GLFW",
        "VkBootstrap"
    }

    local shaderPath = path.getabsolute("Shaders")

    defines {
        "PATH_TO_SHADERS=\"" .. shaderPath .. "\""
    }

    includedirs {
        "Headers",
        "Vendor/glfw/include",
        "Vendor/VulkanMemoryAllocator/include",
        "Vendor/vk-bootstrap/include",
        vkSDK .. "/Include"
    }

    vpaths {
        ["Header Files"] = "Headers/**.h",
        ["Source Files"] = "Sources/**.cpp"
    }

    filter "system:windows"
        links { vkSDK .. "/Lib/vulkan-1.lib" }
    
    filter "system:linux"
        links { "vulkan" }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        defines { "DEBUG" }

    filter "configurations:Release"
        runtime "Release"
        optimize "on"