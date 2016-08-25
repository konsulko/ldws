/*
 * Copyright 2016 Konsulko Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include <opencv2/core/cuda.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudafilters.hpp>
#include <opencv2/cudaimgproc.hpp>
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <tclap/CmdLine.h>
#include <libconfig.h++>
#include <iostream>
#include <string>

#include "config.h"
#include "config_store.h"
#include "fps.h"

using namespace std;
using namespace cv;
using namespace libconfig;

struct Lane {
	Lane(){}
	Lane(Point a, Point b, float angle, float kl, float bl): p0(a),p1(b),angle(angle),
	votes(0),visited(false),found(false),k(kl),b(bl) { }

	Point p0, p1;
	int votes;
	bool visited, found;
	float angle, k, b;
};

void ProcessLanes(vector<Vec4i> lines, Mat frame, Point roi, ConfigStore *cs)
{
	vector<Lane> left, right;

	for(int i = 0; i < lines.size(); i++ )
	{
		Point pt1 = Point(lines[i][0], lines[i][1]), pt2 = Point(lines[i][2], lines[i][3]);
		int dx = pt2.x - pt1.x;
		int dy = pt2.y - pt1.y;
		float angle = atan2f(dy, dx) * 180/CV_PI;
		// FIXME get min angle from defaults and config option
		if (fabs(angle) < 20) {
			continue;
		}

		// assume that vanishing point is close to the image horizontal center
		// calculate line parameters: y = kx + b;
		dx = (dx == 0) ? 1 : dx; // prevent DIV/0!
		float k = dy/(float)dx;
		float b = pt1.y - k*pt1.x;

		// Categorize lines per side based on frame midpoint
		int midx = (pt1.x + pt2.x) / 2;
		if (midx < frame.cols/2) {
			left.push_back(Lane(pt1, pt2, angle, k, b));
		} else if (midx > frame.cols/2) {
			right.push_back(Lane(pt1, pt2, angle, k, b));
		}
	}

	// Draw candidate lines
	if (cs->intermediate_display) {
		for	(int i=0; i<right.size(); i++) {
			line(frame, right[i].p0 + roi, right[i].p1 + roi, CV_RGB(0, 0, 255), 2);
		}

		for	(int i=0; i<left.size(); i++) {
			line(frame, left[i].p0 + roi, left[i].p1 + roi, CV_RGB(255, 0, 0), 2);
		}
	}

	// TODO ProcessSides
	//ProcessSides(left, edge, false);
	//ProcessSides(right, edge, true);

	// TODO Draw lane guides
}

int main(int argc, char* argv[])
{
	ConfigStore *cs = ConfigStore::GetInstance();

	// Parse command line options
	try {
		TCLAP::CmdLine cmd_line("Lane Departure Warning System", ' ', LDWS_VERSION);
		// FIXME CUDA and OpenCL should be mutually exclusive switches
		TCLAP::SwitchArg enable_cuda_switch("u","enable-cuda","Enable CUDA support", cmd_line, false);
		TCLAP::SwitchArg enable_opencl_switch("o","enable-opencl","Enable OpenCL support", cmd_line, false);
		TCLAP::SwitchArg disable_display_switch("d","disable-display","Disable video display", cmd_line, false);
		TCLAP::SwitchArg display_intermediate_switch("i","display-intermediate","Display intermediate processing steps", cmd_line, false);
		TCLAP::SwitchArg write_video_switch("w","write-video","Write video to a file", cmd_line, false);
		TCLAP::SwitchArg verbose_switch("v","verbose","Verbose messages", cmd_line, false);
		TCLAP::ValueArg<string> config_file_string("c","config-file","Configuration file name", false, "ldws.conf", "filename");
		cmd_line.add(config_file_string);
		cmd_line.parse(argc, argv);

		cs->intermediate_display = display_intermediate_switch.getValue();
		cs->cuda_enabled = enable_cuda_switch.getValue();
		cs->opencl_enabled = enable_opencl_switch.getValue();
		cs->display_enabled = !disable_display_switch.getValue();
		cs->file_write = write_video_switch.getValue();
		cs->verbose = verbose_switch.getValue();
		cs->config_file = config_file_string.getValue();
	} catch (TCLAP::ArgException &e) {
		std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	}

	// Parse config file
	Config cfg;
	cfg.readFile(cs->config_file.c_str());
	string video_file = cfg.lookup("video_file");
	int rx = cfg.lookup("region_of_interest.x");
	int ry = cfg.lookup("region_of_interest.y");
	int rw = cfg.lookup("region_of_interest.w");
	int rh = cfg.lookup("region_of_interest.h");

	// Open video input file/device
	VideoCapture capture(video_file);
	// If file open fails, try finding a camera indicated by an integer argument
	if (!capture.isOpened())
	{capture.open(atoi(video_file.c_str()));}

	// Toggle OpenCL on/off
	if (!cs->cuda_enabled)
		cv::ocl::setUseOpenCL(cs->opencl_enabled);

	string mode = "CPU";
	if (cs->cuda_enabled)
		mode = "CUDA";
	else if (cs->opencl_enabled)
		mode = "OpenCL";
	cout << "Mode: " << mode << endl;

	// Report video specs
	double width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
	double height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
	int ex = static_cast<int>(capture.get(CV_CAP_PROP_FOURCC));
	char fourcc[] = {(char)(ex & 0XFF),(char)((ex & 0XFF00) >> 8),(char)((ex & 0XFF0000) >> 16),(char)((ex & 0XFF000000) >> 24),0};
	Size frame_size(static_cast<int>(width), static_cast<int>(height));
	cout << "Video: frame size " << width << "x" << height << ", codec " << fourcc << endl;
	// FIXME Error check ROI settings here


	// Create output window
	string window_name = "Full Video";
	if (cs->display_enabled) {
		namedWindow(window_name, CV_WINDOW_KEEPRATIO);
	}

	// FIXME this should be conditional
	VideoWriter output_writer("ldws-full.avi", CV_FOURCC('P','I','M','1'), 30, frame_size, true);

	Mat frame, edge;
	cv::cuda::GpuMat gpu_frame, gpu_gray, gpu_edge, gpu_lines;
	UMat u_frame, u_gray, u_edge;
	cv::Ptr<cv::cuda::Filter> blur = cv::cuda::createGaussianFilter(CV_8UC1, CV_8UC1, Size(5, 5), 1.5);
	cv::Ptr<cv::cuda::CannyEdgeDetector> canny = cv::cuda::createCannyEdgeDetector(1, 100, 3, false);
	double rho = 1;
	double theta = CV_PI/180;
	vector<Vec4i> lines;
	cv::Ptr<cuda::HoughSegmentDetector> hough = cuda::createHoughSegmentDetector(rho, theta, 50, 100);

	frame_avg_init();

	while (true)
	{
		capture >> frame;
		if (frame.empty())
			break;

		// Display original frame
		if (cs->intermediate_display) {
			namedWindow("Original Video", WINDOW_AUTOSIZE);
			imshow("Original Video", frame);
		}

		frame_begin();

		if (cs->cuda_enabled) {
			// CUDA implementation
			gpu_frame.upload(frame);

			// Set ROI to reduce workload
			cv::cuda::GpuMat gpu_roi(gpu_frame, Rect(rx, ry, rw, rh));

			// Convert to grayscale and blur
			cv::cuda::cvtColor(gpu_roi, gpu_gray, CV_BGR2GRAY);
			blur->apply(gpu_gray, gpu_gray);

			// Canny edge detection
			canny->detect(gpu_gray, gpu_edge);

			// Probabilistic Hough line detection
			hough->detect(gpu_edge, gpu_lines);
			lines.resize(gpu_lines.cols);
			Mat temp(1, gpu_lines.cols, CV_32SC4, &lines[0]);
			gpu_lines.download(temp);
		} else {
			// TAPI implementation
			frame.copyTo(u_frame);

			// Set ROI to reduce workload
			UMat u_roi(u_frame, Rect(rx, ry, rw, rh));

			// Convert to grayscale and blur
			cvtColor(u_roi, u_gray, CV_BGR2GRAY);
			GaussianBlur(u_gray, u_gray, Size(5, 5), 1.5);

			// Canny edge detection
			Canny(u_gray, u_edge, 1, 100);

			// Probabilistic Hough line detection
			HoughLinesP(u_edge, lines, rho, theta, 50, 50, 100);
		}

		ProcessLanes(lines, frame, Point(rx, ry), cs);

		frame_end();

		// Display Canny image
		if (cs->intermediate_display) {
			namedWindow("Edges");
			if (cs->cuda_enabled) {
				gpu_edge.download(edge);
				imshow("Edges", edge);
			} else {
				imshow("Edges", u_edge);
			}
		}

		// Display FPS
		putText(frame, "FPS: " + frame_fps_str(), Point(5,25), FONT_HERSHEY_SIMPLEX, 1., Scalar(255, 100, 0), 2);

		// Display full image
		if (cs->display_enabled)
			imshow(window_name, frame);

		// Write frame to output file
		if (cs->file_write)
			output_writer << frame;

		if (waitKey(1) == 27) break;
	}

	cout << "Average FPS: " << frame_fps_avg_str() << endl;
}




