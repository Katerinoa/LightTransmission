//这个文件负责二维码的编码
#include"code.h"

#define Show_Scale_Img(src) do\
{\
	Mat temp=ScaleToDisSize(src);\
	imshow("Code_DEBUG", temp);\
	waitKey();\
}while (0);
namespace Code
{
	constexpr int BytesPerFrame = 2282;// 区域存放数据量
	constexpr int FrameSize_h = 192;
	constexpr int FrameSize_w = 108;

	//constexpr int FrameSize = 108;

	constexpr int FrameOutputRate = 10;
	constexpr int SafeAreaWidth = 2;
	constexpr int QrPointSize = 18;
	constexpr int SmallQrPointbias = 6;
	constexpr int RectAreaCount = 7;
	const Vec3b pixel[8] =
	{
		Vec3b(0,0,0),Vec3b(0,0,255),Vec3b(0,255,0),Vec3b(0,255,255),
		Vec3b(255,0,0),Vec3b(255,0,255),Vec3b(255,255,0), Vec3b(255,255,255)
	};
	const int lenlim[RectAreaCount] = { 138,312-8,1404-36,144,312-8,16,8 };
	const int areapos[RectAreaCount][2][2] = //[2][2],第一维度代表高宽，第二维度代表左上角坐标
	{
		{{69,16},{QrPointSize + 3,SafeAreaWidth}},
		{{16,156},{SafeAreaWidth,QrPointSize}},
		{{72,156},{QrPointSize,QrPointSize}},
		{{72,16},{QrPointSize,FrameSize_h - QrPointSize}},
		{{16,156},{FrameSize_w - QrPointSize,QrPointSize}},
		{{8,16},{FrameSize_w - QrPointSize,FrameSize_h - QrPointSize}},
		{{8,8},{FrameSize_w - QrPointSize + 8,FrameSize_h - QrPointSize}}
	};
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
	Mat ScaleToDisSize(const Mat& src)
	{
		Mat dis;
		constexpr int FrameOutputSize_w = FrameSize_w * FrameOutputRate;
		constexpr int FrameOutputSize_h = FrameSize_h * FrameOutputRate;
		dis = Mat(FrameOutputSize_w, FrameOutputSize_h, CV_8UC3);              //aaa
		for (int i = 0; i < FrameOutputSize_w; ++i)
		{
			for (int j = 0; j < FrameOutputSize_h; ++j)
			{
				dis.at<Vec3b>(i, j) = src.at<Vec3b>(i / 10, j / 10);
			}
		}
		return dis;
	}
	// 需修改——计算校验码————————————
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
	// 安全空白区域
	void BulidSafeArea(Mat& mat)
	{
		for (int i = 0; i <= 1; i++)
			for (int j = 0; j <= 191; j++)
				mat.at<Vec3b>(i, j) = pixel[White];

		for (int i = 106; i <= 107; i++)
			for (int j = 0; j <= 191; j++)
				mat.at<Vec3b>(i, j) = pixel[White];

		for (int i = 2; i <= 105; i++)
			for (int j = 0; j <= 1; j++)
				mat.at<Vec3b>(i, j) = pixel[White];

		for (int i = 2; i <= 105; i++)
			for (int j = 190; j <= 191; j++)
				mat.at<Vec3b>(i, j) = pixel[White];

#ifdef Code_DEBUG
		Show_Scale_Img(mat);
#endif
		return;
	}
	// 识别点
	void BulidQrPoint(Mat& mat)
	{
		//绘制大二维码识别点
		constexpr int pointPos[4][2] =
		{
			{0,0},
			{0,FrameSize_h - QrPointSize},
			{FrameSize_w - QrPointSize,0}
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
		constexpr int posCenter[2] = { FrameSize_w - SmallQrPointbias,FrameSize_h - SmallQrPointbias };
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
	// 没用的区域
	void FillIn(Mat& mat)
	{
		int k = 0;
		for (int i = 2; i < 106; i++)
			for (int j = 170; j < 174; j++)
			{
				if ((k + i) % 2)mat.at<Vec3b>(i, j) = pixel[Black];
         		k++;
			}
	}
	// 需修改——————————
	void BulidCheckCodeAndFrameNo(Mat& mat, uint16_t checkcode, uint16_t FrameNo)
	{
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
	void BulidInfoRect(Mat& mat, const char* info, int len, int areaID)
	{
		const unsigned char* pos = (const unsigned char*)info;
		const unsigned char* end = pos + len;
		for (int i = 0; i < areapos[areaID][0][0]; ++i)
		{
			uint32_t outputCode = 0;
			for (int j = 0; j < areapos[areaID][0][1] / 8; ++j)
			{
				outputCode |= *pos++;
				for (int k = areapos[areaID][1][1]; k < areapos[areaID][1][1] + 8; ++k)
				{
					//mat.at<Vec3b>(i+areapos[areaID][1][0], j*8+k) = pixel[outputCode&7];
					//outputCode >>= 3;
					mat.at<Vec3b>(i + areapos[areaID][1][0], j * 8 + k) = pixel[(outputCode & 1) ? 7 : 0];
					outputCode >>= 1;
				}
				if (pos == end) break;
			}
			if (pos == end) break;
		}
#ifdef Code_DEBUG
		Show_Scale_Img(mat);
#endif
	}
	void BulidFrameFlag(Mat& mat, FrameType frameType, int tailLen)
	{
		switch (frameType)
		{
		case FrameType::Start:
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 1) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 2) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 3) = pixel[Black];
			break;
		case FrameType::End:
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 1) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 2) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 3) = pixel[White];
			break;
		case FrameType::StartAndEnd:
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 1) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 2) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 3) = pixel[White];
			break;
		default:
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 1) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 2) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 3) = pixel[Black];
			break;
		}
		for (int i = 4; i < 16; ++i)
		{
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + i) = pixel[(tailLen & 1) ? 7 : 0];
			tailLen >>= 1;
		}
