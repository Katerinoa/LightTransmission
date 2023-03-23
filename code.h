#pragma once
#include<opencv2/opencv.hpp>

#include<cstdio>

namespace Code
{
	using namespace cv;
	using namespace std;
	enum class FrameType;
	uint16_t CalCheckCode(const unsigned char* info, int len, bool isStart, bool isEnd, uint16_t frameBase);//����У����
	void BulidSafeArea(Mat& mat);//�ڶ�ά���й�����ȫ���򣬼���ɫ�ı߿�
	void BulidQrPoint(Mat& mat);//������ά�붨λ�㣬����ɫ�Ĵ�������
	void BulidCheckCodeAndFrameNo(Mat& mat, uint16_t checkcode, uint16_t FrameNo);//�ڶ�ά��������У�����֡���
	void BulidInfoRect(Mat& mat, const char* info, int len);//�ڶ�ά���й�����Ϣ���Σ�����ɫ�Ͱ�ɫ����С������
	void BulidFrameFlag(Mat& mat, FrameType frameType, int tailLen);//����֡��־
	Mat CodeFrame(FrameType frameType, const char* info, int tailLen, int FrameNo);//����������֡
	void Main(const char* info, int len, const char* savePath, const char* outputFormat, int FrameCountLimit = INT_MAX);//���ɶ�ά��
	/*
	�����������Ϣ�Ͳ�������һ����ά�룬����������浽ָ��·���µ��ļ��С�
	�Ƚ���Ϣ��ֳ����ɸ�֡��Ȼ�����ÿ��֡���� CodeFrame ��������֡��ͼ���������֡��ͼ��ƴ�������γ������Ķ�ά�롣
	*/
}