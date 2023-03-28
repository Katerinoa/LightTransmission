//������
#include"parser.h"
#include <iostream>
#include"code.h"
#include"ffmpeg.h"
//#include"ImgDecode.h"


int FileToVideo(const char* filePath, const char* videoPath, int timLim = INT_MAX, int fps = 15)
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
	Code::Main(temp, size, "outputImg", "png", 1LL * fps * timLim / 1000);  //ɾ����һ������
	FFMPEG::ImagetoVideo("outputImg", "png", videoPath, fps, 60, 100000);
	system("rd /s /q outputImg");
	free(temp);
	return 0;
}


//int main(int argc, char* argv[])
//{
//	const char* filepath = "test.bin";
//	const char* videopath = "test_video.mp4";
//	FileToVideo(filepath, videopath);
//	return 0;
//}

/*
int main(int argc, char* argv[])
{
	constexpr bool type = false;
	//type==true ���ļ�����Ϊ��Ƶ  �����в��� �� �����ļ�·�� �����Ƶ·�� ���Ƶʱ��
	//type==false ����Ƶ����Ϊ�ļ� �����в��� �� ������Ƶ·�� ���ͼƬ·��
	if constexpr(type)
	{
		if (argc == 4)
			return FileToVideo(argv[1], argv[2], std::stoi(argv[3]));
		else if (argc == 5)
			return FileToVideo(argv[1], argv[2], std::stoi(argv[3]), std::stoi(argv[4]));
	}
	else
	{
		if (argc == 3)
			return VideoToFile(argv[1], argv[2]);
	}
	puts("argument error,please check your argument");
	return 1;
}
*/

//�����������ڲ��Զ�ά��Ķ�λ�Ͳü�
int main() {
	for (int i = 0; i < 15; i++)
	{
		char imgName[256];
		//snprintf(imgName, 256, "C:\\Users\\TingLans\\Desktop\\outputImg\\%05d.png", i);
		snprintf(imgName, 256, "C:\\Users\\TingLans\\Desktop\\myImg\\%05d.jpg", i);


		cv::Mat srcImg = cv::imread(imgName, 1), disImg;

		if (srcImg.empty()) {
			std::cerr << "Failed to open image file" << std::endl;
			return 1;
		}

		ImgParse::Main(srcImg, disImg);

		cv::imshow("disImg", srcImg);
		cv::waitKey(0);
		cv::imshow("disImg", disImg);
		cv::waitKey(0);


	}
	return 0;
}