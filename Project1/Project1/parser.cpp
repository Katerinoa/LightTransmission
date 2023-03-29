#include"parser.h"
//这个文件负责将视频中拆解出的帧，重定位成原始二维码
// 定义下面两个宏来开启debug
//#define FIND_QRPOINT_DEBUG 1
//#define CropParallelRect_DEBUG 1
#define Show_Img(src) do\
{\
	imshow("DEBUG", src);\
	waitKey();\
}while (0);
namespace ImgParse
{
	constexpr float MinRightAngel = 75.0, MaxRightAngel = 105.0;  //可以接受为直角的范围
	constexpr float MaxQrTypeRate = 2.2, minQrTypeRate = 1.8;  //
	constexpr float MaxDistanceRate = 1.1, minDistanceRate = 0.9;
	Mat Rotation_90(const Mat& srcImg)
	{   //返回指定矩阵90度后的拷贝
		Mat tempImg;
		transpose(srcImg, tempImg);
		flip(tempImg, tempImg, 1);
		return tempImg;
	}

	//找到所提取轮廓的中心点
	Point2f CalRectCenter(const vector<Point>& contours)
	{
		//在提取的中心小正方形的边界上每隔周长个像素提取一个点的坐标，求所提取四个点的平均坐标（即为小正方形的大致中心）
		float centerx = 0, centery = 0;
		int n = contours.size();
		centerx = (contours[n / 4].x + contours[n * 2 / 4].x + contours[3 * n / 4].x + contours[n - 1].x) / 4;
		centery = (contours[n / 4].y + contours[n * 2 / 4].y + contours[3 * n / 4].y + contours[n - 1].y) / 4;
		return Point2f(centerx, centery);
	}

	//判断point1和point2的顺逆时针关系
	bool IsClockWise(const Point& basePoint, const Point& point1, const Point& point2)
	{
		float ax = point1.x - basePoint.x, ay = point1.y - basePoint.y;
		float bx = point2.x - basePoint.x, by = point2.y - basePoint.y;
#ifdef FIND_QRPOINT_DEBUG
		puts(((ax * by - bx * ay) > 0) ? "ClockWise" : "Anti-ClockWise");
#endif 
		return (ax * by - bx * ay) > 0;
		//如果点2位于点1的顺时针方向，返回真，否则返回假
	}
	//计算以point0为顶点，三个点构成的角度
	float Cal3PointAngle(const Point& point0, const Point& point1, const Point& point2)
	{
		float dx1 = point1.x - point0.x, dy1 = point1.y - point0.y;
		float dx2 = point2.x - point0.x, dy2 = point2.y - point0.y;
		return acos((dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10f)) * 180.0f / 3.141592653f;
	}
	Mat CropRect(const Mat& srcImg, const RotatedRect& rotatedRect)
	{   //从图像中裁剪出一个矩形（可带角度）
		cv::Mat srcPoints, disImg;
		boxPoints(rotatedRect, srcPoints);//得到该矩阵的四点，并存储在srcPoints中
		vector<Point2f> dis_points =
		{
			Point2f(0,rotatedRect.size.height - 1),
			Point2f(0,0),
			Point2f(rotatedRect.size.width - 1,0),
			Point2f(rotatedRect.size.width - 1,rotatedRect.size.height - 1)
		};//初始化目标矩阵的四点，其大小取决于初始矩阵（长宽不变）
		auto M = getPerspectiveTransform(srcPoints, dis_points); //计算变换矩阵，将src四个点圈出的矩形投射到dis_point圈出的矩形中
		warpPerspective(srcImg, disImg, M, rotatedRect.size); //进行透视变换以完成裁剪
#ifdef FIND_QRPOINT_DEBUG
		Show_Img(disImg);
#endif 
		return disImg;
	}
	float distance(const Point2f& a, const Point2f& b)
	{
		return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
	}
	//通过三个点的坐标计算出第四个点的坐标
	Point CalForthPoint(const Point& poi0, const Point& poi1, const Point& poi2)
	{
		return Point(poi2.x + poi1.x - poi0.x, poi2.y + poi1.y - poi0.y);
	}
	//计算∠p1p0p2的长度为bias的外角平分向量
	pair<float, float> CalExtendVec(const Point2f& poi0, const Point2f& poi1, const Point2f& poi2, float bias)
	{
		float dis0 = distance(poi0, poi1), dis1 = distance(poi0, poi2);
		float rate = dis1 / dis0;
		float x1 = poi0.x - poi2.x, y1 = poi0.y - poi2.y;
		float x2 = (poi0.x - poi1.x) * rate, y2 = (poi0.y - poi1.y) * rate;
		float totx = x1 + x2, toty = y1 + y2, distot = sqrt(totx * totx + toty * toty);
		return { totx / distot * bias, toty / distot * bias };
	}
	//从图像中将一个四边形透视变换为矩形(需要四个点）由点转换为Mat
	Mat CropParallelRect(const Mat& srcImg, const vector<Point2f>& srcPoints, Size size = { 0,0 })
	{
		cv::Mat disImg;
		vector<Point2f> poi4 = srcPoints;
		//int id[4][3] = { { 0,1,2 },{1,0,3},{2,0,3},{3,1,2} };
		for (int i = 0; i < 4; ++i)
		{
#ifdef CropParallelRect_DEBUG 
			line(srcImg, srcPoints[i], poi4[i], CV_RGB(255, 0, 0), 2);
			Show_Img(srcImg);
#endif
		}
		if (size == Size(0, 0))
			size = Size(distance(srcPoints[0], srcPoints[1]), distance(srcPoints[1], srcPoints[3]));
		vector<Point2f> dis_points =
		{
			Point2f(0,0),
			Point2f(size.width - 1,0),
			Point2f(0,size.height - 1),
			Point2f(size.width - 1,size.height - 1)
		};//初始化目标矩阵的四点，其大小取决于初始矩阵（长宽不变）
		auto M = getPerspectiveTransform(srcPoints, dis_points); //计算变换矩阵
		warpPerspective(srcImg, disImg, M, size); //进行透视变换以完成裁剪
#ifdef CropParallelRect_DEBUG 
		Show_Img(disImg);
#endif 
		return disImg;
	}