#ifdef Code_DEBUG
		Show_Scale_Img(mat);
#endif
	}
	Mat CodeFrame(FrameType frameType, const char* info, int tailLen, int FrameNo)
	{
		Mat codeMat = Mat(FrameSize_w, FrameSize_h, CV_8UC3, Vec3d(255, 255, 255));   ///bbb
		if (frameType != FrameType::End && frameType != FrameType::StartAndEnd)
			tailLen = BytesPerFrame;
		BulidSafeArea(codeMat);// 安全区域
		BulidQrPoint(codeMat);// 识别点
		FillIn(codeMat);// 无用区域

		int checkCode = CalCheckCode((const unsigned char*)info, tailLen,
			frameType == FrameType::Start || frameType == FrameType::StartAndEnd,
			frameType == FrameType::End || frameType == FrameType::StartAndEnd, FrameNo);// 计算无用校验码-56807
		BulidFrameFlag(codeMat, frameType, tailLen);// 帧类型和数据量
		BulidCheckCodeAndFrameNo(codeMat, checkCode, FrameNo % 65536);// 校验码和帧号
		if (tailLen != BytesPerFrame)
			tailLen = BytesPerFrame;
		for (int i = 0; i < RectAreaCount && tailLen>0; ++i)
		{
			int lennow = std::min(tailLen, lenlim[i]);
			BulidInfoRect(codeMat, info, lennow, i);
			tailLen -= lennow;
			info += lennow;
		}
		return codeMat;
	}
	void Main(const char* info, int len, const char* savePath, const char* outputFormat, int FrameCountLimit)
	{
		Mat output;
		char fileName[128];
		int counter = 0;
		if (FrameCountLimit == 0) return;
		if (len <= 0);
		else if (len <= BytesPerFrame)
		{
			unsigned char BUF[BytesPerFrame + 5];
			memcpy(BUF, info, sizeof(unsigned char) * len);
			for (int i = len; i <= BytesPerFrame; ++i)
				BUF[i] = rand() % 256;
			output = ScaleToDisSize(CodeFrame(FrameType::StartAndEnd, (char*)BUF, len, 0));
			sprintf_s(fileName, "%s\\%05d.%s", savePath, counter++, outputFormat);
			imwrite(fileName, output);
		}
		else
		{
			int i = 0;
			len -= BytesPerFrame;
			output = ScaleToDisSize(CodeFrame(FrameType::Start, info, len, 0));
			--FrameCountLimit;

			sprintf_s(fileName, "%s\\%05d.%s", savePath, counter++, outputFormat);
			imwrite(fileName, output);

			while (len > 0 && FrameCountLimit > 0)
			{
				info += BytesPerFrame;
				--FrameCountLimit;
				if (len - BytesPerFrame > 0)
				{
					if (FrameCountLimit > 0)
						output = ScaleToDisSize(CodeFrame(FrameType::Normal, info, BytesPerFrame, ++i));
					else output = ScaleToDisSize(CodeFrame(FrameType::End, info, BytesPerFrame, ++i));
				}
				else
				{
					unsigned char BUF[BytesPerFrame + 5];
					memcpy(BUF, info, sizeof(unsigned char) * len);
					for (int i = len; i <= BytesPerFrame; ++i)
						BUF[i] = rand() % 256;
					output = ScaleToDisSize(CodeFrame(FrameType::End, (char*)BUF, len, ++i));
				}
				len -= BytesPerFrame;
				sprintf_s(fileName, "%s\\%05d.%s", savePath, counter++, outputFormat);
				imwrite(fileName, output);
			};
		}
		return;
	}
}