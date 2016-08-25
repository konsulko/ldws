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

#ifndef UTIL_H
#define UTIL_H

#include <list>

class ExpMovingAverage {
	private:
		double alpha; // [0;1] less = more stable, more = less stable
		double oldValue;
		bool unset;
	public:
		ExpMovingAverage() {
			this->alpha = 0.2;
			unset = true;
		}

		void clear() {
			unset = true;
		}

		void add(double value) {
			if (unset) {
				oldValue = value;
				unset = false;
			}
			double newValue = oldValue + alpha * (value - oldValue);
			oldValue = newValue;
		}

		double get() {
			return oldValue;
		}
};

#endif // UTIL_H
