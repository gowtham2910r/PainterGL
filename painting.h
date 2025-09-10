#pragma once

#include "image.h"
struct Stroke {
	float radius;  // Stroke size
	std::vector<unsigned char> color;  // Stroke color (same number of channels as the image)
	std::vector<std::pair<float, float>> points;  // Control points (x, y) along the stroke
};

extern float maxStrokeLength;
extern float minStrokeLength;
extern float thresholdColorDiff;
extern float jitterAmount;

void ComputeGradients(const Image& image, std::vector<float>& gradX, std::vector<float>& gradY);
Stroke MakeStroke(float R, int x0, int y0, const Image& referenceImage, const std::vector<float>& gradX, const std::vector<float>& gradY, float fc);
void ApplyGaussianBlur1D(const Image& input, Image& output, const std::vector<float>& kernel, bool horizontal);
std::vector<float> GenerateGaussianKernel(int radius, float sigma);
Image ApplyGaussianBlur(const Image& input, int radius, float sigma);
void DrawCircularDab(Image& canvas, int cx, int cy, float radius, const std::vector<unsigned char>& color);
Image& ApplyCurvedStroke(Image& canvas, const Stroke& stroke);
Image& PaintLayer(Image& canvas, const Image& referenceImage, float radius, float fc);