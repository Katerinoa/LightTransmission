#include "ErrorCode.h"

int index[130];

namespace ErrorCode
{
	void PreProcessing()
	{
		int gap = 0;
		for (int i = 1; i <= 128; ++i)
		{
			int k = gap + i;
			while ((k & (k - 1)) == 0) k++, gap++;
			index[i] = k;
		}
	}

	int getBit(char byte, int pos)
	{// 得到字节特定位的值
		return (int)byte >> pos & 1;
	}

	void bytesTobits(std::bitset<138>& bits, char bytes, int pos)
	{
		for (int i = 0; i < 8; ++i)
		{
			while ((pos & (pos - 1)) == 0) pos++;
			bits[pos++] = getBit(bytes, i);
		}
	}

	char bitsTobytes(std::bitset<138>& bits, int st)
	{
		int res = 0;
		int pos = st * 8;
		for (int i = 0; i < 8; ++i)
			res = res & (bits[i + st] >> i);
		char ch = res;
		return ch;
	}

	char* EncodeErrorCorrectionCode(char* info, int& len)
	{// 字节大端位小端
		PreProcessing();
		int hammingCodeByte = len / 16;						// 每128位（16字节）得到8位的校验码
		len += hammingCodeByte;								// 最后可能会不足128位，不足的就不加校验码，否则最后128位的校验码长度不足8位，最后不好判断具体有多少位
		char* temp = (char*)malloc(sizeof(char) * len);
		for (int i = 0; i < hammingCodeByte; ++i)
		{
			std::bitset<8> hammingCode;						// 8位海明码									// gap的作用是让数据流中的下标对应到海明码的下标，比如说数据流第一个数据的下标为0，但实际上他在海明码中的下标是3，前两位是校验码
			for (int j = i * 16; j < (i + 1) * 16; ++j)		// j是循环16字节
			{
				int m = (int)info[j];						// 为了位运算，这里将char转为int
				for (int u = 0; u < 8; ++u)					// 遍历这个字节的8位
				{
					// 找到当前位再海明码中的下标
					int pos = index[(j - i * 16) * 8 + u + 1];
					for (int p = 0; p < 8; ++p)				// 根据二进制确定海明码的分组
						if ((pos >> p & 1) && (m >> u & 1)) hammingCode.flip(p);	// 如果属于p组，并且当前位是1，则让海明码异或
				}
			}
			for (int j = 0; j < 16; ++j)
				temp[i * 17 + j] = info[i * 16 + j];
			int u = 0;
			for (int j = 0; j < 8; ++j)
				u += (hammingCode[j] << j);
			temp[i * 17 + 16] = (char)u;
		}
		for (int i = hammingCodeByte * 16; i < (len - hammingCodeByte); ++i)	// 剩余字节写入temp
			temp[hammingCodeByte * 17 + (i - hammingCodeByte * 16)] = info[i];
		info = temp;
		return info;
	}

	void DecodeErrorCorrectionCode(std::vector<unsigned char>& outFile)
	{
		PreProcessing();
		int hammingCodeByte = outFile.size() / 17;
		std::vector<unsigned char> temp;
		for (int i = 0; i < hammingCodeByte; ++i)
		{
			std::bitset<8> G;
			for (int j = i * 17; j < (i + 1) * 17 - 1; ++j)
			{
				int m = (int)outFile[j];
				for (int u = 0; u < 8; ++u)
				{
					int pos = index[(j - i * 17) * 8 + u + 1];
					for (int p = 0; p < 8; ++p)
						if ((pos >> p & 1) && (m >> u & 1)) G.flip(p);
				}
			}
			for (int u = 0; u < 8; ++u)
			{
				int k = (int)outFile[i * 17 + 16];
				if (k >> u & 1) G.flip(u);
			}
			int error = 0;
			for (int j = 0; j < 8; ++j)
				error += (G[j] << j);
			if (error)
			{
				int x = error / 8;
				int y = error % 8;
				int k = (int)outFile[x];
				k ^= (1 << y);
				outFile[x] = (char)k;
			}
			for (int j = 0; j < 16; ++j)
				temp.push_back(outFile[i * 17 + j]);
		}
		for (int i = hammingCodeByte * 17; i < outFile.size(); ++i)
			temp.push_back(outFile[i]);
		outFile.swap(temp);
	}
}