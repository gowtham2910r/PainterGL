
#include "image.h"
#include "painting.h"
#include <vector>

// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>

//GlM includes
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/ext/matrix_transform.hpp"

using namespace std;


float maxStrokeLength = 15.0f, minStrokeLength = 3.0f;
float thresholdColorDiff = 15.0f;
float jitterAmount = 30.0f;


void ComputeGradients(const Image& image, std::vector<float>& gradX, std::vector<float>& gradY) {
	gradX.resize(image.width * image.height);
	gradY.resize(image.width * image.height);

	for (int y = 1; y < image.height - 1; ++y) {
		for (int x = 1; x < image.width - 1; ++x) {
			int idx = y * image.width + x;

			// Compute grayscale intensity (assuming RGB)
			int idxL = idx - 1, idxR = idx + 1;
			int idxT = idx - image.width, idxB = idx + image.width;

			float intensityL = 0.3f * image.data[idxL * 3] + 0.59f * image.data[idxL * 3 + 1] + 0.11f * image.data[idxL * 3 + 2];
			float intensityR = 0.3f * image.data[idxR * 3] + 0.59f * image.data[idxR * 3 + 1] + 0.11f * image.data[idxR * 3 + 2];
			float intensityT = 0.3f * image.data[idxT * 3] + 0.59f * image.data[idxT * 3 + 1] + 0.11f * image.data[idxT * 3 + 2];
			float intensityB = 0.3f * image.data[idxB * 3] + 0.59f * image.data[idxB * 3 + 1] + 0.11f * image.data[idxB * 3 + 2];

			gradX[idx] = intensityR - intensityL;
			gradY[idx] = intensityB - intensityT;
		}
	}
}

Stroke MakeStroke(float R, int x0, int y0, const Image& referenceImage, const std::vector<float>& gradX, const std::vector<float>& gradY, float fc) {
	Stroke stroke;
	stroke.radius = R;

	// Find initial color at the starting point
	int idx = (y0 * referenceImage.width + x0);
	stroke.color.resize(referenceImage.channels);
	for (int c = 0; c < referenceImage.channels; ++c) {
		int colorValue = referenceImage.data[idx * referenceImage.channels + c];



		float jitter = ((rand() % 1000) / 1000.0f) * 2 * jitterAmount - jitterAmount;
		colorValue += static_cast<int>(jitter);

		stroke.color[c] = static_cast<unsigned char>(glm::max(0, glm::min(255, colorValue)));
	}

	// Initialize the first point
	stroke.points.push_back({ x0, y0 });

	// Initialize last direction
	float lastDx = 0.0f;
	float lastDy = 0.0f;
	float x = x0, y = y0;

	// Main loop to generate the stroke
	for (int i = 0; i < maxStrokeLength / 0.2; ++i) {
		// Check for early exit condition based on color difference
		int idxCanvas = (y * referenceImage.width + x);
		float colorDiff = 0.0f;

		// Ensure idxCanvas is within valid bounds
		if (idxCanvas < 0 || idxCanvas >= referenceImage.width * referenceImage.height) {
			std::cerr << "Out-of-bounds access at idxCanvas: " << idxCanvas << std::endl;
			continue;  // Skip if out of bounds
		}

		int idx = idxCanvas * referenceImage.channels;  // Base index for accessing color channels

		// Ensure the index is within bounds for the channels
		if (idx + referenceImage.channels > referenceImage.data.size()) {
			std::cerr << "Out-of-bounds access for color channels at idx: " << idx << std::endl;
			continue;  // Skip if out of bounds
		}

		// Now safely access the pixel data
		for (int c = 0; c < referenceImage.channels; ++c) {
			colorDiff += std::abs(referenceImage.data[idx + c] - stroke.color[c]);
		}

		if (i > minStrokeLength / 0.2 && colorDiff < thresholdColorDiff) {
			break;
		}

		// Get the gradient direction at the current point
		int gradIdx = (y * referenceImage.width + x);
		float gx = gradX[gradIdx];
		float gy = gradY[gradIdx];

		// Compute normal direction (perpendicular to the gradient)
		float dx = -gy;
		float dy = gx;

		// Ensure the direction does not reverse
		if (lastDx * dx + lastDy * dy < 0) {
			dx = -dx;
			dy = -dy;
		}

		// Smooth the stroke direction with the previous one
		dx = fc * dx + (1 - fc) * lastDx;
		dy = fc * dy + (1 - fc) * lastDy;

		// Normalize direction
		float length = std::sqrt(dx * dx + dy * dy);
		dx /= length;
		dy /= length;

		// Move the stroke to the next point
		x += R * dx * 0.2f;
		y += R * dy * 0.2f;

		// Add the new point to the stroke
		stroke.points.push_back({ x, y });

		// Update the last direction
		lastDx = dx;
		lastDy = dy;

		// Stop if we detect a vanishing gradient
		if (std::sqrt(gx * gx + gy * gy) == 0) {
			break;
		}
	}

	return stroke;
}