	//判断是否可以在接受范围内认定为直角
	bool isRightlAngle(float angle)
	{
		return MinRightAngel <= angle && MaxRightAngel >= angle;
	}
	namespace QrcodeParse
	{

#ifdef FIND_QRPOINT_DEBUG
		int TestCaseNumber = 1;
#endif
		// 解析信息类型，包含中心点，点数量和最小外接矩形
		struct ParseInfo
		{
			Point2f Center;
			int size;
			RotatedRect Rect;
			ParseInfo(const vector<Point>& pointSet) :
				Center(CalRectCenter(pointSet)),
				size(pointSet.size()),
				Rect(minAreaRect(pointSet)) {} //最小外接矩形
			ParseInfo() = default;
		};
		constexpr float MaxQRBWRate = 2.25, MinQRBWRate = 0.40;//识别点黑白比例限制（理想1.0）
		constexpr int MinQRSize = 10;//最小识别点大小，模糊半径由图像尺寸动态判定
		constexpr float MaxQRScale = 0.25, MinQRXYRate = 5.0 / 6.0, MaxQRXYRate = 6.0 / 5.0; //识别点长度占原图的最大比例，识别点的长宽比最小限制和最大限制
		//计算三个整数的方差
		double Cal3NumVariance(const int a, const int b, const int c)
		{
			double avg = (a + b + c) / 3.0;
			return (a - avg) * (a - avg) + (b - avg) * (b - avg) + (c - avg) * (c - avg);
			//其实应该除以三，但是都没除三也不影响比较大小。
		}
		//判断黑白比例是否合法
		bool IsQrBWRateLegal(const float rate)
		{
			return rate < MaxQRBWRate && rate > MinQRBWRate;
			// 理想情况rate=1.0，实际可能在一定范围内波动。
		}
		//定位点黑白条纹预处理函数
		bool BWRatePreprocessing(Mat& image, vector<int>& vValueCount)
		{
			int count = 0, nc = image.cols * image.channels(), nr = image.rows / 2;
			uchar lastColor = 0, * data = image.ptr<uchar>(nr); //data指向nr指向的一行，即中间行
			for (int i = 0; i < nc; i++)      //计算条纹数量以及各个条纹的像素数目
			{
				uchar color = data[i];
				if (color > 0)
					color = 255;//只要不是黑色就是白色
				if (i == 0)
				{
					lastColor = color;
					count++;
				}
				else
				{
					if (lastColor != color)
					{
						vValueCount.push_back(count);
						count = 0;
					}
					count++;
					lastColor = color;
				}
			}
			if (count) vValueCount.push_back(count); //每个count值保存条纹的宽度
			bool ans = vValueCount.size() >= 5; //五个条纹，黑白黑白黑
#ifdef FIND_QRPOINT_DEBUG
			printf(ans ? "Preprocess Passed!" : "Preprocess Failed!");
#endif   
			return ans; //二维码未通过预处理（条纹不够黑白黑白黑的5条）
		}
		//计算二维码识别点的XLabel的黑白比例是否满足要求
		bool IsQrBWRateXLabel(Mat& image)
		{

#ifdef FIND_QRPOINT_DEBUG
			printf("%c Labels:", (TestCaseNumber & 1) ? ('Y') : ('X'));
#endif 
			vector<int> vValueCount;
			if (!BWRatePreprocessing(image, vValueCount)) //未通过预处理，不是识别点
				return false;
			//横向黑白比例1:1:3:1:1
			int index = -1, maxCount = -1;
			for (int i = 0; i < vValueCount.size(); i++)
			{
				if (i == 0)
				{
					index = i;
					maxCount = vValueCount[i];
				}
				else if (vValueCount[i] > maxCount)
				{
					index = i;
					maxCount = vValueCount[i];
				}
			}
			//左边 右边 都有两个值，才行
			if (index < 2 || (vValueCount.size() - index) < 3)
				return false;
			//黑白比例1:1:3:1:1测试

			float rate = ((float)maxCount) / 3.00;//以中间的比例3为基准
			bool checkTag = 1;
#ifdef FIND_QRPOINT_DEBUG
			printf("BWRate: "); //正常应该是1：1：3：1：1
#endif
			for (int i = -2; i < 3; ++i)
			{
				float rateNow = vValueCount[index + i] / rate;
#ifdef FIND_QRPOINT_DEBUG
				printf("%f ", rateNow);
#endif
				if (i) checkTag &= IsQrBWRateLegal(rateNow);
			}
			return checkTag;
		}

