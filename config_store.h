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

#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <string>

class ConfigStore
{
	public:
		static ConfigStore* GetInstance();
		void ParseConfig(int argc, char *argv[]);

		// Command line settings
		bool intermediate_display;
		bool cuda_enabled;
		bool opencl_enabled;
		bool display_enabled;
		bool file_write;
		bool verbose;
		std::string config_file;

		// Config file settings
		std::string video_in;
		std::string video_out;
		struct roi_struct {
			int x;
			int y;
			int w;
			int h;
		} roi;
		int line_reject_degrees;
		int canny_min_thresh;
		int canny_max_thresh;
		int hough_thresh;
		int hough_min_length;
		int hough_max_gap;
		int scan_step;
		int bw_thresh;
		int borderx;
		int max_response_dist;
		float k_vary_factor;
		int b_vary_factor;
		int max_lost_frames;

	private:
		static ConfigStore* instance;
		ConfigStore();
		void ParseCmdLine(int argc, char *argv[]);
		void ParseCfgFile();

};

#endif // CONFIG_STORE_H
