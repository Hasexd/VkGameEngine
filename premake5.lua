workspace "VkGameEngine"
    architecture "x64"
    configurations {"Debug", "Release"}
    startproject "Editor"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
vkSDK = os.getenv("VULKAN_SDK")

include "Core"
include "Editor"
include "Core/Vendor/GLFW"
include "Core/Vendor/vk-bootstrap"
include "Editor/Vendor/ImGui"
