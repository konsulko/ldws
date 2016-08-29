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

#include <iostream>
#include <libconfig.h++>
#include <stddef.h>
#include <string>
#include <tclap/CmdLine.h>

#include "config.h"
#include "config_store.h"

using namespace std;
using namespace libconfig;

void ConfigStore::ParseCmdLine(int argc, char* argv[]) {
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

		intermediate_display = display_intermediate_switch.getValue();
		cuda_enabled = enable_cuda_switch.getValue();
		opencl_enabled = enable_opencl_switch.getValue();
		display_enabled = !disable_display_switch.getValue();
		file_write = write_video_switch.getValue();
		verbose = verbose_switch.getValue();
		config_file = config_file_string.getValue();
	} catch (TCLAP::ArgException &e) {
		std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	}
}

void ConfigStore::ParseCfgFile() {
	Config cfg;
	cfg.readFile(config_file.c_str());
	cfg.lookupValue("video_input_file", video_in);
	cfg.lookupValue("video_output_file", video_out);
	cfg.lookupValue("region_of_interest.x", roi.x);
	cfg.lookupValue("region_of_interest.y", roi.y);
	cfg.lookupValue("region_of_interest.w", roi.w);
	cfg.lookupValue("region_of_interest.h", roi.h);
	cfg.lookupValue("line_reject_degrees", line_reject_degrees);
	cfg.lookupValue("canny_min_thresh", canny_min_thresh);
	cfg.lookupValue("canny_max_thresh", canny_max_thresh);
	cfg.lookupValue("hough_thresh", hough_thresh);
	cfg.lookupValue("hough_min_length", hough_min_length);
	cfg.lookupValue("hough_max_gap", hough_max_gap);
}

void ConfigStore::ParseConfig(int argc, char* argv[])
{
	ParseCmdLine(argc, argv);
	ParseCfgFile();
}

ConfigStore::ConfigStore()
{
	// Command line settings
	intermediate_display = false;
	cuda_enabled = false;
	opencl_enabled = false;
	display_enabled = true;
	file_write = false;
	verbose = false;
	config_file = "ldws.conf";

	// Config file settings
	video_in = "ldws-in.avi";
	video_out = "ldws-out.avi";
	roi.x = 0; roi.y=0; roi.w=0; roi.h=0;
	line_reject_degrees = 30;
	canny_min_thresh = 70;
	canny_max_thresh = 140;
	hough_thresh = 50;
	hough_min_length = 50;
	hough_max_gap = 100;
	scan_step = 5;
	bw_thresh = 250;
	borderx = 10;
	max_response_dist = 5;
	k_vary_factor = 0.2f;
	b_vary_factor = 20;
	max_lost_frames = 30;
}

ConfigStore *ConfigStore::instance = NULL;

ConfigStore *ConfigStore::GetInstance()
{
	if (!instance)
		instance = new ConfigStore;

	return instance;
}


