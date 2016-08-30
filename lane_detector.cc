/*
 * The lane detection algorithm is taken from
 * https://code.google.com/archive/p/opencv-lane-vehicle-track/
 * https://github.com/tomazas/opencv-lane-vehicle-track
 * where the Google Code page indicates this is under MIT license
 * despite there being no copyright notice.
 *
 * Additional code to move to OpenCV 3, adapt to the LDWS application
 * framework, and perform lane post processing is:
 *
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
#include <opencv2/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>

#include "config_store.h"
#include "lane_detector.h"
#include "util.h"

using namespace cv;
using namespace std;

CvPoint2D32f sub(CvPoint2D32f b, CvPoint2D32f a) { return cvPoint2D32f(b.x-a.x, b.y-a.y); }
CvPoint2D32f mul(CvPoint2D32f b, CvPoint2D32f a) { return cvPoint2D32f(b.x*a.x, b.y*a.y); }
CvPoint2D32f add(CvPoint2D32f b, CvPoint2D32f a) { return cvPoint2D32f(b.x+a.x, b.y+a.y); }
CvPoint2D32f mul(CvPoint2D32f b, float t) { return cvPoint2D32f(b.x*t, b.y*t); }
float dot(CvPoint2D32f a, CvPoint2D32f b) { return (b.x*a.x + b.y*a.y); }
float dist(CvPoint2D32f v) { return sqrtf(v.x*v.x + v.y*v.y); }

CvPoint2D32f point_on_segment(CvPoint2D32f line0, CvPoint2D32f line1, CvPoint2D32f pt){
	CvPoint2D32f v = sub(pt, line0);
	CvPoint2D32f dir = sub(line1, line0);
	float len = dist(dir);
	float inv = 1.0f/(len+1e-6f);
	dir.x *= inv;
	dir.y *= inv;

	float t = dot(dir, v);
	if(t >= len) return line1;
	else if(t <= 0) return line0;

	return add(line0, mul(dir,t));
}

float dist2line(CvPoint2D32f line0, CvPoint2D32f line1, CvPoint2D32f pt){
	return dist(sub(point_on_segment(line0, line1, pt), pt));
}

void LaneDetector::FindResponses(Mat edge, int startX, int endX, int y, std::vector<int>& list)
{
	// scans for single response: /^\_

	const int row = y * edge.cols * edge.channels();
	unsigned char* ptr = edge.data;

	int step = (endX < startX) ? -1: 1;
	int range = (endX > startX) ? endX-startX+1 : startX-endX+1;

	for(int x = startX; range>0; x += step, range--)
	{
		if(ptr[row + x] <= cs->bw_thresh) continue; // skip black: loop until white pixels show up

		// first response found
		int idx = x + step;

		// skip same response(white) pixels
		while(range > 0 && ptr[row+idx] > cs->bw_thresh){
			idx += step;
			range--;
		}

		// reached black again
		if(ptr[row+idx] <= cs->bw_thresh) {
			list.push_back(x);
		}

		x = idx; // begin from new pos
	}
}

void LaneDetector::ProcessSide(std::vector<Lane> lanes, Mat edge, bool right) {

	Status* side = right ? &laneR : &laneL;

	// response search
	int w = edge.cols;
	int h = edge.rows;
	const int BEGINY = 0;
	const int ENDY = h-1;
	const int ENDX = right ? (w-cs->borderx) : cs->borderx;
	int midx = w/2;
	int midy = h/2;

	// show responses
	int* votes = new int[lanes.size()];
	for(int i=0; i<lanes.size(); i++) votes[i++] = 0;

	for(int y=ENDY; y>=BEGINY; y-=cs->scan_step) {
		std::vector<int> rsp;
		FindResponses(edge, midx, ENDX, y, rsp);

		if (rsp.size() > 0) {
			int response_x = rsp[0]; // use first reponse (closest to screen center)

			float dmin = 9999999;
			float xmin = 9999999;
			int match = -1;
			for (int j=0; j<lanes.size(); j++) {
				// compute response point distance to current line
				float d = dist2line(
						cvPoint2D32f(lanes[j].p0.x, lanes[j].p0.y), 
						cvPoint2D32f(lanes[j].p1.x, lanes[j].p1.y), 
						cvPoint2D32f(response_x, y));

				// point on line at current y line
				int xline = (y - lanes[j].b) / lanes[j].k;
				int dist_mid = abs(midx - xline); // distance to midpoint

				// pick the best closest match to line & to screen center
				if (match == -1 || (d <= dmin && dist_mid < xmin)) {
					dmin = d;
					match = j;
					xmin = dist_mid;
					break;
				}
			}

			// vote for each line
			if (match != -1) {
				votes[match] += 1;
			}
		}
	}

	int bestMatch = -1;
	int mini = 9999999;
	for (int i=0; i<lanes.size(); i++) {
		int xline = (midy - lanes[i].b) / lanes[i].k;
		int dist = abs(midx - xline); // distance to midpoint

		if (bestMatch == -1 || (votes[i] > votes[bestMatch] && dist < mini)) {
			bestMatch = i;
			mini = dist;
		}
	}

	if (bestMatch != -1) {
		Lane* best = &lanes[bestMatch];
		float k_diff = fabs(best->k - side->k.get());
		float b_diff = fabs(best->b - side->b.get());

		bool update_ok = (k_diff <= cs->k_vary_factor && b_diff <= cs->b_vary_factor) || side->reset;

		if (cs->verbose)
			printf("side: %s, k vary: %.4f, b vary: %.4f, lost: %s\n", 
					(right?"RIGHT":"LEFT"), k_diff, b_diff, (update_ok?"no":"yes"));

		if (update_ok) {
			// update is in valid bounds
			side->k.add(best->k);
			side->b.add(best->b);
			side->reset = false;
			side->lost = 0;
		} else {
			// can't update, lanes flicker periodically, start counter for partial reset!
			side->lost++;
			if (side->lost >= cs->max_lost_frames && !side->reset) {
				side->reset = true;
			}
		}

	} else {
		if (cs->verbose)
			printf("no lanes detected - lane tracking lost! counter increased\n");
		side->lost++;
		if (side->lost >= cs->max_lost_frames && !side->reset) {
			// do full reset when lost for more than N frames
			side->reset = true;
			side->k.clear();
			side->b.clear();
		}
	}

	delete[] votes;
}

void LaneDetector::ProcessLanes(vector<Vec4i> lines, Mat frame, Mat edge, Mat temp)
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

	// Process left and right sides
	ProcessSide(left, edge, false);
	ProcessSide(right, edge, true);

	// Draw lane guides
	temp.setTo(0);
	Point lane_pts[4];

	int x = frame.cols * 0.55f;
	int x2 = frame.cols;
	lane_pts[0] = Point(x, laneR.k.get()*x + laneR.b.get()) + roi;
	lane_pts[1] = Point(x2, laneR.k.get() * x2 + laneR.b.get()) + roi;

	x = frame.cols * 0;
	x2 = frame.cols * 0.45f;
	lane_pts[2] = Point(x, laneL.k.get()*x + laneL.b.get()) + roi;
	lane_pts[3] = Point(x2, laneL.k.get() * x2 + laneL.b.get()) + roi;

	fillConvexPoly(temp, lane_pts, 4, CV_RGB(0, 0, 255));
	addWeighted(temp, 0.5, frame, 0.9, 0, frame);
}

LaneDetector::LaneDetector()
{
	cs = ConfigStore::GetInstance();
	roi = Point(cs->roi.x, cs->roi.y);
}

