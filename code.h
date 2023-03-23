#pragma once
#include<opencv2/opencv.hpp>

#include<cstdio>

namespace Code
{
	using namespace cv;
	using namespace std;
	enum class FrameType;
	uint16_t CalCheckCode(const unsigned char* info, int len, bool isStart, bool isEnd, uint16_t frameBase);//计算校验码
	void BulidSafeArea(Mat& mat);//在二维码中构建安全区域，即白色的边框
	void BulidQrPoint(Mat& mat);//构建二维码定位点，即黑色的大正方形
	void BulidCheckCodeAndFrameNo(Mat& mat, uint16_t checkcode, uint16_t FrameNo);//在二维码中生成校验码和帧编号
	void BulidInfoRect(Mat& mat, const char* info, int len);//在二维码中构建信息矩形，即黑色和白色相间的小正方形
	void BulidFrameFlag(Mat& mat, FrameType frameType, int tailLen);//构建帧标志
	Mat CodeFrame(FrameType frameType, const char* info, int tailLen, int FrameNo);//构建完整的帧
	void Main(const char* info, int len, const char* savePath, const char* outputFormat, int FrameCountLimit = INT_MAX);//生成二维码
	/*
	根据输入的信息和参数生成一个二维码，并将结果保存到指定路径下的文件中。
	先将信息拆分成若干个帧，然后针对每个帧调用 CodeFrame 函数生成帧的图像，最后将所有帧的图像拼接起来形成完整的二维码。
	*/
}