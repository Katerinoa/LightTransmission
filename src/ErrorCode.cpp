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
	{// �õ��ֽ��ض�λ��ֵ
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
	{// �ֽڴ��λС��
		PreProcessing();
		int hammingCodeByte = len / 16;						// ÿ128λ��16�ֽڣ��õ�8λ��У����
		len += hammingCodeByte;								// �����ܻ᲻��128λ������ľͲ���У���룬�������128λ��У���볤�Ȳ���8λ����󲻺��жϾ����ж���λ
		char* temp = (char*)malloc(sizeof(char) * len);
		for (int i = 0; i < hammingCodeByte; ++i)
		{
			std::bitset<8> hammingCode;						// 8λ������									// gap�����������������е��±��Ӧ����������±꣬����˵��������һ�����ݵ��±�Ϊ0����ʵ�������ں������е��±���3��ǰ��λ��У����
			for (int j = i * 16; j < (i + 1) * 16; ++j)		// j��ѭ��16�ֽ�
			{
				int m = (int)info[j];						// Ϊ��λ���㣬���ｫcharתΪint
				for (int u = 0; u < 8; ++u)					// ��������ֽڵ�8λ
				{
					// �ҵ���ǰλ�ٺ������е��±�
					int pos = index[(j - i * 16) * 8 + u + 1];
					for (int p = 0; p < 8; ++p)				// ���ݶ�����ȷ��������ķ���
						if ((pos >> p & 1) && (m >> u & 1)) hammingCode.flip(p);	// �������p�飬���ҵ�ǰλ��1�����ú��������
				}
			}
			for (int j = 0; j < 16; ++j)
				temp[i * 17 + j] = info[i * 16 + j];
			int u = 0;
			for (int j = 0; j < 8; ++j)
				u += (hammingCode[j] << j);
			temp[i * 17 + 16] = (char)u;
		}
		for (int i = hammingCodeByte * 16; i < (len - hammingCodeByte); ++i)	// ʣ���ֽ�д��temp
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