		//计算二维码识别点的横纵黑白比例是否满足要求
		bool IsQrBWRate(Mat& image)
		{
#ifdef FIND_QRPOINT_DEBUG
			printf("\t");
#endif
			//计算X轴的比例
			bool xTest = IsQrBWRateXLabel(image);
			if (!xTest)
			{
#ifdef FIND_QRPOINT_DEBUG
				puts("\nFailed!");
#endif
				return false;
			}
#ifdef FIND_QRPOINT_DEBUG
			++TestCaseNumber;
			printf("\n\t");
#endif
			//矩阵旋转90度以计算Y轴比例
			Mat image_rotation_90 = Rotation_90(image);
			bool yTest = IsQrBWRateXLabel(image_rotation_90);
#ifdef FIND_QRPOINT_DEBUG
			puts(yTest ? "\nPass!" : "\nFailed!");
#endif
			return yTest;
		}

		bool IsQrSizeLegal(const Size2f& qrSize, const Size2f&& imgSize)
		{
			float xYScale = qrSize.height / qrSize.width;
#ifdef FIND_QRPOINT_DEBUG
			bool ans = qrSize.height >= MinQRSize && qrSize.width >= MinQRSize;
			ans &= qrSize.height / imgSize.height < MaxQRScale && qrSize.width / imgSize.width < MaxQRScale;
			ans &= xYScale <= MaxQRXYRate && xYScale >= MinQRXYRate;
			printf("Xsize:%.0f Ysize:%.0f XScale:%.2f%% YScale:%.2f%% xYScale:%.2f%% ",
				qrSize.height, qrSize.width,
				qrSize.height / imgSize.height * 100.0,
				qrSize.width / imgSize.width * 100.0,
				xYScale * 100.0
			);
			puts(ans ? "Size test Passed!" : "Size test Failed!");
			return ans;
#else
			if (qrSize.height < MinQRSize || qrSize.width < MinQRSize) //判断宽度和长度是否太小
				return false;
			// TODO: 此处需要修改 修改MaxQRScale，即qrSize.width / imgSize 的标准值 
			if (qrSize.height / imgSize.height >= MaxQRScale || qrSize.width / imgSize.width >= MaxQRScale)
				return false;                                          //判断相对原图所占的比例是否太大
			if (xYScale < MinQRXYRate || xYScale > MaxQRXYRate)        //判断长宽比是否失衡
				return false;
			return true;
#endif
		}

		//本函数判断输入的vector<Point>是否是二维码识别点
		bool IsQrPoint(const vector<Point>& contour, const Mat& img)
		{
#ifdef FIND_QRPOINT_DEBUG
			TestCaseNumber += (TestCaseNumber & 1) ? (1) : (2); //计数器控制
			printf("\nPOINT %d:\n\t", TestCaseNumber / 2);
#endif
			RotatedRect rotatedRect = minAreaRect(contour);
			//计算最小覆盖矩形
			cv::Mat cropImg = CropRect(img, rotatedRect);
			//将二维码从整个图上抠出来
			if (!IsQrSizeLegal(rotatedRect.size, img.size())) return false;
			//判断尺寸是否合法
			return IsQrBWRate(cropImg);
			//判断黑白比例是否合法
		}

