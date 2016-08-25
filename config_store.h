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

#include <string>

class ConfigStore
{
	ConfigStore();

	public:
		static ConfigStore* GetInstance( );

		bool intermediate_display;
		bool cuda_enabled;
		bool opencl_enabled;
		bool display_enabled;
		bool file_write;
		bool verbose;

		std::string config_file;

	private:
		static ConfigStore* instance;
};
