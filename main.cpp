// C++ Code using SFML 3.0, OpenCV, and ImGui

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <opencv2/opencv.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include "tinyfiledialogs.h"

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <optional>
#include <variant>
#include <cstdint>

// Helper struct for processing parameters
struct ProcessingParams {
    float scale = 0.5f;
    int pixelSize = 10;
    int brightness = 0;
    float contrast = 1.0f;
    bool applyBlur = false;
    int blurKernel = 3;
    int cannyLow = 50;
    int cannyHigh = 100;
    bool flipV = false;
    bool flipH = false;
};

// --- Function Prototypes ---
bool loadImage(const std::string& filename, cv::Mat& outOriginalMat, sf::Texture& outOriginalTexture, sf::Image& outOriginalImage);
void processImage(const cv::Mat& originalMat, cv::Mat& outPixelArtMat, const ProcessingParams& params, std::vector<sf::Vector2i>& outBlockCoords);
std::string generateCode(const std::vector<sf::Vector2i>& coords, const std::string& format);
cv::Mat sfImageToCvMat(const sf::Image& image);
void ApplyModernStyle();


// --- Main Application ---
int main() {
    // --- SFML Window Setup ---
    sf::RenderWindow window(sf::VideoMode({ 1600, 900 }), "SFML Image to Edge-Pixel Art Converter");
    window.setFramerateLimit(60);

    // --- ImGui Setup ---
    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML" << std::endl;
        return -1;
    }

    ApplyModernStyle();

    // --- State Variables ---
    // char inputImagePath[256] = "";
    std::string currentImagePath = "";
    cv::Mat originalMat;
    cv::Mat pixelArtMat;
    sf::Texture originalTexture;
    sf::Texture processedTexture;
    sf::Image originalImage;
    bool imageLoaded = false;
    bool needsProcessing = false;
    ProcessingParams params;
    std::vector<sf::Vector2i> blockCoords;
    std::string generatedCodeStr = "// Load an image and process...";
    const char* outputFormats[] = { "C# List<(int x, int y)>", "JavaScript Array [[x, y], ...]", "Python List [(x, y), ...]" };
    int currentFormatIndex = 0;
    std::string codeFormatId = "csharp";
    sf::Clock deltaClock;

    // --- Main Loop ---
    while (window.isOpen()) {
        // --- Event Handling ---
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        // --- ImGui Frame Update ---
        ImGui::SFML::Update(window, deltaClock.restart());

        // --- GUI Layout (Using ImGui) ---
        ImGui::Begin("Controls");

        // --- File Input ---
        ImGui::Text("1. Load Image:");
        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            char const* lFilterPatterns[3] = { "*.png", "*.jpg", "*.bmp" };

            char const* selectedPathRaw = tinyfd_openFileDialog(
                "Select Image File", // Dialog title
                "",                  // Default path
                3,                   // Number of filter patterns
                lFilterPatterns,     // Filter patterns array
                "Image Files",       // Filter description
                0                    // Allow multiple selects? (0=no)
            );

            if (selectedPathRaw != NULL) {
                currentImagePath = selectedPathRaw;
                std::cout << "Selected image path: " << currentImagePath << std::endl;

                if (loadImage(currentImagePath, originalMat, originalTexture, originalImage)) {
                    imageLoaded = true; needsProcessing = true;
                    generatedCodeStr = "// Processing new image...";
                    blockCoords.clear(); pixelArtMat = cv::Mat(); processedTexture = sf::Texture();
                    std::cout << "Image loaded successfully." << std::endl;
                }
                else {
                    imageLoaded = false; std::cerr << "Failed to load image: " << currentImagePath << std::endl;
                    generatedCodeStr = "// Failed to load selected image";
                    blockCoords.clear(); pixelArtMat = cv::Mat(); processedTexture = sf::Texture();
                    tinyfd_messageBox("Error", "Failed to load the selected image file.", "ok", "error", 1); // Show error popup
                }
            }
            else {
                std::cout << "File selection cancelled." << std::endl;
            }
        }
        if (imageLoaded) {
            ImGui::TextWrapped("Loaded: %s", currentImagePath.c_str());
        }
        else {
            ImGui::TextDisabled("No image loaded.");
        }
        ImGui::Separator();

        bool changed = false;
        changed |= ImGui::SliderFloat("Input Scale", &params.scale, 0.1f, 2.0f, "%.2f");
        params.pixelSize = std::max(2, params.pixelSize);
        changed |= ImGui::SliderInt("Pixel Size", &params.pixelSize, 2, 50);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Min 2px for spacing");
        changed |= ImGui::SliderInt("Brightness", &params.brightness, -100, 100);
        changed |= ImGui::SliderFloat("Contrast", &params.contrast, 0.5f, 3.0f, "%.1f");
        ImGui::Separator();
        changed |= ImGui::Checkbox("Apply Gaussian Blur", &params.applyBlur);
        if (params.applyBlur) {
            if (params.blurKernel < 1) params.blurKernel = 1;
            if (params.blurKernel % 2 == 0) params.blurKernel++;
            changed |= ImGui::SliderInt("Blur Kernel", &params.blurKernel, 1, 15);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Must be odd");
        }
        ImGui::Separator();
        ImGui::Text("Canny Edge Thresholds");
        changed |= ImGui::SliderInt("Low##Canny", &params.cannyLow, 0, 250);
        changed |= ImGui::SliderInt("High##Canny", &params.cannyHigh, 0, 250);
        ImGui::Separator();
        changed |= ImGui::Checkbox("Flip Vertically", &params.flipV);
        changed |= ImGui::Checkbox("Flip Horizontally", &params.flipH);
        ImGui::Separator();
        ImGui::Text("Output Format");
        if (ImGui::Combo("##Format", &currentFormatIndex, outputFormats, IM_ARRAYSIZE(outputFormats))) {
            if (currentFormatIndex == 0) codeFormatId = "csharp";
            else if (currentFormatIndex == 1) codeFormatId = "js";
            else codeFormatId = "python";
            if (imageLoaded && !blockCoords.empty()) {
                generatedCodeStr = generateCode(blockCoords, codeFormatId);
            }
        }
        ImGui::Separator();
        if (ImGui::Button("Reset Settings")) {
            params = ProcessingParams(); currentFormatIndex = 0; codeFormatId = "csharp"; changed = true;
        }
        if (changed && imageLoaded) { needsProcessing = true; }
        ImGui::End();

        if (needsProcessing && imageLoaded) {
            std::cout << "Processing image..." << std::endl;
            if (originalMat.empty()) {
                std::cerr << "Error: Attempting to process an empty originalMat!" << std::endl;
                generatedCodeStr = "// Error: Original image data missing";
                needsProcessing = false;
            }
            else {
                processImage(originalMat, pixelArtMat, params, blockCoords);

                if (!pixelArtMat.empty()) {
                    cv::Mat rgbaProcessedMat;
                    cv::cvtColor(pixelArtMat, rgbaProcessedMat, cv::COLOR_GRAY2RGBA);

                    if (rgbaProcessedMat.empty() || !rgbaProcessedMat.ptr()) {
                        std::cerr << "Error: Failed to convert processed Mat to RGBA." << std::endl;
                        generatedCodeStr = "// Error: RGBA conversion failed";
                    }
                    else {
                        sf::Vector2u newSize = { static_cast<unsigned int>(rgbaProcessedMat.cols),
                                                 static_cast<unsigned int>(rgbaProcessedMat.rows) };

                        if (processedTexture.getSize() != newSize) {
                            std::cout << "Resizing processed texture to " << newSize.x << "x" << newSize.y << std::endl;
                            processedTexture = sf::Texture(newSize);
                            if (processedTexture.getSize() != newSize) {
                                std::cerr << "Failed to create/resize processed texture object." << std::endl;
                                generatedCodeStr = "// Failed texture creation";
                                goto skip_texture_update;
                            }
                        }

                        processedTexture.update(static_cast<const std::uint8_t*>(rgbaProcessedMat.ptr()), newSize, { 0u, 0u });
                        std::cout << "Processed texture updated." << std::endl;
                        generatedCodeStr = generateCode(blockCoords, codeFormatId);
                    }
                skip_texture_update:;
                }
                else {
                    std::cerr << "Processing failed, pixelArtMat is empty." << std::endl;
                    generatedCodeStr = "// Processing failed";
                    processedTexture = sf::Texture();
                    blockCoords.clear();
                }
                needsProcessing = false;
                std::cout << "Processing finished." << std::endl;
            }
        }

        ImGui::Begin("Previews & Output");
        const float availableWidth = ImGui::GetContentRegionAvail().x;
        const float originalPreviewWidth = availableWidth * 0.3f;
        const float processedPreviewWidth = availableWidth * 0.6f;
        const float previewHeight = std::min(originalPreviewWidth * 1.5f, processedPreviewWidth);

        ImGui::BeginChild("OriginalPreview", ImVec2(originalPreviewWidth, previewHeight + 30), true);
        ImGui::Text("Original");
        if (imageLoaded && originalTexture.getSize().x > 0) {
            ImGui::Image(originalTexture, sf::Vector2f(originalPreviewWidth, previewHeight));
        }
        else { ImGui::TextDisabled("No image loaded"); }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("ProcessedPreview", ImVec2(0, previewHeight + 30), true);
        ImGui::Text("Processed (Pixelated Edges)");
        if (imageLoaded && processedTexture.getSize().x > 0) {
            ImGui::Image(processedTexture, sf::Vector2f(ImGui::GetContentRegionAvail().x, previewHeight));
        }
        else if (imageLoaded) { ImGui::TextDisabled("Processing..."); }
        else { ImGui::TextDisabled("No image loaded"); }
        ImGui::EndChild();

        // Generated Code Area
        ImGui::Separator();
        ImGui::Text("Generated Coordinates (Edge Pixels)");
        ImGui::SameLine(ImGui::GetWindowWidth() - 80);
        if (ImGui::Button("Copy Code")) {
            if (!generatedCodeStr.empty()) { ImGui::SetClipboardText(generatedCodeStr.c_str()); }
            else { ImGui::SetClipboardText(""); }
        }
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
        ImGui::BeginChild("CodeScroll", ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 10), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextWrapped("%s", generatedCodeStr.empty() ? "// No code generated yet." : generatedCodeStr.c_str());
        ImGui::EndChild();
        ImGui::PopStyleVar();

        ImGui::End();


        window.clear(sf::Color(17, 24, 39));
        ImGui::SFML::Render(window);
        window.display();
    }

    // --- Cleanup ---
    ImGui::SFML::Shutdown();
    return 0;
}