		//输入图像预处理函数 转换为灰度图，进行模糊处理，然后再转换为二值图
		Mat ImgPreprocessing(const Mat& srcImg, float blurRate = 0.0005)
		{
			Mat tmpImg;
			//彩色图转灰度图
			/**
			* 将输入图像从 BGR 颜色空间转换为灰度图像，并将结果存储在 tmpImg 中
			* 在 OpenCV 中，将 BGR 图像转换为灰度图像可以使用 cvtColor 函数，并将参数 COLOR_BGR2GRAY 作为转换方式。该函数会将每个像素的 B、G、R 三个通道的值加权平均，并将结果作为灰度值输出
			* gray = 0.299 * B + 0.587 * G + 0.114 * R
			* 一般情况下，无法从灰度图像中完全恢复原始的彩色图像
			**/
			cvtColor(srcImg, tmpImg, COLOR_BGR2GRAY);
#ifdef FIND_QRPOINT_DEBUG
			Show_Img(tmpImg);
#endif		
			//模糊全图，减少高频信息的干扰（尤其是摩尔纹）
			//实际上摩尔纹去除似乎还有更好的办法，考虑去掉一些高频率信息？
			/**
			* 在图像处理中，模糊操作（也称为滤波操作）可以去除图像中的噪声、平滑图像、模糊图像等。模糊核的大小决定了模糊的程度。
			  其中一个常用的公式是根据图像尺寸和模糊程度来计算模糊核的大小，即：
			  kernel_size = image_size * blur_rate
			  其中 kernel_size 表示模糊核的大小，image_size 表示图像的大小，可以是图像的宽度或高度，blur_rate 表示模糊程度，是一个介于 0 和 1 之间的参数，表示模糊程度与图像大小之比。
			  这个公式的基本原理是，当图像大小固定时，随着模糊程度的增加，模糊核的大小也应该相应增加，以保持合适的平滑效果。当图像大小不同的时候，根据实际需求和经验，可以调整 blur_rate 的值来得到适合的模糊核大小。
			  这个公式只是一种经验性的方法，实际的应用中还需要根据具体的场景和需求进行调整。同时，在图像处理中还有很多其他的滤波算法和技术，例如高斯滤波、中值滤波、双边滤波等，也需要根据实际需求来选择合适的方法和参数。
			**/
			// TODO: 可以考虑采用更好的滤波方式
			float BlurSize = 1.0 + srcImg.rows * blurRate; //计算图像模糊核的大小

			blur(tmpImg, tmpImg, Size2f(BlurSize, BlurSize));
#ifdef FIND_QRPOINT_DEBUG
			Show_Img(tmpImg);
#endif		
			//二值化
			//二值化通常是通过对灰度图像的阈值处理来实现的，即将灰度值大于某个阈值的像素设置为白色，其余像素设置为黑色
			//二值化会导致图像信息的丢失，因为在二值化过程中，所有灰度值被简化为了 0 和 255 两种取值。因此，二值化通常只适用于那些在黑白两色之间有明显分界的图像，对于颜色丰富、细节复杂的图像，二值化可能会导致信息的损失和失真
			threshold(tmpImg, tmpImg, 0, 255, THRESH_BINARY | THRESH_OTSU);
#ifdef FIND_QRPOINT_DEBUG
			Show_Img(tmpImg);
#endif
			return tmpImg;
		}
		//调用查找轮廓函数
		bool ScreenQrPoint(const Mat& srcImg, vector<vector<Point>>& qrPoints)
		{
			vector<vector<Point> > contours;
			/**
			* hierarchy[i][0]，hierarchy[i][1]，hierarchy[i][2]，hierarchy[i][3]分别表示轮廓(contour[i])的后一个轮廓、前一个轮廓、子轮廓和父轮廓的索引
			* 如果没有对应的后一个、前一个、子、父轮廓，则相应的索引值为-1。
			**/
			vector<Vec4i> hierarchy;
			findContours(srcImg, contours, hierarchy, RETR_TREE, CHAIN_APPROX_NONE, Point(0, 0));
			//通过黑色定位角作为父轮廓，有两个子轮廓的特点，筛选出三个定位角

			int parentIdx = -1; //父轮廓索引
			int ic = 0; //子轮廓数量

			for (int i = 0; i < contours.size(); i++)
			{
				// 如果当前轮廓有子轮廓，且子轮廓计数器为 0，则将其作为父轮廓
				if (hierarchy[i][2] != -1 && ic == 0)
				{
					parentIdx = i;
					ic++;
				}
				// 如果当前轮廓有子轮廓，子轮廓数量加一
				else if (hierarchy[i][2] != -1)
				{
					ic++;
				}
				// 如果当前轮廓没有子轮廓，则重置子轮廓数量和父轮廓索引
				else if (hierarchy[i][2] == -1)
				{
					ic = 0;
					parentIdx = -1;
				}
				if (ic == 2)
				{
					bool isQrPoint = IsQrPoint(contours[parentIdx], srcImg);
					//保存找到的三个黑色定位角
					if (isQrPoint)
						qrPoints.push_back(contours[parentIdx]);
					ic = 0;
					parentIdx = -1;
				}
			}
#ifdef FIND_QRPOINT_DEBUG
			printf("Find %d Points!", (int)qrPoints.size());
			printf(qrPoints.size() < 3 ? "Points is too less,Screen Failed!" : "\n");
#endif // FIND_QRPOINT_DEBUG
			return qrPoints.size() < 3;
		}

		// 检查由三个点组成的三角形中是否存在一个直角
		bool isRightAngleExist(const Point& point0, const Point& point1, const Point& point2)
		{
#ifdef FIND_QRPOINT_DEBUG
			static int counter = 0;
			float angle0 = Cal3PointAngle(point0, point1, point2), angle1 = Cal3PointAngle(point1, point0, point2), angle2 = Cal3PointAngle(point2, point0, point1);
			bool flag2 = isRightlAngle(angle0) || isRightlAngle(angle1) || isRightlAngle(angle2);
			printf("\nDumpPoints: %d angle0: %.2f angle1: %.2f,angle2: %.2f %s\n", ++counter, angle0, angle1, angle2, flag2 ? "Passed!" : "Failed!");
#endif
			return isRightlAngle(Cal3PointAngle(point0, point1, point2)) ||
				isRightlAngle(Cal3PointAngle(point1, point0, point2)) ||
				isRightlAngle(Cal3PointAngle(point2, point0, point1));
		}

