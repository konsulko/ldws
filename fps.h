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

#include <opencv2/core/utility.hpp>
#include <string>

using namespace std;
using namespace cv;

static double time_begin;
static double time_elapsed;
static double frame_fps;
static double total_time;
static int frame_cnt;

static inline void frame_avg_init() {
	total_time = 0.0;
	frame_cnt = 0;
}

static inline void frame_begin() { time_begin = getTickCount(); }

static inline void frame_end()
{
	time_elapsed = ((double)getTickCount() - time_begin) / getTickFrequency();
	total_time += time_elapsed;
	frame_cnt++;
	frame_fps = 1/time_elapsed;
}

static inline string frame_fps_str()
{
	stringstream ss;
	ss << frame_fps;
	return ss.str();
}

static inline string frame_fps_avg_str()
{
	stringstream ss;
	ss << (frame_cnt / total_time);
 	return ss.str();
}


