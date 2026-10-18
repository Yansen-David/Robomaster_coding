#include <opencv2/opencv.hpp>
#include <iostream>
#include<vector>
#include<cmath>
#include<algorithm>

int main() {
	system("chcp 65001");

	//pnp准备数据
	cv::Mat neican, jibianxishu;
	cv::FileStorage fs("C:/OpenCV_Data/camera_neicananddiancan.yml", cv::FileStorage::READ);
	if (!fs.isOpened()) {
		std::cout << "标定参数读取失败，检查路径或文件名称"<<std::endl;
		return -1;
	}
	fs["neican"] >> neican;
	fs["jibianxishu"] >> jibianxishu;
	fs.release();


	//图像预处理
	cv::Mat src_color = cv::imread("C:/OpenCV_Data/Robomaster_samples/blue_test.jpg", 1);
	if (src_color.empty()) {
		std::cout << "原图读取失败" << std::endl;
		return -1;
	}
	cv::Mat src_hsv;
	cv::cvtColor(src_color, src_hsv, cv::COLOR_BGR2HSV);

	int hmin = 100, hmax = 124;
	int smin = 100, smax = 255;
	int vmin = 120, vmax = 255;

	cv::Mat src_threshold;
	cv::inRange(src_hsv, cv::Scalar(hmin, smin, vmin), cv::Scalar(hmax, smax, vmax), src_threshold);

	cv::Mat src_threshold_end;
	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(15, 50));
	cv::morphologyEx(src_threshold, src_threshold_end, cv::MORPH_CLOSE, kernel);


	cv::Mat src_color2 = src_color.clone();
	std::vector<cv::RotatedRect>legit;//封装合格的灯带
	//装甲板识别判断
	std::vector<std::vector<cv::Point>>contours;
	cv::findContours(src_threshold_end, contours, cv::noArray(), cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

	int lightbar_num = 0;

	for (size_t i = 0;i < contours.size();i++) {
		double area = cv::contourArea(contours[i]);
		if (area > 1700)continue;
		
		cv::RotatedRect rotatedRect = cv::minAreaRect(contours[i]);
		float w = rotatedRect.size.width;
		float h = rotatedRect.size.height;
		if (w > h) {
			float temp = h;h = w;w = temp;
		}
		float ratio_h_w = h / w;

		if (ratio_h_w > 9.0)continue;
		if (h > 80 || h < 50)continue;

		lightbar_num++;
		cv::Point2f first_4point[4];
		rotatedRect.points(first_4point);
		for (int k = 0;k < 4;k++) {
			cv::line(src_color2, first_4point[k], first_4point[(k + 1) % 4], cv::Scalar(0, 255, 0), 3);
		}
		legit.push_back(rotatedRect);//封装合格的灯带
	}
	std::cout << "共找到" << lightbar_num << "个待选灯条" << std::endl;


	cv::Mat src_color3 = src_color.clone();
	////第三步，装甲板拼接
	int armor_num = 0;
	for (size_t i = 0;i < legit.size();i++) {
		for (size_t j = i + 1;j < legit.size();j++) {
			cv::RotatedRect r1 = legit[i];
			cv::RotatedRect r2 = legit[j];

			float x_gap = std::abs(r1.center.x - r2.center.x);
			float y_gap = std::abs(r1.center.y - r2.center.y);

			float h_max = std::max(r1.size.height, r2.size.height);
			float h_min = std::min(r1.size.height, r2.size.height);

			if (x_gap < 90 || x_gap>163)continue;
			if (y_gap > 20)continue;
			//统计
			armor_num++;
			//重新封装
			std::vector<cv::Point2f>armor_points;
			cv::Point2f p1[4], p2[4];
			r1.points(p1);
			r2.points(p2);
			for (int k = 0;k < 4;k++) {
				armor_points.push_back(p1[k]);
				armor_points.push_back(p2[k]);

			}
			cv::RotatedRect armor_rect = cv::minAreaRect(armor_points);
			//画线
			cv::Point2f armor_rect_4point[4];
			armor_rect.points(armor_rect_4point);
			for (int k = 0;k < 4;k++) {
				cv::line(src_color3, armor_rect_4point[k], armor_rect_4point[(k + 1) % 4], cv::Scalar(0, 0, 255), 3);
			}
			//画点
			cv::Point2f armor_center = (r1.center + r2.center) / 2;
			cv::circle(src_color3, armor_center, 8, cv::Scalar(0, 0, 255), -1);
			
			std::cout << "第" << armor_num << "个装甲板配对成功" << std::endl;
			std::cout << "中心点坐标:" << armor_center << std::endl;

			//pnp定义3D点阵
			float armor_width = 135.0f;
			float armor_height = 55.0f;
			std::vector<cv::Point3f>point3D;
			point3D.push_back(cv::Point3f(-armor_width / 2, -armor_height / 2, 0));
			point3D.push_back(cv::Point3f(armor_width / 2, -armor_height / 2, 0));
			point3D.push_back(cv::Point3f(armor_width / 2, armor_height / 2, 0));
			point3D.push_back(cv::Point3f(-armor_width / 2, armor_height / 2, 0));
			//pnp---定义2D点阵
			cv::Point2f zs, ys, zx, yx;
			zs = armor_rect_4point[0];
			ys = armor_rect_4point[0];
			yx = armor_rect_4point[0];
			zx = armor_rect_4point[0];

			float min_sum = armor_rect_4point[0].x + armor_rect_4point[0].y;
			float max_sum = armor_rect_4point[0].x + armor_rect_4point[0].y;
			float min_diff = armor_rect_4point[0].x - armor_rect_4point[0].y;
			float max_diff = armor_rect_4point[0].x - armor_rect_4point[0].y;

			for (int k = 0;k < 4;k++) {
				float sum = armor_rect_4point[k].x + armor_rect_4point[k].y;
				float diff = armor_rect_4point[k].x - armor_rect_4point[k].y;

				if (sum < min_sum) {
					min_sum = sum;
					zs = armor_rect_4point[k];
				}

				if (sum > max_sum) {
					max_sum = sum;
					zx = armor_rect_4point[k];
				}

				if (diff < min_diff) {
					min_diff = sum;
					zx = armor_rect_4point[k];
				}

				if (diff > max_sum) {
					min_sum = sum;
					ys = armor_rect_4point[k];
				}
			}

			std::vector<cv::Point2f>point2D;
			point2D.push_back(zs);
			point2D.push_back(ys);
			point2D.push_back(yx);
			point2D.push_back(zx);
			//pnp计算
			cv::Mat Rvecs, Tvecs;
			bool success = cv::solvePnP(point3D, point2D, neican, jibianxishu, Rvecs, Tvecs);
			if (success) {
				double distance = cv::norm(Tvecs);
				std::cout << "pnp测得相机与第" << armor_num << "个装甲板距离为：" << distance << " mm" << std::endl;
			}
		}
	}




	cv::imshow("threshold", src_threshold_end);
	cv::imshow("selected lightbar", src_color2);
	cv::imshow("end", src_color3);


	std::cout << "按任意键退出" << std::endl;
	cv::waitKey(0);
	cv::destroyAllWindows();
	return 0;
}