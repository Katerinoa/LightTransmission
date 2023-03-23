//����ļ������ά��ı���
#include"code.h"
#include <cstring>
#include <string>
#include <vector>
//��������һ����������debug
//#define Code_DEBUG
/* ����� Show_Scale_Img(src)��չʾ���ź��ͼ�񣬲��� src �������sͼ�񣬺��ڲ����� ScaleToDisSize() �������� src ���ŵ�һ���Ĵ�С��Ȼ����ʾ���ź��ͼ�񣬲��ȴ�������*/
//#define Show_Scale_Img(src) do\
//{
//	Mat temp=ScaleToDisSize(src);\
//	imshow("Code_DEBUG", temp);\
//	waitKey();\
//}while (0);//�����������ʾ�������ź��ͼ��
namespace Code
{
	constexpr int BytesPerFrame = 5738;
	//constexpr int BytesPerFrame = 1242;//ÿ֡��������
	constexpr int FrameSize = 108;//ÿ֡���صĴ�С��108*108����ÿ�����ؿ��Դ�һ��bit
	constexpr int FrameOutputRate = 10;//���ű������Ŵ�10*10��
	constexpr int SafeAreaWidth = 2;//��ȫ����Ŀ�ȣ���λΪ����
	constexpr int QrPointSize = 18;//��ά��Ĵ�С
	constexpr int SmallQrPointbias = 6;//��ά����С���ƫ����
	constexpr int RectAreaCount = 7;//�������������
	const Vec3b pixel[8] =
	{
		Vec3b(0,0,0),Vec3b(0,0,255),Vec3b(0,255,0),Vec3b(0,255,255),
		Vec3b(255,0,0),Vec3b(255,0,255),Vec3b(255,255,0), Vec3b(255,255,255)
	};//8����ɫֵ������ֻ�õ���pixel[0]����ɫ����pixel[7]����ɫ��
	const int lenlim[RectAreaCount] = { 526,432,2444,432,832,248,224 };
	//const int lenlim[RectAreaCount] = { 138,144,648,144,144,16,8 };
	const int areapos[RectAreaCount][2][2] = //[2][2],��һά�ȴ���߿��ڶ�ά�ȴ������Ͻ�����
	{
		{{69,16},{QrPointSize + 3,SafeAreaWidth}},
		{{16,72},{SafeAreaWidth,QrPointSize}},
		{{72,72},{QrPointSize,QrPointSize}},
		{{72,16},{QrPointSize,FrameSize - QrPointSize}},
		{{16,72},{FrameSize - QrPointSize,QrPointSize}},
		{{8,16},{FrameSize - QrPointSize,FrameSize - QrPointSize}},
		{{8,8},{FrameSize - QrPointSize + 8,FrameSize - QrPointSize}}
	};//���������λ�úʹ�С
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
	/* �������ͼ��������ţ��������ź��ͼ��*/
	Mat ScaleToDisSize(const Mat& src)
	{
		Mat dis;
		constexpr int FrameOutputSize = FrameSize * FrameOutputRate;
		dis = Mat(FrameOutputSize, FrameOutputSize, CV_8UC3);
		for (int i = 0; i < FrameOutputSize; ++i)
		{
			for (int j = 0; j < FrameOutputSize; ++j)
			{
				dis.at<Vec3b>(i, j) = src.at<Vec3b>(i / 10, j / 10);//�Ŵ�ͼƬ����ԭͼ�У�18,28���ĵ�����ͼ�зŴ�����18~27,28~37����10*10������
			}
		}
		return dis;
	}
	// ����У����ĺ�������Ϊ��Ϣ�����ȡ��Ƿ�Ϊ��ʼλ���Ƿ�Ϊ����λ��֡���������ΪУ����
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
	// ������ȫ���򣬽���ȫ�����ڵ�����ֵȫ����Ϊ��ɫ
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
		//��������2���ؿ�ȵĿ�ȫ������Ϊ��ɫ
#ifdef Code_DEBUG
		Show_Scale_Img(mat);
#endif
		return;
	}
	//������ά��ĺ�����ʶ���
	void BulidQrPoint(Mat& mat)
	{
		//���ƴ��ά��ʶ���
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
		//����С��ά��ʶ���
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
	//��ͼ��İ�ȫ�����ڹ�����ά���У�����֡��
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
	/*��һ���ַ�����Ϣ����Ϊ��ά���еľ�������Ȼ����ָ����λ�ý���������Ϣд�뵽�����ͼ���С�*/
	void BulidInfoRect(Mat& mat, const char* info, int len, int areaID)
	{
		const unsigned char* pos = (const unsigned char*)info;  /* ���ַ��� info ת��Ϊ�޷����ַ�ָ�����ͣ�ָ�� pos ָ���ַ��������ַ�*/
		const unsigned char* end = pos + len;/* ָ�� end ָ���ַ�����ĩ�ַ�����һ��λ�á�*/
		for (int i = 0; i < areapos[areaID][0][0]; ++i)//i���ڱ���ÿһ��
		{
			uint32_t outputCode = 0;//���ڱ���8���ַ�����Ӧ�Ķ����Ʊ���
			for (int j = 0; j < areapos[areaID][0][1] / 8; ++j)//j���ڱ���ÿ�е������ַ��ռ�
			{
				outputCode |= *pos++;
				for (int k = 0; k < 3; ++k)
				{
					outputCode <<= 8;
					if (pos != end)
						outputCode |= *pos++;
				}
				for (int k = areapos[areaID][1][1]; k < areapos[areaID][1][1] + 8; ++k)//k���ڱ���ÿ���ַ��ռ��е�ÿһλ
				{
					mat.at<Vec3b>(i + areapos[areaID][1][0], j * 8 + k) = pixel[outputCode & 7];
					outputCode >>= 3;

					/*
					�����8���Ҷ�ֵ����Ӧ����ɫֵ�洢������mat��ָ��λ�ã�����Ⱦ������ء�
					���λ���Ǿ�����λ�ڵ�ǰ���ؿ��к�i��������areaID����Ϣ�����ʼ�кţ��к�Ϊ(j*8+k)������ͨ��areapos��������ȡ��Ϣ�����ʼ�к��С�
					*/

					//mat.at<Vec3b>(i+areapos[areaID][1][0], j*8+k) = pixel[(outputCode&1)?7:0];//����ÿһλѡ��д���ɫ���ǰ�ɫ
					//outputCode >>= 1;//����һλ����ȡ��һλ�����Ʊ���ĻҶ�ֵ
				}
				if (pos == end) break;
			}
			if (pos == end) break;
		}
#ifdef Code_DEBUG
		Show_Scale_Img(mat);//���δע�͵�Code_DEBUG,ִ�е���λ�û����ͼƬ
#endif
	}
	/* ����֡��־��ָʾ��֡�����ͺʹ洢������������λ���ֽڣ� */
	void BulidFrameFlag(Mat& mat, FrameType frameType, int tailLen)
	{
		//ǰ4λ����֡���ͽ���ѡ��
		switch (frameType)
		{
		case FrameType::Start:
			//��ʼ֡���� �� �� ��
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 1) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 2) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 3) = pixel[Black];
			break;
		case FrameType::End:
			//����֡���� �� �� ��
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 1) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 2) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 3) = pixel[White];
			break;
		case FrameType::StartAndEnd:
			//��ʼ������֡���� �� �� ��
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 1) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 2) = pixel[White];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 3) = pixel[White];
			break;
		default:
			//��ͨ֡���� �� �� ��
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 1) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 2) = pixel[Black];
			mat.at<Vec3b>(QrPointSize, SafeAreaWidth + 3) = pixel[Black];
			break;
		}
		//����12λд��洢��������
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
	//����һ֡�Ķ�ά�����
	Mat CodeFrame(FrameType frameType, const char* info, int tailLen, int FrameNo)
	{
		Mat codeMat = Mat(FrameSize, FrameSize, CV_8UC3, Vec3d(255, 255, 255));
		if (frameType != FrameType::End && frameType != FrameType::StartAndEnd)
			tailLen = BytesPerFrame;//����������һ֡����ô�洢��������һ����Ԥ��õ� BytesPerFrame
		BulidSafeArea(codeMat);//������ȫ���򣨴��ױ߿�
		BulidQrPoint(codeMat);//������ά�루�ڰ׵���

		int checkCode = CalCheckCode((const unsigned char*)info, tailLen,
			frameType == FrameType::Start || frameType == FrameType::StartAndEnd,
			frameType == FrameType::End || frameType == FrameType::StartAndEnd, FrameNo);//����У����
		BulidFrameFlag(codeMat, frameType, tailLen);//����֡��־���洢֡���ͺ����ݳ��ȣ�
		BulidCheckCodeAndFrameNo(codeMat, checkCode, FrameNo % 65536);
		if (tailLen != BytesPerFrame)
			tailLen = BytesPerFrame;//������ϣ�tailLenȫ����ΪBytesPerFrame��������һ���Ĺ���
		for (int i = 0; i < RectAreaCount && tailLen>0; ++i)//�ֿ鹹����ά��
		{
			int lennow = std::min(tailLen, lenlim[i]);//lennowȡ��������ֽ��������ߵ�tailLenʣ�಻����������ֽ���ʱ��ȡtailLen
			BulidInfoRect(codeMat, info, lennow, i);
			tailLen -= lennow;
			info += lennow;
		}
		return codeMat;
	}
	void Main(const char* info, int len, const char* savePath, const char* outputFormat, int FrameCountLimit)
	{
		Mat output;
		char fileName[128];//�ļ���
		int counter = 0;
		if (FrameCountLimit == 0) return; //֡������
		if (len <= 0);
		else if (len <= BytesPerFrame)//���ļ�����С��һ֡����󳤶ȣ���ֻ��Ҫһ֡���ɱ���ȫ����Ϣ
		{
			unsigned char BUF[BytesPerFrame + 5];//������ʱ�ռ�
			memcpy(BUF, info, sizeof(unsigned char) * len);//��������
			for (int i = len; i <= BytesPerFrame; ++i)
				BUF[i] = rand() % 256;//�ڿ����ֽ��������8λ��������������һ֡
			output = ScaleToDisSize(CodeFrame(FrameType::StartAndEnd, (char*)BUF, len, 0));//����һ֡��ά�����Ȼ������10��
			sprintf_s(fileName, "%s\\%05d.%s", savePath, counter++, outputFormat);//�����ļ�·��
			imwrite(fileName, output);//���ͼƬ
		}
		else
		{
			//������ʼ֡
			int i = 0;
			len -= BytesPerFrame;
			output = ScaleToDisSize(CodeFrame(FrameType::Start, info, len, 0));
			--FrameCountLimit;//����֡������

			sprintf_s(fileName, "%s\\%05d.%s", savePath, counter++, outputFormat);//�����ļ�·��
			imwrite(fileName, output);//�����ʼ֡

			//��������֡
			while (len > 0 && FrameCountLimit > 0)
			{
				info += BytesPerFrame;//�ƶ��ڴ�ָ��
				--FrameCountLimit;//����֡������
				if (len - BytesPerFrame > 0)//ʣ����Ϣ���Դ���һ֡�ɴ洢�����ֵ
				{
					if (FrameCountLimit > 0)
						output = ScaleToDisSize(CodeFrame(FrameType::Normal, info, BytesPerFrame, ++i));//δ��֡���ƣ���������֡������
					else output = ScaleToDisSize(CodeFrame(FrameType::End, info, BytesPerFrame, ++i));//�ﵽ֡���ƣ���������֡������
				}
				else //ʣ����Ϣ������һ֡
				{
					unsigned char BUF[BytesPerFrame + 5];
					memcpy(BUF, info, sizeof(unsigned char) * len);
					for (int i = len; i <= BytesPerFrame; ++i)
						BUF[i] = rand() % 256;//�ڿ����ֽ��������8λ��������������һ֡
					output = ScaleToDisSize(CodeFrame(FrameType::End, (char*)BUF, len, ++i));//��������֡������
				}
				len -= BytesPerFrame;//����ʣ�����ݳ���
				sprintf_s(fileName, "%s\\%05d.%s", savePath, counter++, outputFormat);
				imwrite(fileName, output);
			};
		}
		return;
	}
}