// Helper to convert sf::Image to cv::Mat
cv::Mat sfImageToCvMat(const sf::Image& image) {
    sf::Vector2u size = image.getSize();
    if (size.x == 0 || size.y == 0) return cv::Mat();
    cv::Mat rgbaMat(size.y, size.x, CV_8UC4, const_cast<std::uint8_t*>(image.getPixelsPtr()));
    cv::Mat bgrMat;
    cv::cvtColor(rgbaMat, bgrMat, cv::COLOR_RGBA2BGR);
    return bgrMat.clone();
}

// Loads image using SFML, updates texture, and converts to cv::Mat
bool loadImage(const std::string& filename, cv::Mat& outOriginalMat, sf::Texture& outOriginalTexture, sf::Image& outOriginalImage) {
    if (!outOriginalImage.loadFromFile(filename)) {
        std::cerr << "Failed to load image from file: " << filename << std::endl; return false;
    }
    if (!outOriginalTexture.loadFromImage(outOriginalImage)) {
        std::cerr << "Failed to load texture from image" << std::endl; return false;
    }
    outOriginalMat = sfImageToCvMat(outOriginalImage);
    if (outOriginalMat.empty()) {
        std::cerr << "Failed to convert sf::Image to cv::Mat" << std::endl; return false;
    }
    return true;
}

