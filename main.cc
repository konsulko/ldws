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

#include "opencv2/highgui/highgui.hpp"
//#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <tclap/CmdLine.h>
#include <libconfig.h++>
#include <iostream>
#include <string>

#include "config.h"

using namespace std;
using namespace cv;
using namespace libconfig;

int main(int argc, char* argv[])
{
	string config_file;
	bool display_intermediate;
	bool write_output;
	bool verbose;

	// Parse command line options
	try {
		TCLAP::CmdLine cmd_line("Lane Departure Warning System", ' ', LDWS_VERSION);
		TCLAP::SwitchArg display_intermediate_switch("i","display-intermediate","Display intermediate processing steps", cmd_line, false);
		TCLAP::SwitchArg write_output_switch("w","write-output","Write output to a file", cmd_line, false);
		TCLAP::SwitchArg verbose_switch("v","verbose","Verbose messages", cmd_line, false);
		TCLAP::ValueArg<string> config_file_string("c","config-file","Configuration file name", false, "ldws.conf", "filename");
		cmd_line.add(config_file_string);

		cmd_line.parse(argc, argv);

		display_intermediate = display_intermediate_switch.getValue();
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

	// Open video input file/device
	VideoCapture capture(video_file);
	// If file open fails, try finding a camera indicated by an integer argument
	if (!capture.isOpened())
	{capture.open(atoi(video_file.c_str()));}

	// Create output window
	string window_name = "Full Video";
	namedWindow(window_name, CV_WINDOW_KEEPRATIO);

	double width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
	double height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
	int ex = static_cast<int>(capture.get(CV_CAP_PROP_FOURCC));
	char fourcc[] = {(char)(ex & 0XFF),(char)((ex & 0XFF00) >> 8),(char)((ex & 0XFF0000) >> 16),(char)((ex & 0XFF000000) >> 24),0};

	Size frameSize(static_cast<int>(width), static_cast<int>(height));

	std::cout << "Frame Size = " << width << "x" << height << std::endl;
	std::cout << "FOURCC = " << fourcc << std::endl;

	// FIXME this should be conditional
	VideoWriter output_writer("ldws-full.avi", CV_FOURCC('P','I','M','1'), 30, frameSize, true);

	UMat frame;

	while (true)
	{
		capture >> frame;
		if (frame.empty())
			break;
		UMat gray;
		cvtColor(frame,gray,CV_RGB2GRAY);
		//		Rect roi(0,180,640,180);// set the ROI for the frame
		//		Mat imgROI = frame(roi);

		// Display original frame
		if (display_intermediate) {
			//namedWindow("Original Video", WINDOW_AUTOSIZE);
			imshow("Original Video", frame);
		}

		UMat contours;
		Canny(frame, contours, 80, 250);
		UMat contoursInv;
		threshold(contours, contoursInv, 128, 255, THRESH_BINARY_INV);

		// Display Canny image
		if (display_intermediate) {
			//namedWindow("Contours");
			imshow("Contours",contoursInv);
		}

		// Display full image
		// FIXME just edges for now
		imshow(window_name, contoursInv);

		// Write frame to output file
		if (write_output)
			output_writer << frame.getMat(ACCESS_READ);

		char key = (char) waitKey(10);
	}


}