		//找到最可能是定位点的三个点，将选择出来的三个点放在最前面
		bool DumpExcessQrPoint(vector<vector<Point>>& qrPoints)
		{	//目前的实现：排序后计算面积存在直角的方差最接近的三个点
			//可能以后有更好的实现？？
			// 
			//将 qrPoints 中的子向量按照它们包含的点数从小到大排序
			sort(
				qrPoints.begin(), qrPoints.end(),
				[](const vector<Point>& a, const vector<Point>& b) {return a.size() < b.size(); }
			);
			//按面积排序
			double mindis = INFINITY; //最小面积方差
			int pos = -1; //第三个点的索引
			Point Point0 = CalRectCenter(qrPoints[0]), Point1 = CalRectCenter(qrPoints[1]), Point2;
			for (int i = 2; i < qrPoints.size(); ++i)
			{
				bool tag = 0;
				if (!isRightAngleExist(Point2 = CalRectCenter(qrPoints[i]), Point1, Point0))
					tag = 1; //如果不构成直角，直接pass掉
				if (!tag)
				{
					auto temp = Cal3NumVariance(qrPoints[i].size(), qrPoints[i - 1].size(), qrPoints[i - 2].size());//计算面积方差，三个定位点的面积应相同
					if (mindis > temp)
					{
						mindis = temp;
						pos = i;
					}
				}
				Point0 = Point1;
				Point1 = Point2;
			}
			//如果pos==-1，则按大小排序后不存在夹角90度左右的识别点。
			if (pos == -1) return 1;
			else
			{
				vector<vector<Point>> temp =
				{
					std::move(qrPoints[pos - 2]),
					std::move(qrPoints[pos - 1]),
					std::move(qrPoints[pos])
				};
				for (int i = 0; i < pos - 2; ++i)
					temp.push_back(std::move(qrPoints[i]));
				for (int i = pos + 1; i < qrPoints.size(); ++i)
					temp.push_back(std::move(qrPoints[i]));
				std::swap(temp, qrPoints);
				//将选择出来的三个点放在最前面
				return 0;
			}
			//如不等于1，则找到成直角且面积方差最小的三个点
			//清除多余的点
		}

		//调整定位点的顺序 0 左上角 1 右上角 2左下角
		void AdjustPointsOrder(vector<vector<Point>>& src3Points)
		{
			vector<vector<Point>> temp;
			Point p3[3] = { CalRectCenter(src3Points[0]),CalRectCenter(src3Points[1]),CalRectCenter(src3Points[2]) };
			int index[3][3] = { { 0,1,2 },{1,0,2},{2,0,1} };
			for (int i = 0; i < 3; i++)
			{
				if (isRightlAngle(Cal3PointAngle(p3[index[i][0]], p3[index[i][1]], p3[index[i][2]])))
				{
					temp.push_back(std::move(src3Points[index[i][0]]));     //左上角的点位于0号 Red
					if (IsClockWise(p3[index[i][0]], p3[index[i][1]], p3[index[i][2]]))//判断1号点和2号点的顺逆时针关系
					{
						temp.push_back(std::move(src3Points[index[i][1]]));//右上角的点位于1号 Green
						temp.push_back(std::move(src3Points[index[i][2]]));//左下角的点位于2号 Blue
					}
					else
					{
						temp.push_back(std::move(src3Points[index[i][2]]));//右上角的点位于1号 Green
						temp.push_back(std::move(src3Points[index[i][1]]));//左下角的点位于2号 Blue
					}
					for (int i = 3; i < src3Points.size(); ++i)
						temp.push_back(std::move(src3Points[i]));  //移动其他的点
					std::swap(temp, src3Points);
					return;
				}
			}
			return;
		}

		//预处理srcImg，在五种模糊率的情况下，尝试扫描定位点，并将符合要求的三个定位点调整顺序后存储在qrPoints的最前面
		bool Main(const Mat& srcImg, vector<vector<Point>>& qrPoints)
		{
			//五种模糊率设置
			vector<vector<Point>> qrPointsTemp;
			std::array<float, 5> ar = { 0.0005,0.0000,0.00025,0.001,0.0001 };
			for (auto& rate : ar)//尝试不同的模糊率
			{
				//图像预处理,然后扫描定位点
				if (!ScreenQrPoint(ImgPreprocessing(srcImg, rate), qrPointsTemp))
				{
					if (qrPointsTemp.size() >= 4 && !DumpExcessQrPoint(qrPointsTemp))
					{
						qrPointsTemp.swap(qrPoints);
						AdjustPointsOrder(qrPoints);
						return 0;
					}
				}
			}
			return 1;
		}

		//将从srcImg中解析得到的定位点，并将其封装为ParseInfo类型
		bool Main(const Mat& srcImg, vector<ParseInfo>& Points3Info)
		{
			vector<vector<Point>> qrPoints;
			if (Main(srcImg, qrPoints)) return 1;
			for (auto& e : qrPoints)
				Points3Info.emplace_back(e);
			return 0;
		}