// --- processImage function ---
void processImage(const cv::Mat& originalMat, cv::Mat& outPixelArtMat, const ProcessingParams& params, std::vector<sf::Vector2i>& outBlockCoords) {
    if (originalMat.empty()) { std::cerr << "processImage: Input originalMat is empty." << std::endl; return; }
    cv::Mat inputMat;
    if (originalMat.channels() == 4) { cv::cvtColor(originalMat, inputMat, cv::COLOR_BGRA2BGR); }
    else { originalMat.copyTo(inputMat); }

    cv::Mat adjustedMat, grayMat, edgeMat;
    outBlockCoords.clear();

    // 1. Clone and apply scale
    inputMat.copyTo(adjustedMat);
    if (params.scale != 1.0f) {
        cv::Size dsize(static_cast<int>(std::round(adjustedMat.cols * params.scale)), static_cast<int>(std::round(adjustedMat.rows * params.scale)));
        int interp = params.scale < 1.0f ? cv::INTER_AREA : cv::INTER_LINEAR;
        cv::resize(adjustedMat, adjustedMat, dsize, 0, 0, interp);
    }
    // 2. Brightness/contrast
    if (params.contrast != 1.0f || params.brightness != 0) { adjustedMat.convertTo(adjustedMat, -1, params.contrast, params.brightness); }
    // 3. Blur
    if (params.applyBlur && params.blurKernel > 1) { cv::GaussianBlur(adjustedMat, adjustedMat, { params.blurKernel, params.blurKernel }, 0, 0, cv::BORDER_DEFAULT); }
    // 4. Flip
    int flipCode = -2; if (params.flipV && params.flipH) flipCode = -1; else if (params.flipV) flipCode = 0; else if (params.flipH) flipCode = 1; if (flipCode > -2) { cv::flip(adjustedMat, adjustedMat, flipCode); }
    // 5. Grayscale
    if (adjustedMat.channels() == 3) { cv::cvtColor(adjustedMat, grayMat, cv::COLOR_BGR2GRAY); }
    else if (adjustedMat.channels() == 4) { cv::cvtColor(adjustedMat, grayMat, cv::COLOR_BGRA2GRAY); }
    else if (adjustedMat.channels() == 1) { grayMat = adjustedMat; }
    else { std::cerr << "Unsupported channels for grayscale: " << adjustedMat.channels() << std::endl; return; }
    // 6. Canny
    cv::Canny(grayMat, edgeMat, params.cannyLow, params.cannyHigh, 3, false);
    // 7. Create output mat
    outPixelArtMat = cv::Mat::zeros(edgeMat.size(), CV_8UC1);
    const int pixelSize = std::max(2, params.pixelSize); const int spacing = 1;

    const int drawW_fixed = std::max(1, pixelSize - 2 * spacing);
    const int drawH_fixed = std::max(1, pixelSize - 2 * spacing);

    // 8. Pixelation Loop
    for (int y = 0; y < edgeMat.rows; y += pixelSize) {
        for (int x = 0; x < edgeMat.cols; x += pixelSize) {
            int blockW = std::min(pixelSize, edgeMat.cols - x); int blockH = std::min(pixelSize, edgeMat.rows - y); if (blockW <= 0 || blockH <= 0) continue;
            cv::Mat roiEdge = edgeMat({ x, y, blockW, blockH });
            if (cv::mean(roiEdge)[0] > 0) {
                outBlockCoords.push_back({ x / pixelSize, y / pixelSize });
                int drawX = x + spacing; int drawY = y + spacing;
                if ((drawX + drawW_fixed <= outPixelArtMat.cols) && (drawY + drawH_fixed <= outPixelArtMat.rows)) {
                    cv::rectangle(outPixelArtMat, { drawX, drawY, drawW_fixed, drawH_fixed }, cv::Scalar(255), cv::FILLED);
                }
                else if (blockW == 1 && blockH == 1) { cv::rectangle(outPixelArtMat, { x, y, 1, 1 }, cv::Scalar(255), cv::FILLED); }
            }
        }
    }
}

