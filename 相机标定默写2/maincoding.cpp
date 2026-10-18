#include<iostream>
#include<opencv2/opencv.hpp>
#include<vector>
#include<string>

//设置标定参数
float gap = 23.0f;
int pattern_width = 10;
int pattern_height = 7;
cv::Size pattern(pattern_width, pattern_height);
cv::Size src_size(1706, 1279);

int main() {
	system("chcp 65001");
	//生成世界坐标系
	std::vector<cv::Point3f>world;
	for (int i = 0;i < pattern_height;i++) {
		for (int j = 0;j < pattern_width;j++) {
			world.push_back(cv::Point3f(j * gap, i * gap, 0));
		}
	}

	//准备存放世界坐标系和图像像素坐标系的容器
	std::vector<std::vector<cv::Point3f>>point3D;
	std::vector<std::vector<cv::Point2f>>point2D;

	//读取文件，找标定点
	std::vector<cv::String>files;
	cv::glob("C:/OpenCV_Data/test_images_chessboard/*.jpg", files);
	if (files.empty())return -1;

	int success_count = 0;

	for (size_t i = 0;i < files.size();i++) {
		cv::Mat src_gray = cv::imread(files[i], 0);
		if (src_gray.empty())return -1;

		std::vector<cv::Point2f>Corners;
		bool isFound = cv::findChessboardCorners(src_gray, pattern, Corners);

		//执行标定
		if (isFound) {
			cv::TermCriteria criteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.1);
			cv::cornerSubPix(src_gray, Corners, cv::Size(11, 11), cv::Size(-1, -1), criteria);
			point3D.push_back(world);
			point2D.push_back(Corners);
			success_count++;
			std::cout << "成功读取第" << success_count << "张图片" << std::endl;
		}
		else {
			std::cout << "读取失败图片(" << files[i] << ")" << std::endl;
		}
	}

	if (success_count < 5) {
		std::cout << "读取成功不足五张，无法执行计算" << std::endl;
		return -1;
	}
	
	std::vector<cv::Mat>Rvecs, Tvecs;
	cv::Mat neican, jibianxishu;
	double RMS = cv::calibrateCamera(point3D, point2D, src_size, neican,jibianxishu, Rvecs, Tvecs);

	std::cout << "\n========================================" << std::endl;
	std::cout << "✅ 标定完成！" << std::endl;
	std::cout << "重投影误差 (RMS): " << RMS << " (越小越好，<1.0 算合格)" << std::endl;
	std::cout << "内参矩阵:\n" << neican << std::endl;
	std::cout << "畸变系数:\n" << jibianxishu << std::endl;
	std::cout << "========================================" << std::endl;

	//保存
	cv::FileStorage fs("C:/OpenCV_Data/moxie.yml", cv::FileStorage::WRITE);
	if (!fs.isOpened()) {
		std::cout << "保存失败" << std::endl;
		return -1;
	}
	fs << "neican" << neican;
	fs << "jibianxishu" << jibianxishu;
	fs.release();
	std::cout << "保存成功" << std::endl;

	//没要求掌握，畸变读取与去畸变
	cv::Mat sample_jibian = cv::imread("C:/OpenCV_Data/Robomaster_samples/light_line.jpg", 1);
	if (sample_jibian.empty())return -1;
	cv::Mat sample_finished(4096, 3072, CV_8UC3);
	cv::undistort(sample_jibian, sample_finished, neican, jibianxishu);
	cv::imshow("orginal", sample_jibian);
	cv::imshow("dst", sample_finished);

	std::cout << "按任意键退出" << std::endl;
	cv::waitKey(0);
	cv::destroyAllWindows();
	system("pause");
	return 0;
}