		//延伸由四个定位点中心所确定的矩形，得到要裁剪的区域，返回四个点
		vector<Point2f> Adjust3PointsToParallelogram(vector<ParseInfo> Points3Info)
		{
			float avglen = 0.0;
			vector<Point2f> ret;
			int id[4][3] = { { 0,1,2 },{1,0,3},{2,0,3},{3,1,2} };
			for (int i = 0; i < 3; ++i)
			{
				ret.push_back(Points3Info[i].Center);
				avglen += Points3Info[i].Rect.size.height;
				avglen += Points3Info[i].Rect.size.width;
			}
			/**
			* 得到的是要延伸的长度
			* 6.0是因为要计算三个点的总长度，除以sqrt(2.0)得到半对角线长度，0.8是一个经验值，用于调整最终的平均长度
			**/
			avglen /= 6.0 * sqrt(2.0) * 0.8;
			ret.push_back
			(
				CalForthPoint
				(
					Points3Info[0].Center,
					Points3Info[1].Center,
					Points3Info[2].Center
				)
			);
			pair<float, float> temp[4];
			// TODO: 是否需要修改CalExtendVec?
			for (int i = 0; i < 4; ++i)
				temp[i] = CalExtendVec(ret[id[i][0]], ret[id[i][1]], ret[id[i][2]], avglen);
			for (int i = 0; i < 4; ++i)
			{
				ret[i].x += temp[i].first;
				ret[i].y += temp[i].second;
			}
			return ret;
		}
		// 通过计算出的第四个定位点，从srcImg中裁剪出定位出的有效区域
		bool Main(const Mat& srcImg, Mat& disImg, vector<ParseInfo>& Points3Info)
		{
			if (Main(srcImg, Points3Info)) return 1;
			vector<float> avglen;
			for (auto& e : Points3Info)
				avglen.push_back((e.Rect.size.height + e.Rect.size.width) / 2.0);//似乎没有发挥作用
			disImg = CropParallelRect(srcImg, Adjust3PointsToParallelogram(Points3Info));
		}
		//将定位点填充颜色并用线连接，用于显示图像预处理后扫描得到的定位点的效果
		void __DisPlay(const char* ImgPath)
		{
			Mat srcImg = imread(ImgPath, 1);
			imshow("原始", srcImg);
			vector<vector<Point>> qrPoints;
			if (!Main(srcImg, qrPoints))
			{
				//填充定位点
				//左上角的点显示为红色，右上角的点显示为绿色，左下角的点显示为蓝色
				for (int i = 0, C = 0x00FF0000; i < qrPoints.size(); ++i, C >>= 8)
					drawContours(srcImg, vector<vector<Point>>{qrPoints[i]}, 0, CV_RGB(C >> 16, (C >> 8) & 0xFF, C & 0xFF), -1);

				Point2f point[3];
				for (int i = 0; i < qrPoints.size(); i++)
					point[i] = CalRectCenter(qrPoints[i]);
				line(srcImg, point[0], point[1], Scalar(255, 255, 0), 2);
				line(srcImg, point[1], point[2], Scalar(0, 255, 255), 2);
				line(srcImg, point[0], point[2], Scalar(255, 0, 255), 2);
				//连接定位点
				imshow("结果", srcImg);
			}
			waitKey(0);
		}
		//画出定位点的外界矩形并用线连接，显示封装为ParseInfo后的效果
		void __DisPlay2(const char* ImgPath)
		{
			Mat srcImg = imread(ImgPath, 1);
			imshow("原始", srcImg);
			vector<ParseInfo> qrPoints;
			if (!Main(srcImg, qrPoints))
			{
				line(srcImg, qrPoints[0].Center, qrPoints[1].Center, Scalar(0, 0, 255), 2);
				line(srcImg, qrPoints[1].Center, qrPoints[2].Center, Scalar(0, 255, 0), 2);
				line(srcImg, qrPoints[0].Center, qrPoints[2].Center, Scalar(255, 0, 0), 2);
				Point2f poi[4];
				for (auto& e : qrPoints) //画出定位点的外接矩形
				{
					e.Rect.points(poi);
					line(srcImg, poi[0], poi[1], Scalar(128, 128, 128), 2);
					line(srcImg, poi[1], poi[2], Scalar(128, 128, 128), 2);
					line(srcImg, poi[2], poi[3], Scalar(128, 128, 128), 2);
					line(srcImg, poi[3], poi[0], Scalar(128, 128, 128), 2);
				}
				//连接定位点
				imshow("结果", srcImg);
			}
			waitKey(0);
		}
		// 显示通过定位点裁剪出的有效区域
		void __DisPlay3(const char* ImgPath)
		{
			Mat srcImg = imread(ImgPath, 1), disImg;
			vector<ParseInfo>  Points3Info;
			if (Main(srcImg, disImg, Points3Info))
			{
				puts("ERR");
				return;
			}
			imshow("ans", disImg);
			waitKey(0);
		}
	}
	// 判断该轮廓的大小是否在接收范围内
	bool IsQrTypeRateLegal(float rate)
	{
		return rate <= MaxQrTypeRate && rate >= minQrTypeRate;
	}
	//判断该轮廓与左上角定位点的距离是否在接收范围内
	bool isLegalDistanceRate(float rate)
	{
		return rate <= MaxDistanceRate && rate >= minDistanceRate;
	}

