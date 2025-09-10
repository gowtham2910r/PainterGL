#pragma once
#include<vector>
using namespace std;

struct Image {
	int width, height, channels;
	std::vector<unsigned char> data; // Stores pixel data

	Image(int w, int h, int c) : width(w), height(h), channels(c), data(w* h* c) {}
};

Image LoadImageFromFile(const char* filename);
Image CreateConstantColorImage(int width, int height, unsigned char r = 255, unsigned char g = 255, unsigned char b = 255, unsigned char a = 255);
Image ComputeDifference(const Image& canvas, const Image& reference);
Image rescaleImage(const Image& src, float newWidth, float newHeight);