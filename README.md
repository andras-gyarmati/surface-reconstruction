# 3D Surface Reconstruction from Point Clouds using LiDAR Sensor Data

![program_mesh_solid](https://github.com/andras-gyarmati/surface-reconstruction/assets/3900156/c9328582-418a-4ba9-8d3c-cb61970852de)

This repository focuses on the creation of 3D surfaces from point clouds captured by a university-used LiDAR sensor. For coloring the surfaces, images from cameras positioned alongside the sensor are used.

## Files in Repository

- **[OGLPack.zip](OGLPack.zip)**: Required OGLPack package for setting up the development environment.
- **[Szakdolgozat_UY18CX.pdf](Szakdolgozat_UY18CX.pdf)**: The PDF of the thesis containing detailed explanations, algorithms, and figures.

## Overview

The point clouds and images are loaded from files and prepared for rendering. After loading, OpenGL natively supports point rendering. For surface drawing, the points are connected through indexed triangles.

Additionally, the structure of the points in an octree can be displayed, as well as a visualization of a portion of the Delaunay tetrahedralization.

## Technologies and Methods

The application is built with C++17 and utilizes SDL (Simple DirectMedia Layer) for window display and user interaction management. The loading of OpenGL extensions is managed by GLEW (OpenGL Extension Wrangler). Inside the SDL window, OpenGL 4.6 is used for rendering graphical elements which include both the user interface and graphically displayed sensor data.

The Graphical User Interface (GUI) is implemented using the ImGui package.

## Installation Guide

The program can be run within the Visual Studio development environment and requires the OGLPack package to be placed in the system root directory. The [package](OGLPack.zip) can be obtained from the repository or ELTE's graphics course webpage at [http://cg.elte.hu/~bsc_cg/resources/OGLPack.zip](http://cg.elte.hu/~bsc_cg/resources/OGLPack.zip). Afterwards, the drive must be virtually cloned using the `subst t: c:\` command for easy setup of external library references in Visual Studio.
