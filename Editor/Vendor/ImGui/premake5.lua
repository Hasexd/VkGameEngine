project "ImGui"
    kind "StaticLib"
    language "C++"
    cppdialect "C++23"
    staticruntime "on"

    targetdir ("../../../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../../../bin-int/" .. outputdir .. "/%{prj.name}")

    files 
    {
        "./*.cpp",
        "./*.h"
    }

    includedirs
    {
        "./",
        "../../../Core/Vendor/glfw/include",
        vkSDK .. "/Include"
    }
    
    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        defines { "DEBUG" }

    filter "configurations:Release"
        runtime "Release"
        optimize "on"