# 3D Surface Reconstruction from Point Clouds using LiDAR Sensor Data

![program_mesh_solid](https://github.com/andras-gyarmati/surface-reconstruction/assets/3900156/c9328582-418a-4ba9-8d3c-cb61970852de)

This repository focuses on the creation of 3D surfaces from point clouds captured by a university-used LiDAR sensor. For coloring the surfaces, images from cameras positioned alongside the sensor are used.

## Files in Repository

- **[Szakdolgozat_UY18CX.pdf](Szakdolgozat_UY18CX.pdf)**: The PDF of the thesis containing detailed explanations, algorithms, and figures.

## Overview

The point clouds and images are loaded from files and prepared for rendering. After loading, OpenGL natively supports point rendering. For surface drawing, the points are connected through indexed triangles.

Additionally, the structure of the points in an octree can be displayed, as well as a visualization of a portion of the Delaunay tetrahedralization.

## Technologies and Methods

The application is built with C++17 and utilizes SDL (Simple DirectMedia Layer) for window display and user interaction management. The loading of OpenGL extensions is managed by GLEW (OpenGL Extension Wrangler). Inside the SDL window, OpenGL (4.1 core on macOS, the newest the driver offers elsewhere) is used for rendering graphical elements which include both the user interface and graphically displayed sensor data.

The Graphical User Interface (GUI) is implemented using the ImGui package.

## Building

Needs CMake 3.24+, a C++17 compiler and git. CMake downloads SDL2, SDL2_image, GLEW, glm and Dear ImGui on the first configure, so nothing has to be installed by hand. Works on macOS, Linux and Windows.

```bash
cmake -S . -B build
cmake --build build --config Release
```

The executable is `build/SurfaceReconstruction` (`build/Release/SurfaceReconstruction.exe` with Visual Studio), with `shaders/` and `inputs/` copied next to it. Visual Studio, CLion and VS Code open the folder directly as a CMake project.

The thesis version was built in Visual Studio against ELTE's OGLPack on a `subst t: c:\` drive; that setup is in the git history before the CMake build.
