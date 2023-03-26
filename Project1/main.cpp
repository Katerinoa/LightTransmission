//主函数
//#include"pic.h"
#include <iostream>
#include"code.h"
#include"ffmpeg.h"
//#include"ImgDecode.h"


int FileToPhoto(const char* filePath, const char* videoPath, int timLim = INT_MAX, int fps = 15)
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
	Code::Main(temp, size, "outputImg", "png", 1LL * fps * timLim / 1000);  //删除了一个变量
	FFMPEG::ImagetoVideo("outputImg", "png", videoPath, fps, 60, 100000);
	system("rd /s /q outputImg");
	free(temp);
	return 0;
}


int main(int argc, char* argv[])
{
	const char* filepath = "test.bin";
	const char* videopath = "test_video.mp4";
	FileToPhoto(filepath, videopath);
	return 0;
}

/*
int main(int argc, char* argv[])
{
	constexpr bool type = false;
	//type==true 将文件编码为视频  命令行参数 ： 输入文件路径 输出视频路径 最长视频时长
	//type==false 将视频编码为文件 命令行参数 ： 输入视频路径 输出图片路径
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
