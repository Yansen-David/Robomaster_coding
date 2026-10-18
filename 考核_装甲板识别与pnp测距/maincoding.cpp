#include<iostream>
#include<opencv2/opencv.hpp>
#include<string>
#include<vector>
#include<cmath>
#include<algorithm>

//int hmin1 = 100, hmax1 = 124;
//int smin1 = 50, smax1 = 255;
//int vmin1 = 100, vmax1 = 255;

//我发现灯带发一种偏青的蓝光，过滤掉蓝色，发现效果极佳(我一开始的想法)

int hmin2 = 78, hmax2 = 99;
int smin2 = 50, smax2 = 255;
int vmin2 = 100, vmax2 = 255;

int main() {
	system("chcp 65001");

	//---pnp测算前，数据准备
	cv::FileStorage fs("C:/OpenCV_Data/second_test.yml", cv::FileStorage::READ);//因为我相机标定是在没有找到报错原因，所以我先用了我手机摄像头的参数
	cv::Mat neican, jibianxishu;
	fs["neican"] >> neican;
	fs["jibianxishu"] >> jibianxishu;
	double armor_h = 56e-3;//同时准备好现实的物理距离
	double armor_w = 135e-3;


	cv::Mat src,src_hsv,src_thresholdblue,src_thresholdqing,src_threshold_and,src_threshold;

	cv::VideoCapture video("C:/OpenCV_Data/second_test_video/test_video.avi");

	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(2, 5));

	while (true) {
		video >> src;
		if (src.empty()) {
			std::cout << "已退出" << std::endl;
			break;
		}
		cv::Mat src1 = src.clone(), src2 = src.clone();

		//---要在循环内部测算实时的pnp距离,就要搭建实时的3D，2D坐标系
		std::vector<cv::Point3f>point3D;
		std::vector<cv::Point2f>point2D;
		
		//图像预处理
		cv::cvtColor(src, src_hsv, cv::COLOR_BGR2HSV);
		//cv::inRange(src_hsv, cv::Scalar(hmin1, smin1, vmin1), cv::Scalar(hmax1, smax1, vmax1), src_thresholdblue);
		cv::inRange(src_hsv, cv::Scalar(hmin2, smin2, vmin2), cv::Scalar(hmax2, smax2, vmax2), src_thresholdqing);
		//cv::bitwise_or(src_thresholdblue, src_thresholdqing, src_threshold_and);
		cv::morphologyEx(src_thresholdqing, src_threshold, cv::MORPH_OPEN, kernel);

		//边缘提取
		std::vector<std::vector<cv::Point>>contours;
		cv::findContours(src_threshold, contours, cv::noArray(), cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

		//准备封装合法灯带
		std::vector<cv::RotatedRect>legit_lightbar;
		
		for (size_t i = 0; i < contours.size(); i++) {
			cv::RotatedRect lightbar = cv::minAreaRect(contours[i]);
			cv::Point2f lightbar_points[4];
			lightbar.points(lightbar_points);

			double area = cv::contourArea(contours[i]);
			if (area < 1)continue;//去除小噪点
			if (area > 100)continue;//筛选掉大灯带
			if (lightbar.center.y < 512||lightbar.center.y>768)continue;

			for (int j = 0;j < 4;++j) {
				cv::line(src1, cv::Point2f(lightbar_points[j]), cv::Point2f(lightbar_points[(j + 1) % 4]), cv::Scalar(0, 255, 0), 3);
			}
			legit_lightbar.push_back(lightbar);//封装合法灯带
		}

		//绘制装甲板
		for (size_t i = 0;i < legit_lightbar.size();++i) {
			for (size_t j = i + 1;j < legit_lightbar.size();++j) {
				cv::RotatedRect r1 = legit_lightbar[i];
				cv::RotatedRect r2 = legit_lightbar[j];

				//再次通过一一配对的方式，判断看是否合适作为装甲板灯带
				float x_distance = std::abs(r1.center.x - r2.center.x);
				float y_distance = std::abs(r1.center.y - r2.center.y);

				if (x_distance > 50)continue;
				if (y_distance > 20)continue;
				if (x_distance < 30)continue;
				//其实这里我好像没有办法像图片一样知道像素点的距离，可能是因为视频帧率的原因，导致灯带的像素点距离不够准确
				
				//其实封装合法灯带时就已经处理了大部分噪点了

				//将配对合格的灯带点一一取出，重新封装到一个新的矩形，称它为装甲板
				cv::Point2f p1[4], p2[4];
				r1.points(p1);
				r2.points(p2);
				std::vector<cv::Point2f>armor_points;
				for (int k = 0;k < 4;++k) {
					armor_points.push_back(p1[k]);
					armor_points.push_back(p2[k]);
				}
				cv::RotatedRect armor= cv::minAreaRect(armor_points);//得到装甲板
				//绘制装甲板
				cv::Point2f armor_4points[4];
				armor.points(armor_4points);
				for (int p = 0;p < 4;++p) {
					cv::line(src2, cv::Point2f(armor_4points[p]), cv::Point2f(armor_4points[(p + 1) % 4]), cv::Scalar(0, 0, 255), 3);
				}
				cv::circle(src2, armor.center, 8, cv::Scalar(0, 0, 255), -1);
				//std::cout << "装甲板在图像中的中心点坐标为：" << armor.center << std::endl;    这段不建议打印了，太多了，看不来

				//将装甲板的3D坐标和2D坐标封装到对应的容器中
				//2D坐标系的点需要先排好序，才能进行pnp测算
				cv::Point2f TL, BL, BR, TR;
				TL = armor_4points[0];
				BL = armor_4points[0];
				BR = armor_4points[0];
				TR = armor_4points[0];
				float o_sum = armor_4points[0].x + armor_4points[0].y;
				float o_diff = armor_4points[0].x - armor_4points[0].y;

				for (int t = 0;t < 4;++t) {
					float sum= armor_4points[t].x + armor_4points[t].y;
					float diff = armor_4points[t].x - armor_4points[t].y;

					if (sum < o_sum) {
						TL = armor_4points[t];
						float min_sum = sum;
					}

					if (sum > o_sum) {
						BR = armor_4points[t];
						float max_sum = sum;
					}

					if (diff < o_diff) {
						BL = armor_4points[t];
						float min_diff = diff;
					}

					if (diff > o_diff) {
						TR = armor_4points[t];
						float max_diff = diff;
					}
				}

				point2D.push_back(TL);
				point2D.push_back(BL);
				point2D.push_back(BR);
				point2D.push_back(TR);
				//按顺序封装3D点
				point3D.push_back(cv::Point3f(-armor_w / 2, -armor_h / 2, 0));//左上
				point3D.push_back(cv::Point3f(-armor_w / 2, armor_h / 2, 0));//左下
				point3D.push_back(cv::Point3f(armor_w / 2, armor_h / 2, 0));//右下
				point3D.push_back(cv::Point3f(armor_w / 2, -armor_h / 2, 0));//右上

				//执行pnp测算，这种感觉类似于python中的numpy的数组自动对应
				cv::Mat Rvecs, Tvecs;//这两个外参是一次性的，写在循环内部
				cv::solvePnP(point3D, point2D, neican, jibianxishu, Rvecs, Tvecs);
				std::cout << "测得装甲板到摄像头的距离为：" << cv::norm(Tvecs) << std::endl;
			}
		}

		cv::imshow("test_threshold", src_threshold);//可以知道预处理的效果如何
		cv::imshow("test1", src1);//可以看到哪些灯带被选中
		cv::imshow("test2", src2);//可以看到哪些装甲板被选中
		int key = cv::waitKey(25);
		if (key == 27)break;
	}


	cv::destroyAllWindows();
	system("pause");
	return 0;
}