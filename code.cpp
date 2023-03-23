//这个文件负责二维码的编码
#include"code.h"
#include <cstring>
#include <string>
#include <vector>
//定义下面一个宏来开启debug
//#define Code_DEBUG
/* 定义宏 Show_Scale_Img(src)，展示缩放后的图像，参数 src 是输入的s图像，宏内部调用 ScaleToDisSize() 函数，将 src 缩放到一定的大小，然后显示缩放后的图像，并等待按键。*/
//#define Show_Scale_Img(src) do\
//{
//	Mat temp=ScaleToDisSize(src);\
//	imshow("Code_DEBUG", temp);\
//	waitKey();\
//}while (0);//这个宏用于显示经过缩放后的图像
namespace Code
{
	constexpr int BytesPerFrame = 5738;
	//constexpr int BytesPerFrame = 1242;//每帧的数据量
	constexpr int FrameSize = 108;//每帧像素的大小（108*108），每个像素可以存一个bit
	constexpr int FrameOutputRate = 10;//缩放比例，放大10*10倍
	constexpr int SafeAreaWidth = 2;//安全区域的宽度，单位为像素
	constexpr int QrPointSize = 18;//二维码的大小
	constexpr int SmallQrPointbias = 6;//二维码中小点的偏移量
	constexpr int RectAreaCount = 7;//矩形区域的数量
	const Vec3b pixel[8] =
	{
		Vec3b(0,0,0),Vec3b(0,0,255),Vec3b(0,255,0),Vec3b(0,255,255),
		Vec3b(255,0,0),Vec3b(255,0,255),Vec3b(255,255,0), Vec3b(255,255,255)
	};//8种颜色值，但是只用到了pixel[0]（黑色）和pixel[7]（白色）
	const int lenlim[RectAreaCount] = { 526,432,2444,432,832,248,224 };
	//const int lenlim[RectAreaCount] = { 138,144,648,144,144,16,8 };
	const int areapos[RectAreaCount][2][2] = //[2][2],第一维度代表高宽，第二维度代表左上角坐标
	{
		{{69,16},{QrPointSize + 3,SafeAreaWidth}},
		{{16,72},{SafeAreaWidth,QrPointSize}},
		{{72,72},{QrPointSize,QrPointSize}},
		{{72,16},{QrPointSize,FrameSize - QrPointSize}},
		{{16,72},{FrameSize - QrPointSize,QrPointSize}},
		{{8,16},{FrameSize - QrPointSize,FrameSize - QrPointSize}},
		{{8,8},{FrameSize - QrPointSize + 8,FrameSize - QrPointSize}}
	};//矩形区域的位置和大小
	enum color
	{
		Black = 0,
		White = 7
	};
	enum class FrameType
	{
		Start = 0,
		End = 1,
		StartAndEnd = 2,
		Normal = 3
	};
	/* 对输入的图像进行缩放，返回缩放后的图像。*/
	Mat ScaleToDisSize(const Mat& src)
	{
		Mat dis;
		constexpr int FrameOutputSize = FrameSize * FrameOutputRate;
		dis = Mat(FrameOutputSize, FrameOutputSize, CV_8UC3);
		for (int i = 0; i < FrameOutputSize; ++i)
		{
			for (int j = 0; j < FrameOutputSize; ++j)
			{
				dis.at<Vec3b>(i, j) = src.at<Vec3b>(i / 10, j / 10);//放大图片，如原图中（18,28）的点在新图中放大至（18~27,28~37）的10*10正方形
			}
		}
		return dis;
	}
	// 计算校验码的函数输入为信息、长度、是否为起始位、是否为结束位和帧基数，输出为校验码
	uint16_t CalCheckCode(const unsigned char* info, int len, bool isStart, bool isEnd, uint16_t frameBase)
	{
		uint16_t ans = 0;
		int cutlen = (len / 2) * 2;
		for (int i = 0; i < cutlen; i += 2)
			ans ^= ((uint16_t)info[i] << 8) | info[i + 1];
		if (len & 1)
			ans ^= (uint16_t)info[cutlen] << 8;
		ans ^= len;
		ans ^= frameBase;
		uint16_t temp = (isStart << 1) + isEnd;
		ans ^= temp;
		return ans;
	}
	// 构建安全区域，将安全区域内的像素值全部设为白色
	void BulidSafeArea(Mat& mat)
	{
		constexpr int pos[4][2][2] =
		{
			{{0,FrameSize},{0,SafeAreaWidth}},
			{{0,FrameSize},{FrameSize - SafeAreaWidth,FrameSize}},
			{{0, SafeAreaWidth },{0,FrameSize}},
			{{FrameSize - SafeAreaWidth,FrameSize},{0,FrameSize}}
		};
		for (int k = 0; k < 4; ++k)
			for (int i = pos[k][0][0]; i < pos[k][0][1]; ++i)
				for (int j = pos[k][1][0]; j < pos[k][1][1]; ++j)
					mat.at<Vec3b>(i, j) = pixel[White];
		//将最外层的2像素宽度的框全部绘制为白色
#ifdef Code_DEBUG
		Show_Scale_Img(mat);
#endif
		return;
	}
	//构建二维码的函数的识别点
	void BulidQrPoint(Mat& mat)
	{
		//绘制大二维码识别点
		constexpr int pointPos[4][2] =
		{
			{0,0},
			{0,FrameSize - QrPointSize},
			{FrameSize - QrPointSize,0}
		};
		const Vec3b vec3bBig[9] =
		{
			pixel[Black],
			pixel[Black],
			pixel[Black],
			pixel[White],
			pixel[White],
			pixel[Black],
			pixel[Black],
			pixel[White],
			pixel[White]
		};
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < QrPointSize; ++j)
				for (int k = 0; k < QrPointSize; ++k)
					mat.at<Vec3b>(pointPos[i][0] + j, pointPos[i][1] + k) =
					vec3bBig[(int)max(fabs(j - 8.5), fabs(k - 8.5))];
		//绘制小二维码识别点
		constexpr int posCenter[2] = { FrameSize - SmallQrPointbias,FrameSize - SmallQrPointbias };
		const Vec3b vec3bsmall[5] =
		{
			pixel[Black],
			pixel[Black],
			pixel[White],
			pixel[Black],
			pixel[White],
		};
		for (int i = -4; i <= 4; ++i)
			for (int j = -4; j <= 4; ++j)
				mat.at<Vec3b>(posCenter[0] + i, posCenter[1] + j) =
				vec3bsmall[max(abs(i), abs(j))];
#ifdef Code_DEBUG
		Show_Scale_Img(mat);
#endif
	}
	//在图像的安全区域内构建二维码的校验码和帧号
	void BulidCheckCodeAndFrameNo(Mat& mat, uint16_t checkcode, uint16_t FrameNo)
	{
		uint32_t outputCode = (checkcode << 8) | (FrameNo);
		for (int i = 8; i < 16; ++i)
		{
			mat.at<Vec3b>(QrPointSize + 1, SafeAreaWidth + i) = pixel[outputCode & 7];
			outputCode >>= 3;
		}
		for (int i = 0; i < 16; ++i)
		{
			mat.at<Vec3b>(QrPointSize + 1, SafeAreaWidth + i) = pixel[(checkcode & 1) ? 7 : 0];
			checkcode >>= 1;
		}
		for (int i = 0; i < 16; ++i)
		{
			mat.at<Vec3b>(QrPointSize + 2, SafeAreaWidth + i) = pixel[(FrameNo & 1) ? 7 : 0];
			FrameNo >>= 1;
		}
#ifdef Code_DEBUG
		Show_Scale_Img(mat);
#endif
	}
	/*将一段字符串信息编码为二维码中的矩形区域，然后在指定的位置将编码后的信息写入到传入的图像中。*/
	void BulidInfoRect(Mat& mat, const char* info, int len, int areaID)
	{
		const unsigned char* pos = (const unsigned char*)info;  /* 将字符串 info 转换为无符号字符指针类型，指针 pos 指向字符串的首字符*/
		const unsigned char* end = pos + len;/* 指针 end 指向字符串的末字符的下一个位置。*/
		for (int i = 0; i < areapos[areaID][0][0]; ++i)//i用于遍历每一行
		{
			uint32_t outputCode = 0;//用于保存8个字符所对应的二进制编码
			for (int j = 0; j < areapos[areaID][0][1] / 8; ++j)//j用于遍历每行的所有字符空间
			{
				outputCode |= *pos++;
				for (int k = 0; k < 3; ++k)
				{
					outputCode <<= 8;
					if (pos != end)
						outputCode |= *pos++;
				}
				for (int k = areapos[areaID][1][1]; k < areapos[areaID][1][1] + 8; ++k)//k用于遍历每个字符空间中的每一位
				{
					mat.at<Vec3b>(i + areapos[areaID][1][0], j * 8 + k) = pixel[outputCode & 7];
					outputCode >>= 3;

					/*
					将这个8级灰度值所对应的颜色值存储到矩阵mat的指定位置，以渲染这个像素。
					这个位置是矩阵中位于当前像素块行号i加上区域areaID中信息块的起始行号，列号为(j*8+k)。这里通过areapos数组来获取信息块的起始行和列。
					*/

					//mat.at<Vec3b>(i+areapos[areaID][1][0], j*8+k) = pixel[(outputCode&1)?7:0];//根据每一位选择写入黑色还是白色
					//outputCode >>= 1;//右移一位，获取下一位二进制编码的灰度值
				}
				if (pos == end) break;
			}
			if (pos == end) break;
		}
#ifdef Code_DEBUG
		Show_Scale_Img(mat);//如果未注释掉Code_DEBUG,执行到此位置会输出图片
#endif
	}
	/* 构建帧标志，指示该帧的类型和存储的数据量（单位：字节） */
	void BulidFrameFlag(Mat& mat, FrameType frameType, int tailLen)
	{
		//前4位根据帧类型进行选择
		switch (frameType)
		{
		case FrameType::Start:
			//开始帧：白 白 黑 黑
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 1) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 2) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 3) = pixel[Black];
			break;
		case FrameType::End:
			//结束帧：黑 黑 白 白
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 1) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 2) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 3) = pixel[White];
			break;
		case FrameType::StartAndEnd:
			//开始并结束帧：白 白 白 白
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 1) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 2) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 3) = pixel[White];
			break;
		default:
			//普通帧：黑 黑 黑 黑
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 1) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 2) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 3) = pixel[Black];
			break;
		}
		//后续12位写入存储的数据量
		for (int i = 4; i < 16; ++i)
		{
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + i) = pixel[(tailLen & 1) ? 7 : 0];
			tailLen >>= 1;
		}
		for (int i = 4; i < 8; ++i)
		{
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + i) = pixel[tailLen & 7];
			tailLen >>= 3;
		}
