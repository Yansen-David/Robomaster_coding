#include <opencv2/opencv.hpp>
#include <iostream>
#include<vector>
#include<cmath>
#include<algorithm>

int main() {
	system("chcp 65001");

	cv::VideoCapture redVideo("C:/OpenCV_Data/videos/red.mp4");
	cv::Mat src, src_gray, src_threshold1, src_threshold2, src_threshold, src_threshold_end;
	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_OPEN, cv::Size(3, 5));


	int hmin1 = 156, hmax1 = 180;
	int hmin2 = 0, hmax2 = 15;
	int smin = 43, smax = 255;
	int vmin = 46, vmax = 255;


	while (true) {
		redVideo >> src;
		if (src.empty()) {
			std::cout << "视频播放结束" << std::endl;
			break;
		}
		std::vector<cv::RotatedRect>legit;

		cv::cvtColor(src, src_gray, cv::COLOR_BGR2HSV);
		cv::inRange(src_gray, cv::Scalar(hmin1, smin, vmin), cv::Scalar(hmax1, smax, vmax), src_threshold1);
		cv::inRange(src_gray, cv::Scalar(hmin2, smin, vmin), cv::Scalar(hmax2, smax, vmax), src_threshold2);
		cv::bitwise_or(src_threshold1, src_threshold2, src_threshold);

		cv::morphologyEx(src_threshold, src_threshold_end, cv::MORPH_CLOSE, kernel);

		std::vector<std::vector<cv::Point>>contours;
		cv::findContours(src_threshold_end, contours, cv::noArray(), cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

		for (size_t i = 0;i < contours.size();i++) {
			double area = cv::contourArea(contours[i]);
			if (area < 50)continue;

			cv::RotatedRect rotatedRect_lightbar = cv::minAreaRect(contours[i]);
			legit.push_back(rotatedRect_lightbar);
		}

		for (size_t i = 0;i < legit.size();i++) {
			for (size_t j = i+1;j < legit.size();j++) {
				std::vector<cv::Point2f>armor_point;
				cv::Point2f p1[4], p2[4];
				legit[i].points(p1);
				legit[j].points(p2);

				float diff = std::abs(legit[i].center.y - legit[j].center.y);
				if (diff > 25)continue;

				for (int k = 0;k < 4;k++) {
					armor_point.push_back(p1[k]);
					armor_point.push_back(p2[k]);
				}
				cv::RotatedRect armor_rect = cv::minAreaRect(armor_point);
				cv::Point2f armor_rect_4point[4];
				armor_rect.points(armor_rect_4point);
				for (int t = 0;t < 4;t++) {
					cv::line(src, armor_rect_4point[t], armor_rect_4point[(t + 1) % 4], cv::Scalar(0, 255, 0), 3);
				}
				cv::circle(src, armor_rect.center, 8, cv::Scalar(0, 255, 0), -1);
				std::cout << "装甲板在图像中的中点坐标为：" << armor_rect.center << std::endl;
			}
		}
		cv::imshow("red_armor", src);

		int Key=cv::waitKey(1);
		if (Key == 27) {
			break;
		}
	}

	cv::destroyAllWindows();
	system("pause");
	return 0;
}