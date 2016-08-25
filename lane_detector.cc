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

#include <opencv2/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>

#include "config_store.h"
#include "lane_detector.h"

using namespace cv;

void LaneDetector::ProcessLanes(vector<Vec4i> lines, Mat frame)
{
	vector<Lane> left, right;

	for(int i = 0; i < lines.size(); i++ )
	{
		Point pt1 = Point(lines[i][0], lines[i][1]), pt2 = Point(lines[i][2], lines[i][3]);
		int dx = pt2.x - pt1.x;
		int dy = pt2.y - pt1.y;
		float angle = atan2f(dy, dx) * 180/CV_PI;
		if (fabs(angle) < cs->line_reject_degrees) {
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

LaneDetector::LaneDetector()
{
	cs = ConfigStore::GetInstance();
	roi = Point(cs->roi.x, cs->roi.y);
}

