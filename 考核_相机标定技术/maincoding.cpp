#include<iostream>
#include<opencv2/opencv.hpp>
#include<string>
#include<vector>

int main() {
	system("chcp 65001");

	//相机标定参数列出
	float gaps = 50.0f;//真实的格子间距，单位mm
	int pattern_w = 11;
	int pattern_h = 8;
	cv::Size pattern(pattern_w, pattern_h);
	cv::Size src_size(1280,1024);//这是相机的分辨率

	//定义一个世界坐标系
	std::vector<cv::Point3f>world;
	for (int i = 0;i < pattern_h;++i) {
		for (int j = 0;j < pattern_w;++j) {
			world.push_back(cv::Point3f(pattern_w * gaps, pattern_h * gaps, 0));
		}
	}

	//开始读取文件
	cv::Mat src;
	std::vector<cv::String>Files;
	cv::glob("C:/OpenCV_Data/second_test_photo/*.png", Files);
	if (Files.empty()) {
		std::cout << "文件读取失败，检查路径" << std::endl;
		return -1;
	}
	
	//准备3D与2D容器，计算世界坐标系与图像像素坐标系的关系
	std::vector<std::vector<cv::Point3f>>point3D;
	std::vector<std::vector<cv::Point2f>>point2D;

	//找标定点
	int find_success = 0;
	cv::TermCriteria criteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.1);
	for (size_t i = 0;i < Files.size();++i) {
		src = cv::imread(Files[i], 0);
		std::vector<cv::Point2f>corners;
		bool found = cv::findChessboardCorners(src, pattern, corners);
		if (found) {
			cv::cornerSubPix(src, corners, cv::Size(11, 11), cv::Size(-1, -1), criteria);
			point3D.push_back(world);//这里类似于python中numpy的数组一一对应，按顺序自动对应
			point2D.push_back(corners);
			std::cout << "第" << i + 1 << "图片处理成功" << std::endl;
			find_success++;
		}
		else {
			std::cout << "无法处理图片(" << Files[i] << ")" << std::endl;
		}
	}

	if (find_success < 10) {
		std::cout << "成功标定的图片不足十张，无法执行标定" << std::endl;
		return -1;
	}

	//执行标定
	std::vector<cv::Mat>Rvecs, Tvecs;
	cv::Mat neican;
	cv::Mat jibianxishu;
	double RMS = cv::calibrateCamera(point3D, point2D, src_size, neican, jibianxishu, Rvecs, Tvecs);

	std::cout << "-----------------------" << std::endl;
	std::cout << "相机标定完成，数据如下" << std::endl;
	std::cout << "RMS： " << RMS << std::endl;
	std::cout << "内参矩阵：" << neican << std::endl;
	std::cout << "畸变系数：" << jibianxishu << std::endl;
	std::cout << "-----------------------" << std::endl;

	//存储测定的相机数据
	cv::FileStorage fs("C:/OpenCV_Data/second_test.yml", cv::FileStorage::WRITE);
	if (!fs.isOpened()) {
		std::cout << "保存失败，检查保存路径" << std::endl;
		return -1;
	}
	fs << "neican" << neican;
	fs << "jibianxishu" << jibianxishu;
	fs.release();
	std::cout << "保存成功" << std::endl;


	cv::destroyAllWindows();
	cv::waitKey(0);
	system("pause");
	return 0;
}