void ApplyGaussianBlur1D(const Image& input, Image& output, const std::vector<float>& kernel, bool horizontal) {
	int width = input.width;
	int height = input.height;
	int radius = kernel.size() / 2;

	output = Image(width, height, input.channels); // Create output image

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			std::vector<float> sum(input.channels, 0.0f);

			for (int k = -radius; k <= radius; k++) {
				int sampleX = horizontal ? glm::clamp(x + k, 0, width - 1) : x;
				int sampleY = horizontal ? y : glm::clamp(y + k, 0, height - 1);
				int sampleIdx = (sampleY * width + sampleX) * input.channels;

				for (int c = 0; c < input.channels; c++) {
					sum[c] += input.data[sampleIdx + c] * kernel[k + radius];
				}
			}

			int outputIdx = (y * width + x) * input.channels;
			for (int c = 0; c < input.channels; c++) {
				output.data[outputIdx + c] = static_cast<unsigned char>(sum[c]);
			}
		}
	}
}

std::vector<float> GenerateGaussianKernel(int radius, float sigma) {
		int size = 2 * radius + 1;
	std::vector<float> kernel(size);
	float sum = 0.0f;

	for (int i = 0; i < size; i++) {
		int x = i - radius;
		kernel[i] = std::exp(-(x * x) / (2 * sigma * sigma));
		sum += kernel[i];
	}

	// Normalize the kernel
	for (int i = 0; i < size; i++) {
		kernel[i] /= sum;
	}

	return kernel;
}

Image ApplyGaussianBlur(const Image& input, int radius, float sigma) {
	std::vector<float> kernel = GenerateGaussianKernel(radius, sigma);

	Image temp(input.width, input.height, input.channels);
	Image blurred(input.width, input.height, input.channels);

	// Apply horizontal blur
	ApplyGaussianBlur1D(input, temp, kernel, true);

	// Apply vertical blur
	ApplyGaussianBlur1D(temp, blurred, kernel, false);

	return blurred;
}


void DrawCircularDab(Image& canvas, int cx, int cy, float radius, const std::vector<unsigned char>& color) {
	// Ensure the coordinates are within bounds of the canvas
	cx = glm::clamp(cx, 0, canvas.width - 1);
	cy = glm::clamp(cy, 0, canvas.height - 1);

	int xMin = std::max(0, cx - static_cast<int>(radius));
	int xMax = std::min(canvas.width - 1, cx + static_cast<int>(radius));
	int yMin = std::max(0, cy - static_cast<int>(radius));
	int yMax = std::min(canvas.height - 1, cy + static_cast<int>(radius));

	// Iterate through the circle's bounding box and apply the dab if inside the circle
	for (int x = xMin; x <= xMax; ++x) {
		for (int y = yMin; y <= yMax; ++y) {
			float dx = x - cx;
			float dy = y - cy;
			if (dx * dx + dy * dy <= radius * radius) {  // Only paint within the circle
				int idx = (y * canvas.width + x) * canvas.channels;
				for (int c = 0; c < canvas.channels; ++c) {
					canvas.data[idx + c] = static_cast<unsigned char>(
						0.5f * canvas.data[idx + c] + 0.5f * color[c]  // Soft blending
						);
				}
			}
		}
	}
}

Image& ApplyCurvedStroke(Image& canvas, const Stroke& stroke) {
	for (const auto& point : stroke.points) {
		float x = point.first;
		float y = point.second;

		// Here you can apply the dab method (circular stroke) at each control point
		DrawCircularDab(canvas, static_cast<int>(x), static_cast<int>(y), stroke.radius, stroke.color);
	}
	return canvas;
}


Image& PaintLayer(Image& canvas, const Image& referenceImage, float radius, float fc) {
	Image difference = ComputeDifference(canvas, referenceImage);
	std::vector<Stroke> strokes;

	float grid_size_factor = 1.0f;
	int grid_size = static_cast<int>(radius * grid_size_factor);

	// Compute gradients for stroke alignment
	std::vector<float> gradX, gradY;
	ComputeGradients(referenceImage, gradX, gradY);

	float T = 10.0f;  // Threshold for significant error

	for (int x = 0; x < canvas.width; x += grid_size) {
		for (int y = 0; y < canvas.height; y += grid_size) {
			float areaError = 0.0f;
			int maxErrorX = x, maxErrorY = y;
			float maxError = 0.0f;

			// Find the max error in the surrounding area
			for (int i = -grid_size / 2; i <= grid_size / 2; ++i) {
				for (int j = -grid_size / 2; j <= grid_size / 2; ++j) {
					int xi = glm::clamp(x + i, 0, canvas.width - 1);
					int yj = glm::clamp(y + j, 0, canvas.height - 1);
					int idx = (yj * canvas.width + xi) * canvas.channels;

					if (idx < 0 || idx >= static_cast<int>(difference.data.size())) {
						std::cerr << "Out-of-bounds access at idx: " << idx << std::endl;
						continue;
					}

					float error = 0;
					for (int c = 0; c < canvas.channels; ++c) {
						error = std::max(error, static_cast<float>(abs(difference.data[idx + c])));
					}

					areaError += error;
					if (error > maxError) {
						maxError = error;
						maxErrorX = xi;
						maxErrorY = yj;
					}
				}
			}

			areaError /= sqrt(grid_size * grid_size);
			if (areaError > T) {
				// Generate a stroke if the area error is significant
				strokes.push_back(MakeStroke(radius, maxErrorX, maxErrorY, referenceImage, gradX, gradY, 0.8f));
			}
		}
	}

	// Apply the curved strokes to the canvas
	for (const Stroke& stroke : strokes) {
		canvas = ApplyCurvedStroke(canvas, stroke);
	}
	return canvas;
}