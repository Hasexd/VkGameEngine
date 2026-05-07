# 3D Game Engine

WORK IN PROGRESS. A Vulkan-based game engine written in C++23, with a separate editor application.

# Requirements

- C++23-capable compiler
- Vulkan SDK with `VULKAN_SDK` environment variable set

# Build

Open a terminal in the project's root directory, then run:

```bash
Premake/premake5 [action]
```
To generate appropriate build files. [List of all the premake actions](https://premake.github.io/docs/Using-Premake/).

For example, to generate a Visual Studio 2022 solution one would run:

```bash
Premake/premake5 vs2022
```
Then open the generated `.sln` file.  
Output is placed in `bin/<config>-<system>-x64/`

# Projects

When the Editor launches, the user will be prompted to open an existing project, press `Yes`, then navigate to the `/Demo` directory and choose the `Demo.vkproj` file to properly load the Demo project, as there is currently no way to create a project, this is the only way to test the Editor. 