	//找到第四个小定位点，处理结束后PointsInfo中只包含三个大定位点和一个小定位点
	bool FindForthPoint(vector<QrcodeParse::ParseInfo>& PointsInfo)
	{
		float avgSize = (PointsInfo[0].size + PointsInfo[1].size + PointsInfo[2].size) / 3.0;
		float expectDistance = distance(PointsInfo[0].Center, CalForthPoint(PointsInfo[0].Center, PointsInfo[1].Center, PointsInfo[2].Center));
		QrcodeParse::ParseInfo possiblePoints;
		bool tag = 1;
		for (int i = 3; i < PointsInfo.size(); ++i)
		{
#ifdef FIND_QRPOINT_DEBUG
#endif
			if (IsQrTypeRateLegal(avgSize / PointsInfo[i].size))//大小合适
				if (isRightlAngle(Cal3PointAngle(PointsInfo[i].Center, PointsInfo[1].Center, PointsInfo[2].Center))) //构成直角
					if (isLegalDistanceRate(distance(PointsInfo[i].Center, PointsInfo[0].Center) / expectDistance)) //距离合适
					{
						possiblePoints = std::move(PointsInfo[i]), tag = 0;
						break;
					}
		}

		if (tag) return 1;
		//删除第3个元素后的所有点，然后将possiblePoints添加到最后
		PointsInfo.erase(PointsInfo.begin() + 3, PointsInfo.end());
		PointsInfo.push_back(std::move(possiblePoints));
		return 0;
	}
	//通过三个大定位点和一个小定位点，裁剪出有效区域
	//tag 表示是否精确裁剪 若tag为false，则裁剪出一个包含有效区域的矩形用于下一次裁剪，否则直接裁剪出有效区域矩形
	vector<Point2f> AdjustForthPoint(const vector<QrcodeParse::ParseInfo> PointsInfo, bool tag)
	{
		float avglen = 0.0;
		vector<Point2f> ret;
		int id[4][3] = { { 0,1,2 },{1,0,3},{2,0,3},{3,1,2} };
		for (int i = 0; i < 3; ++i)
		{
			ret.push_back(PointsInfo[i].Center);
			avglen += PointsInfo[i].Rect.size.height;
			avglen += PointsInfo[i].Rect.size.width;
		}
		ret.push_back(PointsInfo[3].Center); //添加第四个小定位点
		avglen += PointsInfo[3].Rect.size.height * 2.0;
		avglen += PointsInfo[3].Rect.size.width * 2.0;
		avglen /= (tag) ? 8.0 : 2.0;
		if (tag) avglen = avglen / 14.0 * 9 * sqrt(2);
		pair<float, float> temp[4];
		for (int i = 0; i < 3; ++i)
			temp[i] = CalExtendVec(ret[id[i][0]], ret[id[i][1]], ret[id[i][2]], avglen);
		float forthCornerLen = avglen * ((tag) ? (11.0 / 18.0) : ((56.0 - 3.5 * sqrt(2.0)) / 56.0));
		temp[3] = CalExtendVec(ret[id[3][0]], ret[id[3][1]], ret[id[3][2]], forthCornerLen);
		for (int i = 0; i < 4; ++i)
		{
			ret[i].x += temp[i].first;
			ret[i].y += temp[i].second;
		}
		return ret;
	}
	//在一块区域内计算区分阈值，之后处理图像矩阵，大于该值则为白色，否则为黑色
	void GetVec(Mat& mat)
	{
		uint16_t minVec[3] = { 255,255,255 }, maxVec[3] = { 0, 0, 0 };
		Mat tempMat;
		// 计算区分阈值
		for (int i = 10; i < 100; ++i)
		{
			for (int j = 10; j < 100; ++j)
			{
				Vec3b& temp = mat.at<Vec3b>(i, j);
				minVec[0] = min(minVec[0], (uint16_t)temp[0]);
				minVec[1] = min(minVec[1], (uint16_t)temp[1]);
				minVec[2] = min(minVec[2], (uint16_t)temp[2]);
				maxVec[0] = max(maxVec[0], (uint16_t)temp[0]);
				maxVec[1] = max(maxVec[1], (uint16_t)temp[1]);
				maxVec[2] = max(maxVec[2], (uint16_t)temp[2]);
			}
		}
		float avg = (minVec[0] + maxVec[0] + minVec[1] + maxVec[1] + minVec[2] + maxVec[2]) / 6.0;
		// TODO: 此处需修改尺寸
		for (int i = 0; i < 1080; ++i)
		{
			for (int j = 0; j < 1920; ++j)
			{
				Vec3b& temp = mat.at<Vec3b>(i, j);
				float sum = (temp[0] + temp[1] + temp[2]) / 3.0;
				if (sum < avg) temp = Vec3b(0, 0, 0);
				else temp = Vec3b(255, 255, 255);
			}
		}
		return;
	}

