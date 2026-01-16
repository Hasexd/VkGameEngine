workspace "VkGameEngine"
    architecture "x64"
    configurations {"Debug", "Release"}
    startproject "Editor"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
vkSDK = os.getenv("VULKAN_SDK")

rootPath = os.getcwd()

include "Core"
include "Editor"
include (rootPath .. "/Core/Vendor/glfw")
include (rootPath .. "/Core/Vendor/vk-bootstrap")
include (rootPath .. "/Editor/Vendor/ImGui")
