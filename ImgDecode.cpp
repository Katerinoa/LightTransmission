//这个文件封装了二维码的解码
#include"ImgDecode.h"
#include"pic.h"
#include"code.h"
namespace ImageDecode
{
	enum color
	{
		Black = 0,
		White = 7
	};
	const Vec3b pixel[8] =
	{
		Vec3b(0,0,0),Vec3b(0,0,255),Vec3b(0,255,0),Vec3b(0,255,255),
		Vec3b(255,0,0),Vec3b(255,0,255),Vec3b(255,255,0), Vec3b(255,255,255)
	};
	//const int lenlim[RectAreaCount] = { 426,432,1944,432,432,48,24 };
	const int lenlim[RectAreaCount] = { 138,144,648,144,144,16,8 };
	const int areapos[RectAreaCount][2][2] = //[2][2],第一维度代表高宽，第二维度代表左上角坐标
	{
		{{69,16},{QrPointSize + 3,SafeAreaWidth}},
		{{16,72},{SafeAreaWidth,QrPointSize}},
		{{72,72},{QrPointSize,QrPointSize}},
		{{72,16},{QrPointSize,FrameSize - QrPointSize}},
		{{16,72},{FrameSize - QrPointSize,QrPointSize}},
		{{8,16},{FrameSize - QrPointSize,FrameSize - QrPointSize}},
		{{8,8},{FrameSize - QrPointSize + 8,FrameSize - QrPointSize}}
	};
	/* 读取校验码和帧编号 */
	void GetcheckCodeAndFrameNo(const Mat& mat, uint16_t& checkcode, uint16_t& FrameNo)
	{
		//uint32_t outputCode = 0;//(checkcode << 8) | (FrameNo)
		/*for (int i = 15; i >= 8; --i)
		{
			const auto& temp = mat.at<Vec3b>(QrPointSize, SafeAreaWidth + i);
			int j;
			for (j = 0; j < 7; ++j)
				if (temp == pixel[j])
					break;
			outputCode <<= 3;
			outputCode |= j;
		}*/
		for (int i = 15; i >= 0; --i)
		{
			const auto& temp = mat.at<Vec3b>(QrPointSize + 1, SafeAreaWidth + i);//第二列的16位：校验码
			checkcode <<= 1;
			checkcode |= ((temp == pixel[0]) ? 0 : 1);//读取校验码
		}
		for (int i = 15; i >= 0; --i)
		{
			const auto& temp = mat.at<Vec3b>(QrPointSize + 2, SafeAreaWidth + i);//第三列的16位：帧号
			FrameNo <<= 1;
			FrameNo |= ((temp == pixel[0]) ? 0 : 1);//读取帧号
		}
		//checkcode = outputCode >> 8;
		//FrameNo = outputCode & 255;
	}
	/* 读取一块矩形空间中的内容并存入文件 */
	void GetInfoRect(const Mat& mat, unsigned char* info, int len, int areaID)
	{
		unsigned char* pos = info;//文件头指针
		const unsigned char* end = pos + len;//文件尾指针
		for (int i = 0; i < areapos[areaID][0][0]; ++i)//遍历每一行
		{//每次循环结束得到一行中的所有内容
			uint32_t outputCode = 0;
			for (int j = 0; j < areapos[areaID][0][1] / 8; ++j)//遍历每一个字符空间
			{//每次循环结束得到一个8位字节

				for (int k = areapos[areaID][1][1] + 7; k >= areapos[areaID][1][1]; --k)//遍历每一列，即每个字节
				{
					const auto& temp = mat.at<Vec3b>(i + areapos[areaID][1][0], j * 8 + k);
					int l;
					/*for (l = 0;l < 7; ++l)
						if (temp == pixel[l])
							break;
					outputCode <<= 3;
					outputCode |= l;*/
					outputCode <<= 1;
					outputCode |= ((temp == pixel[0]) ? 0 : 1);//读取temp中的内容
				}
				*(pos) = outputCode;//读取的8位字节传入文件中
				/*for (int k = 2; k >= 0; --k)
				{
					if (pos + k < end)
						*(pos + k) = outputCode & 255;
					outputCode >>= 8;
				}*/
				//pos += 3;
				++pos;//文件指针一位（一个char类型 8bit）
				if (pos == end) break;
			}
			if (pos == end) break;
		}
	}
	/* 读取一帧中的帧类型和存储数据量 */
	void GetFrameFlag(const Mat& mat, int& tailLen, bool& start, bool& end)
	{
		tailLen = 0;
		start = (mat.at<Vec3b>(QrPointSize, SafeAreaWidth) == pixel[White]);//第一位是白色说明是开始帧
		end = (mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 2) == pixel[White]);//第三位是白色说明是结束帧
		/*for (int i = 7; i >= 4; --i)
		{
			const auto& temp = mat.at<Vec3b>(QrPointSize, SafeAreaWidth + i);
			int j;
			for (j = 0; j < 7; ++j)
				if (temp == pixel[j])
					break;
			tailLen <<= 3;
			tailLen |= j;
		}*/
		for (int i = 15; i >= 4; --i)
		{
			const auto& temp = mat.at<Vec3b>(QrPointSize, SafeAreaWidth + i);
			tailLen <<= 1;
			tailLen |= ((temp == pixel[0]) ? 0 : 1);//读取该帧存储的数据量
		}
	}
	/* 解码一帧 若校验码不等返回true，否则返回false */
	bool Main(Mat& mat, ImageInfo& imageInfo)
	{
		int tailLen = 1242, codeLen = 0;
		GetFrameFlag(mat, codeLen, imageInfo.IsStart, imageInfo.IsEnd);
		if (codeLen < 0) return 1;
		if (imageInfo.IsEnd == 1) tailLen = std::min(tailLen, 1242);//若是最后一帧，则数据量取1242和tailLen的较小值
		GetcheckCodeAndFrameNo(mat, imageInfo.CheckCode, imageInfo.FrameBase);//读取校验码和帧号
		imageInfo.Info = vector<unsigned char>(tailLen); //获取 imageInfo.Info 的指针，指向该向量的第一个元素
		unsigned char* info = imageInfo.Info.data();
		for (int i = 0; i < RectAreaCount && tailLen>0; ++i)
		{
			int lennow = std::min(tailLen, lenlim[i]);
			GetInfoRect(mat, info, lennow, i);
			tailLen -= lennow;
			info += lennow;
		}//分块将内容读取到imageInfo的data域中
		/* imageInfo.Info 中多余的元素，直到它的大小等于 codeLen（编码时使用随机数来填补最后一帧中的剩余空间） */
		while (imageInfo.Info.size() > codeLen) imageInfo.Info.pop_back();
		return imageInfo.CheckCode != Code::CalCheckCode(imageInfo.Info.data(), codeLen, imageInfo.IsStart, imageInfo.IsEnd, imageInfo.FrameBase);
	}
}