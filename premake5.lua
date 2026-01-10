workspace "VkGameEngine"
    architecture "x64"
    configurations {"Debug", "Release"}
    startproject "Editor"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Core"
include "Editor"
include "Core/Vendor/GLFW"