#ifdef Code_DEBUG
		Show_Scale_Img(mat);
#endif
	}
	//构建一帧的二维码矩阵
	Mat CodeFrame(FrameType frameType, const char* info, int tailLen, int FrameNo)
	{
		Mat codeMat = Mat(FrameSize, FrameSize, CV_8UC3, Vec3d(255, 255, 255));
		if (frameType != FrameType::End && frameType != FrameType::StartAndEnd)
			tailLen = BytesPerFrame;//如果不是最后一帧，那么存储的数据量一定是预设好的 BytesPerFrame
		BulidSafeArea(codeMat);//构建安全区域（纯白边框）
		BulidQrPoint(codeMat);//构建二维码（黑白点阵）

		int checkCode = CalCheckCode((const unsigned char*)info, tailLen,
			frameType == FrameType::Start || frameType == FrameType::StartAndEnd,
			frameType == FrameType::End || frameType == FrameType::StartAndEnd, FrameNo);//生成校验码
		BulidFrameFlag(codeMat, frameType, tailLen);//构建帧标志（存储帧类型和数据长度）
		BulidCheckCodeAndFrameNo(codeMat, checkCode, FrameNo % 65536);
		if (tailLen != BytesPerFrame)
			tailLen = BytesPerFrame;//构建完毕，tailLen全部设为BytesPerFrame，用于下一步的构建
		for (int i = 0; i < RectAreaCount && tailLen>0; ++i)//分块构建二维码
		{
			int lennow = std::min(tailLen, lenlim[i]);//lennow取区域最大字节数，或者当tailLen剩余不足区域最大字节数时，取tailLen
			BulidInfoRect(codeMat, info, lennow, i);
			tailLen -= lennow;
			info += lennow;
		}
		return codeMat;
	}
	void Main(const char* info, int len, const char* savePath, const char* outputFormat, int FrameCountLimit)
	{
		Mat output;
		char fileName[128];//文件名
		int counter = 0;
		if (FrameCountLimit == 0) return; //帧数限制
		if (len <= 0);
		else if (len <= BytesPerFrame)//若文件长度小于一帧的最大长度，则只需要一帧即可保存全部信息
		{
			unsigned char BUF[BytesPerFrame + 5];//开辟临时空间
			memcpy(BUF, info, sizeof(unsigned char) * len);//传送数据
			for (int i = len; i <= BytesPerFrame; ++i)
				BUF[i] = rand() % 256;//在空闲字节随机生成8位二进制数，填满一帧
			output = ScaleToDisSize(CodeFrame(FrameType::StartAndEnd, (char*)BUF, len, 0));//构建一帧二维码矩阵，然后缩放10倍
			sprintf_s(fileName, "%s\\%05d.%s", savePath, counter++, outputFormat);//生成文件路径
			imwrite(fileName, output);//输出图片
		}
		else
		{
			//构建开始帧
			int i = 0;
			len -= BytesPerFrame;
			output = ScaleToDisSize(CodeFrame(FrameType::Start, info, len, 0));
			--FrameCountLimit;//更新帧数限制

			sprintf_s(fileName, "%s\\%05d.%s", savePath, counter++, outputFormat);//生成文件路径
			imwrite(fileName, output);//输出开始帧

			//构建其他帧
			while (len > 0 && FrameCountLimit > 0)
			{
				info += BytesPerFrame;//移动内存指针
				--FrameCountLimit;//更新帧数限制
				if (len - BytesPerFrame > 0)//剩余信息量仍大于一帧可存储的最大值
				{
					if (FrameCountLimit > 0)
						output = ScaleToDisSize(CodeFrame(FrameType::Normal, info, BytesPerFrame, ++i));//未达帧限制，构建正常帧并缩放
					else output = ScaleToDisSize(CodeFrame(FrameType::End, info, BytesPerFrame, ++i));//达到帧限制，构建结束帧并缩放
				}
				else //剩余信息量不足一帧
				{
					unsigned char BUF[BytesPerFrame + 5];
					memcpy(BUF, info, sizeof(unsigned char) * len);
					for (int i = len; i <= BytesPerFrame; ++i)
						BUF[i] = rand() % 256;//在空闲字节随机生成8位二进制数，填满一帧
					output = ScaleToDisSize(CodeFrame(FrameType::End, (char*)BUF, len, ++i));//构建结束帧并缩放
				}
				len -= BytesPerFrame;//更新剩余数据长度
				sprintf_s(fileName, "%s\\%05d.%s", savePath, counter++, outputFormat);
				imwrite(fileName, output);
			};
		}
		return;
	}
}
