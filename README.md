# EdgePixelizer-SFML

A desktop application built with C++, SFML, OpenCV, and Dear ImGui that converts images into pixelated edge art. It provides real-time previews and allows adjustment of various image processing parameters.


![Application Screenshot](https://raw.githubusercontent.com/Piotrixek/EdgePixelizer-SFML/main/image.png) 

## Features

* Load images (common formats supported by SFML/OpenCV).
* Real-time preview of the original and processed (pixelated edge) image.
* Adjustable parameters with immediate visual feedback:
    * Input image scaling.
    * Output pixel block size (controls blockiness).
    * Brightness and Contrast adjustment (pre-processing).
    * Optional Gaussian Blur (pre-processing) with adjustable kernel size.
    * Canny edge detection thresholds (Low & High) to control edge sensitivity.
    * Vertical and Horizontal image flipping.
* Generate coordinate lists of the resulting 'on' pixel blocks.
* Selectable output formats for coordinates:
    * C# `List<(int x, int y)>`
    * JavaScript Array `[[x, y], ...]`
    * Python List `[(x, y), ...]`
* Copy generated code to clipboard.
* Reset settings to default values.

## Dependencies

You need the following libraries installed/downloaded to build this project:

1.  **SFML >= 3.0.0:** (Simple and Fast Multimedia Library) Used for windowing, graphics context, and events.
    * Download: [https://www.sfml-dev.org/download.php](https://www.sfml-dev.org/download.php) (Get the pre-compiled libraries for your compiler, e.g., Visual C++ 17 2022 - 64-bit)
2.  **OpenCV >= 4.x:** (Open Source Computer Vision Library) Used for image loading and processing (Canny, resize, blur, etc.).
    * Download: [https://opencv.org/releases/](https://opencv.org/releases/) (Get the Windows release, run the installer to extract files).
3.  **Dear ImGui (latest):** Used for creating the graphical user interface (controls, windows).
    * Download/Clone: [https://github.com/ocornut/imgui](https://github.com/ocornut/imgui) (You'll compile this *with* the project).
4.  **ImGui-SFML (latest compatible with SFML 3.0):** Binding to make Dear ImGui work with SFML.
    * Download/Clone: [https://github.com/eliasdaler/imgui-sfml](https://github.com/eliasdaler/imgui-sfml) (Check for SFML 3.0 compatibility or necessary branches/versions).
5.  **CMake >= 3.15:** Used to configure the project and generate build files (e.g., Visual Studio solution).
    * Download: [https://cmake.org/download/](https://cmake.org/download/)
6.  **C++ Compiler:** A compiler supporting C++17 (e.g., Visual Studio 2022 Community Edition).

## Setup & Building (using CMake and Visual Studio 2022)

These instructions assume you have downloaded the dependencies and placed them in a known location (e.g., `C:/libs/`).
1.  **Configure CMake:**
    * Open `CMakeLists.txt` in a text editor.
    * **Crucially, update the paths** near the top to point to *your* extracted library locations:
        ```cmake
        set(SFML_DIR "C:/libs/SFML-3.0.0/lib/cmake/SFML" CACHE PATH ...)
        set(OpenCV_DIR "C:/libs/opencv/build" CACHE PATH ...) # Point to the dir containing OpenCVConfig.cmake
        set(IMGUI_DIR "C:/libs/imgui" CACHE PATH ...)
        set(IMGUI_SFML_DIR "C:/libs/imgui-sfml" CACHE PATH ...)
        ```
        *(Adjust `SFML-3.0.0` and the OpenCV path/version as needed)*.
    * Open a command prompt or terminal in the project's root directory (where `CMakeLists.txt` is).
    * Create and navigate to a build directory:
        ```bash
        mkdir build
        cd build
        ```
    * Run CMake to generate the Visual Studio solution (adjust generator if needed):
        ```bash
        cmake .. -G "Visual Studio 17 2022" -A x64
        ```
2.  **Open in Visual Studio:**
    * Navigate into the `build` directory.
    * Open the generated `.sln` file (e.g., `EdgePixelConverter.sln`) with Visual Studio 2022.
3.  **Build the Project:**
    * Select the desired configuration (`Debug` or `Release`) and platform (`x64`) at the top of Visual Studio.
    * Set the main project (`EdgePixelConverter`) as the Startup Project (right-click -> Set as Startup Project).
    * Build the solution (Build -> Build Solution).
4.  **Run:**
    * Run the executable from Visual Studio (`Ctrl+F5` or Debug -> Start Without Debugging). The DLLs should be copied automatically, allowing the program to start.

## Usage

1.  Run the executable.
2.  Enter the full path to an image file in the "Image Path" box and click "Load" or press Enter.
3.  Adjust the sliders and checkboxes in the "Controls" panel. The processed preview will update in near real-time.
4.  Select the desired output format for the coordinates.
5.  Click "Copy Code" to copy the generated coordinates to the clipboard.
6.  Click "Reset Settings" to revert controls to their default values.

## License

MIT, Apache 2.0

