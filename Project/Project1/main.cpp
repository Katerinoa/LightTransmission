//������
#include <iostream>
#include <chrono>
#include"parser.h"
#include"code.h"
#include"ffmpeg.h"
#include"ErrorCode.h"
#include"ImgDecode.h"
using namespace std;

#define Show_Img(src) do\
{\
	cv::imshow("DEBUG", src);\
	cv::waitKey();\
}while (0);
//��ƵתͼƬ
int FileToVideo(const char* filePath, const char* videoPath, int timLim = INT_MAX,bool useHamming=false,int fps = 10)
{
	FILE* fp = fopen(filePath, "rb");
	if (fp == nullptr) return 1;
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	rewind(fp);
	char* temp = (char*)malloc(sizeof(char) * size);
	if (temp == nullptr) {
		printf("null!"); return 1;
	}
	fread(temp, 1, size, fp);
	fclose(fp);
	system("md outputImg");
	if(useHamming)temp = ErrorCode::EncodeErrorCorrectionCode(temp, size);				// ������

	Code::Main(temp, size, "outputImg", "png", 1LL * fps * timLim / 1000);
	FFMPEG::ImagetoVideo("outputImg", "png", videoPath, fps, 60, 100000);

	system("rd /s /q outputImg");
	free(temp);
	return 0;
}

//��ƵתͼƬ
int VideoToFile(const char* videoPath, const char* filePath,bool useHammingcode=false)
{
	char imgName[256];

	system("rd /s /q inputImg");
	system("md inputImg");
	bool isThreadOver = false;

	std::thread th([&] {FFMPEG::VideotoImage(videoPath, "inputImg", "jpg"); isThreadOver = true; });

	// precode���ں���ָ��֡�ı�ţ����ж��Ƿ������֡�����߳�����ͬ֡
	int precode = -1;
	std::vector<unsigned char> outputFile;
	// hasStarted����ָ����Ƶ�Ƿ�ʼ��ע������ָ����Ƶ��Ҫ�������Ƶ��ʵ���������Ƶ���ȴ��ڵ���Ҫ�������Ƶ��
	bool hasStarted = 0;
	// retָ���Ƿ������֡
	bool ret = 0;
	std::this_thread::sleep_for(std::chrono::seconds(1)); // �ó�����ͣ1����
	for (int i = 1;; ++i, system((std::string("del ") + imgName).c_str()))
	{
		printf("Reading Image %05d.jpg\n", i);
		snprintf(imgName, 256, "inputImg\\%05d.jpg", i);
		FILE* fp;
		do
		{
			fp = fopen(imgName, "rb");
		} while (fp == nullptr && !isThreadOver);
		if (fp == nullptr)
		{
			FILE* fp_res = fopen("res.txt", "w");
			fprintf(fp_res, "Failed to open the video, is the video Incomplete?");
			fclose(fp_res);
			puts("failed to open the video, is the video Incomplete?");
			ret = 1;
			break;
		}
		cv::Mat srcImg = cv::imread(imgName, 1), disImg;
		fclose(fp);

		if (ImgParse::Main(srcImg, disImg))
		{
			continue;
		}
		//Show_Img(disImg);

		ImageDecode::ImageInfo imageInfo;
		bool ans = ImageDecode::Main(disImg, imageInfo);

		if (ans)
		{
			continue;
		}
		if (!hasStarted)
		{
			if (imageInfo.IsStart)
				hasStarted = 1;
			else continue;
		}
		// ��ǰһ֡��ͬ
		if (precode == imageInfo.FrameBase)
			continue;
		// ��Ƶ��������������֡����retΪ1
		if (((precode + 1) & UINT16_MAX) != imageInfo.FrameBase)
		{
			puts("error, there is a skipped frame,there are some images parsed failed.");
			ret = 1;
			break;
		}
		printf("Frame %d is parsed!\n", imageInfo.FrameBase);

		precode = (precode + 1) & UINT16_MAX;
		// ���������������д�뵽����ļ���
		for (auto& e : imageInfo.Info)
			outputFile.push_back(e);
		// �����������һ֡
		if (imageInfo.IsEnd)
			break;
	}
	if (ret == 0)
	{
		th.join();	// �������߳�

		if(useHammingcode)ErrorCode::DecodeErrorCorrectionCode(outputFile);
		FILE* fp_res = fopen("res.txt", "w");
		fprintf(fp_res, "Video Parse is success.\nFile Size:%lldB\nTotal Frame:%d\n", outputFile.size(), precode);
		fclose(fp_res);
		outputFile.push_back('\0');
		printf("\nVideo Parse is success.\nFile Size:%lldB\nTotal Frame:%d\n", outputFile.size(), precode);
		FILE* fp = fopen(filePath, "wb");
		if (fp == nullptr) return 1;
		fwrite(outputFile.data(), sizeof(unsigned char), outputFile.size() - 1, fp);
		fclose(fp);
		return ret;
	}
	exit(1);
	return 0;
}


int main(int argc, char* argv[])
{
	VideoToFile(argv[1], argv[2], std::stoi(argv[3]));
	return 0;
}
