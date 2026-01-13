project "Editor"
    kind "ConsoleApp"
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
        "Core"
    }

    includedirs {
        "Headers",
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

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        defines { "DEBUG" }

    filter "configurations:Release"
        runtime "Release"
        optimize "on"