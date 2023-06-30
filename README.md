# Swiftcanon
Rendering Engine

Prerequisites:
- Git Client
- CMake 3.14 or greater
- Visual Studio / VSCode / XCode / GCC
- VulkanSDK

`NOTE: On MacOS, run: source ~/VulkanSDK/<VulkanVersion>/setup-env.sh`

`git clone --recursive https://github.com/RutvikPanchal/Swiftcanon.git`

## VSCode Intellisense config
- Install C/C++ VSCode Extension
- Run ./_package.sh
- Reload VSCode Window and Click on Yes on the C++ Extension notification that pops up asking to configure using /build/compile_commands.json

## Windows:

```
Step 1: cmake -S . -B ./build
Step 2: Open the solution file in build folder in Visual Studio and the project is ready to run
```

## Linux / MacOS:

```
Step 1: ./_package.sh
Step 2: ./build/Swiftcanon
```