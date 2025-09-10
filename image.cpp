#include "image.h"

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

//For Image textures
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Assimp includes
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

using namespace std;

Image LoadImageFromFile(const char* filename) {
	int width, height, channels;
	unsigned char* data = stbi_load(filename, &width, &height, &channels, 0); // Keep original channels

	if (!data) {
		std::cerr << "Failed to load image: " << filename << std::endl;
		return Image(0, 0, 0); // Return empty image
	}

	Image image(width, height, channels); // Use actual number of channels
	std::memcpy(image.data.data(), data, width * height * channels); // Copy pixel data

	stbi_image_free(data); // Free the original data from stb_image
	std::cout << "Loaded image: " << filename << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
	return image;
}

Image CreateConstantColorImage(int width, int height, unsigned char r, unsigned char g, unsigned char b , unsigned char a ) {
	Image canvas(width, height, 3); // 4 channels (RGB)

	for (size_t i = 0; i < canvas.data.size(); i += 3) {
		canvas.data[i] = r;     // Red
		canvas.data[i + 1] = g; // Green
		canvas.data[i + 2] = b; // Blue
	}

	return canvas;
}


Image ComputeDifference(const Image& canvas, const Image& reference) {
	Image diff(canvas.width, canvas.height, canvas.channels);
	for (int i = 0; i < canvas.width * canvas.height * canvas.channels; ++i) {
		diff.data[i] = (reference.data[i] - canvas.data[i]) * (reference.data[i] - canvas.data[i]);
	}
	return diff;
}

Image rescaleImage(const Image& src, float newWidth, float newHeight) {

	Image scaled(newWidth, newHeight, 3);
	for (int y = 0; y < newHeight; ++y) {
		for (int x = 0; x < newWidth; ++x) {
			int srcX = x * src.width / newWidth;
			int srcY = y * src.height / newHeight;

			int srcIndex = (srcY * src.width + srcX) * 3;
			int dstIndex = (y * newWidth + x) * 3;

			for (int c = 0; c < 3; ++c) {
				scaled.data[dstIndex + c] = src.data[srcIndex + c];
			}
		}
	}
	return scaled;
}