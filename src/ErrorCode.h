#pragma once
#include<opencv2/opencv.hpp>
#include <vector>
#include<cstdio>
#include <cstring>
#include <string>
#include <bitset>
#include <algorithm>

namespace ErrorCode
{
	char* EncodeErrorCorrectionCode(char* info, int& len);
	void DecodeErrorCorrectionCode(std::vector<unsigned char>& outFile);
}