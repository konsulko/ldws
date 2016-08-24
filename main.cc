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
#include "fps.h"

using namespace std;
using namespace cv;
using namespace libconfig;

int main(int argc, char* argv[])
{
	string config_file;
	bool display_intermediate;
	bool enable_cuda;
	bool enable_opencl;
	bool enable_display;
	bool write_output;
	bool verbose;

	// Parse command line options
	try {
		TCLAP::CmdLine cmd_line("Lane Departure Warning System", ' ', LDWS_VERSION);
		// FIXME CUDA and OpenCL should be mutually exclusive switches
		TCLAP::SwitchArg enable_cuda_switch("u","enable-cuda","Enable CUDA support", cmd_line, false);
		TCLAP::SwitchArg enable_opencl_switch("o","enable-opencl","Enable OpenCL support", cmd_line, false);
		TCLAP::SwitchArg disable_display_switch("d","disable-display","Disable video display", cmd_line, false);
		TCLAP::SwitchArg display_intermediate_switch("i","display-intermediate","Display intermediate processing steps", cmd_line, false);
		TCLAP::SwitchArg write_output_switch("w","write-video","Write video to a file", cmd_line, false);
		TCLAP::SwitchArg verbose_switch("v","verbose","Verbose messages", cmd_line, false);
		TCLAP::ValueArg<string> config_file_string("c","config-file","Configuration file name", false, "ldws.conf", "filename");
		cmd_line.add(config_file_string);
		cmd_line.parse(argc, argv);

		display_intermediate = display_intermediate_switch.getValue();
		enable_cuda = enable_cuda_switch.getValue();
		enable_opencl = enable_opencl_switch.getValue();
		enable_display = !disable_display_switch.getValue();
		write_output = write_output_switch.getValue();
		verbose = verbose_switch.getValue();
		config_file = config_file_string.getValue();
	} catch (TCLAP::ArgException &e) {
		std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	}

	// Parse config file
	Config cfg;
	cfg.readFile(config_file.c_str());
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
	if (!enable_cuda)
		cv::ocl::setUseOpenCL(enable_opencl);

	string mode = "CPU";
	if (enable_cuda)
		mode = "CUDA";
	else if (enable_opencl)
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
	if (enable_display) {
		namedWindow(window_name, CV_WINDOW_KEEPRATIO);
	}

	// FIXME this should be conditional
	VideoWriter output_writer("ldws-full.avi", CV_FOURCC('P','I','M','1'), 30, frame_size, true);

	Mat frame, edge;
	cv::cuda::GpuMat gpu_frame, gpu_gray, gpu_edge;
	UMat u_frame, u_gray, u_edge;
	cv::Ptr<cv::cuda::Filter> blur = cv::cuda::createGaussianFilter(CV_8UC1, CV_8UC1, Size(5, 5), 1.5);
	cv::Ptr<cv::cuda::CannyEdgeDetector> canny = cv::cuda::createCannyEdgeDetector(1, 100, 3, false);

	frame_avg_init();

	while (true)
	{
		capture >> frame;
		if (frame.empty())
			break;

		// Display original frame
		if (display_intermediate) {
			namedWindow("Original Video", WINDOW_AUTOSIZE);
			imshow("Original Video", frame);
		}

		frame_begin();

		if (enable_cuda) {
			// CUDA
			gpu_frame.upload(frame);
			cv::cuda::GpuMat gpu_roi(gpu_frame, Rect(rx, ry, rw, rh));
			cv::cuda::cvtColor(gpu_roi, gpu_gray, CV_BGR2GRAY);
			blur->apply(gpu_gray, gpu_gray);
			canny->detect(gpu_gray, gpu_edge);
		} else {
			// TAPI
			frame.copyTo(u_frame);
			UMat u_roi(u_frame, Rect(rx, ry, rw, rh));
			cvtColor(u_roi, u_gray, CV_BGR2GRAY);
			GaussianBlur(u_gray, u_gray, Size(5, 5), 1.5);
			Canny(u_gray, u_edge, 1, 100);
		}

		// TODO Add line detection

		frame_end();

		// Display Canny image
		if (display_intermediate) {
			namedWindow("Edges");
			if (enable_cuda) {
				gpu_edge.download(edge);
				imshow("Edges", edge);
			} else {
				imshow("Edges", u_edge);
			}
		}

		// Display FPS
		putText(frame, "FPS: " + frame_fps_str(), Point(5,25), FONT_HERSHEY_SIMPLEX, 1., Scalar(255, 100, 0), 2);

		// Display full image
		if (enable_display)
			imshow(window_name, frame);

		// Write frame to output file
		if (write_output)
			output_writer << frame;

		if (waitKey(1) == 27) break;
	}

	cout << "Average FPS: " << frame_fps_avg_str() << endl;
}




