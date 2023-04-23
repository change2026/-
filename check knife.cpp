#include<iostream>
#include<opencv2/opencv.hpp>

using namespace std;
using namespace cv;

Mat tpl;//获取完整的模板

void sort_box(vector<Rect>&boxes);
void detect_defect(Mat&binary, vector<Rect>rects,vector<Rect>&defect);
int main(int argc, char** argv)
{
	Mat src = imread("D:/Image/IMAGE/opencv_tutorial_data-master/images/ce_01.jpg");
	if (src.empty())
	{
		printf("can't load image");
		return -1;
	}
	namedWindow("input", WINDOW_AUTOSIZE);
	imshow("src", src);
	Mat gray, binary;
	//颜色转换
	cvtColor(src, gray, COLOR_BGR2GRAY);
	//二值化
	threshold(gray, binary, 0, 255, THRESH_BINARY_INV | THRESH_OTSU);
	//imshow("binary", binary);
	//形态学开操作，先腐蚀在膨胀  去掉一些细微点
	Mat se = getStructuringElement(MORPH_RECT, Size(3, 3), Point(-1, -1));
	morphologyEx(binary, binary, MORPH_OPEN, se);
	//imshow("dst", binary);
	//轮廓发现
	vector<vector<Point>>contours;
	vector<Vec4i>hierachy;
	vector<Rect>rects;
	findContours(binary, contours, hierachy, RETR_LIST, CHAIN_APPROX_SIMPLE);
	int height = src.rows;
	for (size_t t = 0; t < contours.size(); t++)
	{
		Rect rect = boundingRect(contours[t]);
		double area = contourArea(contours[t]);
		double len = arcLength(contours[t],true);
		if (rect.height > (height/2))continue;
		if (area < 150)continue;
		rects.push_back(rect);
		//rectangle(src, rect, Scalar(0, 0, 255), 2, 8, 0);
		drawContours(src, contours, t, Scalar(0, 0, 255), 2, 8);
	}
	
	sort_box(rects);
	tpl = binary(rects[1]);
	vector<Rect>defects;
	detect_defect(binary, rects, defects);
	for (int i = 0; i < defects.size(); i++)
	{
		rectangle(src, defects[i], Scalar(0, 0, 255), 2, 8);
		putText(src, format("bad:%d\n", i), defects[i].tl(), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 255, 0), 2, 8); 
	}
	imshow("result", src);
	waitKey(0);
	destroyAllWindows();
	return 0;
}
void sort_box(vector<Rect>&boxes)
{
	int size = boxes.size();
	for(int i=0;i<size-1;i++)
	{
		for (int j = i; j < size; j++)
		{
			int x = boxes[j].x;
			int y = boxes[j].y;
			if (y < boxes[i].y)
			{
				Rect temp = boxes[i];
				boxes[i] = boxes[j];
				boxes[j] = temp;

			}
		}
	}
}
void detect_defect(Mat&binary, vector<Rect>rects, vector<Rect>&defect)
{
	int h = tpl.rows;
	int w = tpl.cols;
	int size = rects.size();
	for (int i = 0; i < size; i++)
	{
		//构建diff
		Mat roi = binary(rects[i]);
		resize(roi, roi, tpl.size());
		Mat mask;
		subtract(tpl, roi, mask);
		Mat se = getStructuringElement(MORPH_RECT, Size(3, 3), Point(-1, -1));
		morphologyEx(mask, mask, MORPH_OPEN, se);
		threshold(mask, mask, 0, 255, THRESH_BINARY);
		imshow("mask", mask);
		waitKey(0);
		//根据diff查找缺陷，阈值化。
		int count = 0;
		for (int row = 0; row < h; row++)
		{
			for (int col = 0; col < w; col++)
			{
				int pv = mask.at<uchar>(row, col);
				if (pv == 255)
				{
					count++;
				}
			}
		}
		//填充一个像素宽
		int mh = mask.rows + 2;
		int mw = mask.cols + 2;
		Mat m1 = Mat::zeros(Size(mw, mh), mask.type());
		Rect mroi;
		mroi.x = 1;
		mroi.y = 1;
		mroi.height = mask.rows;
		mroi.width = mask.cols;
		mask.copyTo(m1(mroi));
		//轮廓分析
		vector<vector<Point>>contours;
		vector<Vec4i>hierachy;
		findContours(m1, contours, hierachy, RETR_LIST, CHAIN_APPROX_SIMPLE);
		bool find = false;
		for(size_t t=0;t<contours.size();t++)
		{
			Rect rect = boundingRect(contours[t]);
			float ratio = (float)rect.width / ((float)rect.height);
			if (ratio > 4.0 && (rect.y < 5 || (m1.rows - (rect.height + rect.y)) < 10))
			{
				continue;
			}
			double area = contourArea(contours[t]);
			if (area > 10)
			{
				printf("ratio:%.2f,area:%.2f\n", ratio, area);
                find = true;
			}
		}
		if (count > 50&&find)
		{
			printf("count:%d\n", count);
			defect.push_back(rects[i]);
		}
	}
	//返回结果
}