std::string generateCode(const std::vector<sf::Vector2i>& coords, const std::string& format) {
    if (coords.empty()) { return "// No edge blocks detected."; }
    std::string output = "", lineEnding = ",\n";
    if (format == "csharp") {
        output += "private static readonly List<(int x, int y)> edgePixels = new List<(int x, int y)>\n{\n";
        for (size_t i = 0; i < coords.size(); ++i) { output += "    (" + std::to_string(coords[i].x) + ", " + std::to_string(coords[i].y) + ")"; output += (i == coords.size() - 1) ? "\n" : lineEnding; } output += "};";
    }
    else if (format == "js") {
        output += "const edgePixels = [\n";
        for (size_t i = 0; i < coords.size(); ++i) { output += "  [" + std::to_string(coords[i].x) + ", " + std::to_string(coords[i].y) + "]"; output += (i == coords.size() - 1) ? "\n" : lineEnding; } output += "];";
    }
    else if (format == "python") {
        output += "edge_pixels = [\n";
        for (size_t i = 0; i < coords.size(); ++i) { output += "    (" + std::to_string(coords[i].x) + ", " + std::to_string(coords[i].y) + ")"; output += (i == coords.size() - 1) ? "\n" : lineEnding; } output += "]";
    }
    else { output = "// Unknown format selected"; }
    return output;
}

void ApplyModernStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    ImVec4 bgDark = ImVec4(0.10f, 0.10f, 0.11f, 1.00f); // Very dark background
    ImVec4 bgMedium = ImVec4(0.13f, 0.14f, 0.15f, 1.00f); // Slightly lighter bg for elements
    ImVec4 bgLight = ImVec4(0.18f, 0.19f, 0.20f, 1.00f); // Lighter background for frames, headers
    ImVec4 accent = ImVec4(0.22f, 0.68f, 0.54f, 1.00f); // Teal/Green accent
    ImVec4 accentHover = ImVec4(0.28f, 0.74f, 0.60f, 1.00f);
    ImVec4 accentActive = ImVec4(0.32f, 0.80f, 0.65f, 1.00f);
    ImVec4 text = ImVec4(0.90f, 0.90f, 0.90f, 1.00f); // Light gray text
    ImVec4 textDisabled = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    ImVec4 border = ImVec4(0.25f, 0.25f, 0.27f, 1.00f); // Subtle border

    // Rounding
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.TabRounding = 4.0f;

    // Borders
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f; // No frame border for cleaner look
    style.PopupBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f; // Subtle border for child windows

    // Padding and Spacing
    style.WindowPadding = ImVec2(8.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f); // Slightly tighter frame padding
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(5.0f, 5.0f);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;

    // Colors
    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = textDisabled;
    colors[ImGuiCol_WindowBg] = bgDark;
    colors[ImGuiCol_ChildBg] = bgMedium; // Child windows slightly lighter
    colors[ImGuiCol_PopupBg] = bgDark;   // Popups same as main background
    colors[ImGuiCol_Border] = border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // No border shadow
    colors[ImGuiCol_FrameBg] = bgLight;  // Frames lighter than child bg
    colors[ImGuiCol_FrameBgHovered] = ImVec4(bgLight.x * 1.2f, bgLight.y * 1.2f, bgLight.z * 1.2f, 1.00f); // Subtle hover
    colors[ImGuiCol_FrameBgActive] = ImVec4(bgLight.x * 1.4f, bgLight.y * 1.4f, bgLight.z * 1.4f, 1.00f); // Subtle active
    colors[ImGuiCol_TitleBg] = bgDark;   // Title bar same as main bg
    colors[ImGuiCol_TitleBgActive] = accent;   // Active title bar uses accent
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = bgMedium;
    colors[ImGuiCol_ScrollbarBg] = ImVec4(bgMedium.x, bgMedium.y, bgMedium.z, 0.6f); // Semi-transparent scrollbar bg
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(border.x * 1.5f, border.y * 1.5f, border.z * 1.5f, 0.8f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(border.x * 1.8f, border.y * 1.8f, border.z * 1.8f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = accent; // Scrollbar grab active uses accent
    colors[ImGuiCol_CheckMark] = accent; // Checkmark uses accent
    colors[ImGuiCol_SliderGrab] = accent; // Slider grab uses accent
    colors[ImGuiCol_SliderGrabActive] = accentActive;
    colors[ImGuiCol_Button] = accent; // Button uses accent
    colors[ImGuiCol_ButtonHovered] = accentHover;
    colors[ImGuiCol_ButtonActive] = accentActive;
    colors[ImGuiCol_Header] = bgLight; // Header uses lighter background
    colors[ImGuiCol_HeaderHovered] = accentHover;
    colors[ImGuiCol_HeaderActive] = accentActive;
    colors[ImGuiCol_Separator] = border; // Separator uses border color
    colors[ImGuiCol_SeparatorHovered] = accentHover;
    colors[ImGuiCol_SeparatorActive] = accentActive;
    colors[ImGuiCol_ResizeGrip] = ImVec4(accent.x, accent.y, accent.z, 0.20f); // Subtle resize grip
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(accent.x, accent.y, accent.z, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(accent.x, accent.y, accent.z, 0.95f);
    colors[ImGuiCol_Tab] = bgLight; // Tabs use lighter bg
    colors[ImGuiCol_TabHovered] = accentHover;
    colors[ImGuiCol_TabActive] = accent; // Active tab uses accent
    colors[ImGuiCol_TabUnfocused] = bgMedium;
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(accent.x * 0.7f, accent.y * 0.7f, accent.z * 0.7f, 1.00f); // Muted accent for unfocused active tab
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = accent; // Histogram uses accent
    colors[ImGuiCol_PlotHistogramHovered] = accentHover;
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = border;
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f); // Slightly more visible alt row bg
    colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(accent.x, accent.y, accent.z, 0.90f);
    colors[ImGuiCol_NavHighlight] = accent; // Navigation highlight uses accent
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.35f); // Darker modal dim
}