	// 将矩阵中所有的连通块都找出来，并用二维布尔数组（ispass）标记为已访问，以便后续处理。
	void dfs(int i, int j, int limi, int limj, int* dir, bool(*ispass)[16], const Mat& mat)
	{
		if ((limi - i) * dir[0] > 0 || (limj - j) * dir[1] > 0) return;
		if ((limi - i) * dir[0] <= -16 || (limj - j) * dir[1] <= -16) return;
		if (ispass[(i - limi) * dir[0]][(j - limj) * dir[1]]) return;
		auto temp = mat.at<Vec3b>(i, j);
		if (temp[0] == temp[1] && temp[1] == temp[2] && temp[2] == 255)
		{
			ispass[(i - limi) * dir[0]][(j - limj) * dir[1]] = 1;
			dfs(i + 1 * dir[0], j, limi, limj, dir, ispass, mat);
			dfs(i - 1 * dir[0], j, limi, limj, dir, ispass, mat);
			dfs(i, j - 1 * dir[0], limi, limj, dir, ispass, mat);
			dfs(i, j + 1 * dir[0], limi, limj, dir, ispass, mat);
		}
	}
	// 深搜+暴搜 自定义寻找轮廓的函数
	vector<Point2f> FindConner(Mat& mat)
	{
		int dis = 0;
		int dir[4][2] = { {1,1},{-1,1},{1,-1},{-1,-1} };
		// TODO: 此处需修改坐标
		int poi[4][2] = { {0,0},{1079,0},{0,1919},{1079,1919} }; //角点的可能位置
		vector<Point2f> ret;
		for (int k = 0; k < 4; ++k)
		{
			bool ispass[16][16] = { 0 };
			dfs(poi[k][0] + 10 * dir[k][0], poi[k][1] + 10 * dir[k][1], poi[k][0], poi[k][1], dir[k], ispass, mat);
			for (dis = 0; dis <= 15; ++dis)
			{
				for (int i = 0; i <= dis; ++i)
				{
					int j = dis - i;
					auto temp = mat.at<Vec3b>(poi[k][0] + i * dir[k][0], poi[k][1] + j * dir[k][1]);
					if (temp[0] == temp[1] && temp[1] == temp[2] && temp[2] == 255 && ispass[i][j])
					{
						ret.emplace_back(poi[k][0] + i * dir[k][0], poi[k][1] + j * dir[k][1]);
						goto Final;
					}
				}
			}
		Final:
			if (dis == 100)
				break;
		}
		//Show_Img(mat);
		if (ret.size() != 4) return ret;
		std::swap(ret[1].x, ret[2].y);
		std::swap(ret[2].x, ret[1].y);
		std::swap(ret[3].x, ret[3].y);
		return ret;
	}
	// 缩放图片
	void Resize(Mat& mat)
	{
		// TODO: 此处需修改尺寸
		Mat temp = Mat(108, 192, CV_8UC3);
		for (int i = 0; i < 108; ++i)
		{
			for (int j = 0; j < 192; ++j)
			{
				int counter[8] = { 0 };
				for (int k = 4; k <= 5; ++k)
				{
					for (int l = 4; l <= 5; ++l)
					{
						int id = 0;
						const auto& vec = mat.at<Vec3b>(i * 10 + k, j * 10 + l);
						if (vec[0] == 255)
							id += 4;
						if (vec[1] == 255)
							id += 2;
						if (vec[2] == 255)
							id += 1;
						++counter[id];
					}
				}
				int maxpos = 0;
				for (int i = 1; i < 8; ++i)
					if (counter[maxpos] < counter[i])
						maxpos = i;
				temp.at<Vec3b>(i, j) = Vec3b((maxpos >> 2) * 255, ((maxpos >> 1) & 1) * 255, (maxpos & 1) * 255);
			}
		}
		mat = temp;
	}
	bool Main(const Mat& srcImg, Mat& disImg)
	{
		Mat temp;

		//以下为定位过程
		{
			//Show_Img(srcImg);


			vector<QrcodeParse::ParseInfo> PointsInfo;
			if (Main(srcImg, PointsInfo) || PointsInfo.size() < 4)
				return 1;

			if (FindForthPoint(PointsInfo)) return 1;
			//一阶裁剪，完成初步筛选
			temp = CropParallelRect(srcImg, AdjustForthPoint(PointsInfo, 0)); //裁剪一个包含有效区域的矩形，用于下一次裁剪
#ifdef FIND_QRPOINT_DEBUG
			Show_Img(temp);
#endif 
			disImg = CropParallelRect(srcImg, AdjustForthPoint(PointsInfo, 1)); //精确裁剪出有效区域
#ifdef FIND_QRPOINT_DEBUG
			Show_Img(disImg);
#endif 
			//二阶裁剪，完成实际映射，消除二阶像差
			PointsInfo.clear();
			//return 0;
			//对temp进行二次裁剪
			if (Main(temp, PointsInfo) || PointsInfo.size() < 4);//没有找到三个大定位点和可能是小定位点的轮廓
			else
			{//成果找到三个大定位点和可能是小定位点的轮廓
				if (FindForthPoint(PointsInfo)) return 0; //在定位结果中未找到第四个定位点，直接输出disImg
				disImg = CropParallelRect(temp, AdjustForthPoint(PointsInfo, 1)); //找到第四个定位点，进行精确裁剪
			}
			//Show_Img(disImg);
			//如果二阶裁剪失败根据一阶裁剪的信息返回一个不知道是否精确的结果。
			//三阶微调，完成最终矫正
			disImg.copyTo(temp);
			//TODO: 此处需要修改尺寸
			cv::resize(temp, temp, Size(1920, 1080));
			GetVec(temp);//锐化处理
			auto poi4 = FindConner(temp); //手动寻找地位点
			if (poi4.size() != 4) return 1;
			//TODO: 此处需要修改尺寸
			cv::resize(disImg, disImg, Size(1920, 1080));
			temp = CropParallelRect(disImg, poi4, Size(1920, 1080));
		}

		disImg = temp;
#ifdef FIND_QRPOINT_DEBUG
		Show_Img(disImg);
#endif 
		GetVec(disImg);
		Resize(disImg);
#ifdef FIND_QRPOINT_DEBUG
		Show_Img(disImg);
#endif 
		return 0;
	}
	//void __DisPlay(const char *ImgPath)
	//{
	//	Mat srcImg = imread(ImgPath, 1), disImg;
	//	if (Main(srcImg, disImg))
	//	{
	//		puts("ERR");
	//		return;
	//	}
	//	imshow("ans", disImg);
	//	waitKey(0);
	//}
}
