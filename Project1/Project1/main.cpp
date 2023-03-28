//主函数
#include"parser.h"
#include <iostream>
#include"code.h"
#include"ffmpeg.h"
#include"ErrorCode.h"
#include"ImgDecode.h"

//视频转图片
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
	temp = ErrorCode::EncodeErrorCorrectionCode(temp, size);				// 海明码
	Code::Main(temp, size, "outputImg", "png", 1LL * fps * timLim / 1000);  //删除了一个变量
	FFMPEG::ImagetoVideo("outputImg", "png", videoPath, fps, 60, 100000);
	system("rd /s /q outputImg");
	free(temp);
	return 0;
}

//视频转图片
int VideoToFile(const char* videoPath, const char* filePath)
{
	char imgName[256];
	system("rd /s /q inputImg");
	system("md inputImg");
	// 设置一个新线程以完成视频到图片的转换
	bool isThreadOver = false;
	std::thread th([&] {FFMPEG::VideotoImage(videoPath, "inputImg", "jpg"); isThreadOver = true; });
	// precode用于后续指明帧的编号，以判断是否出现跳帧，或者出现相同帧
	int precode = -1;
	std::vector<unsigned char> outputFile;
	// hasStarted用于指明视频是否开始（注意这里指的视频是要解码的视频，实际拍摄的视频长度大于等于要解码的视频）
	bool hasStarted = 0;
	// ret指明是否出现跳帧
	bool ret = 0;
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
		// Show_Img(disImg);
		ImageDecode::ImageInfo imageInfo;
		bool ans = ImageDecode::Main(disImg, imageInfo);
		if (ans)
		{
			continue;
		}
		// 判断是否为第一帧（由于拍摄的第一帧并不是所需内容的第一帧，所以需要过滤掉无用帧）
		if (!hasStarted)
		{
			if (imageInfo.IsStart)
				hasStarted = 1;
			else continue;
		}
		// 和前一帧相同
		if (precode == imageInfo.FrameBase)
			continue;
		// 视频不连续，出现跳帧，置ret为1
		if (((precode + 1) & UINT16_MAX) != imageInfo.FrameBase)
		{
			puts("error, there is a skipped frame,there are some images parsed failed.");
			ret = 1;
			break;
		}
		printf("Frame %d is parsed!\n", imageInfo.FrameBase);

		precode = (precode + 1) & UINT16_MAX;
		// 将解码出来的内容写入到输出文件中
		for (auto& e : imageInfo.Info)
			outputFile.push_back(e);
		// 处理到了最后一帧
		if (imageInfo.IsEnd)
			break;
	}
	if (ret == 0)
	{
		th.join();	// 结束子线程
		printf("\nVideo Parse is success.\nFile Size:%lldB\nTotal Frame:%d\n", outputFile.size(), precode);
		FILE* fp = fopen(filePath, "wb");
		if (fp == nullptr) return 1;
		ErrorCode::DecodeErrorCorrectionCode(outputFile);
		outputFile.push_back('\0');
		fwrite(outputFile.data(), sizeof(unsigned char), outputFile.size() - 1, fp);
		fclose(fp);
		return ret;
	}
	exit(1);
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

//该主函数用于测试二维码的定位和裁剪
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