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

#ifndef LANE_DETECTOR_H
#define LANE_DETECTOR_H

#include <opencv2/core.hpp>
#include <vector>

#include "config_store.h"
#include "util.h"

using namespace cv;
using namespace std;

class LaneDetector
{
	public:
		LaneDetector();
		void ProcessLanes(vector<Vec4i> lines, Mat frame, Mat edge, Mat temp);

	private:
		ConfigStore *cs;
		Point roi;
		struct Lane {
			Lane(){}
			Lane(Point a, Point b, float angle, float kl, float bl): p0(a),p1(b),angle(angle),
			votes(0),visited(false),found(false),k(kl),b(bl) { }

			Point p0, p1;
			int votes;
			bool visited, found;
			float angle, k, b;
		};
		struct Status {
			Status():reset(true),lost(0){}
			ExpMovingAverage k, b;
			bool reset;
			int lost;
		};
		Status laneR, laneL;
		void FindResponses(Mat edge, int startX, int endX, int y, vector<int>& list);
		void ProcessSide(vector<Lane> lanes, Mat edge, bool right);
};

#endif // LANE_DETECTOR_H
