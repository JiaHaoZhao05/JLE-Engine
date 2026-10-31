#pragma once

#include <vector>
class Primitive {
public:
	Primitive() {}

	std::vector<float> vertex;
	unsigned int num_index;
	std::vector<int> index;
	const char* texturePath = nullptr;
};