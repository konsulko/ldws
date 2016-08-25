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
#include <stddef.h>
#include <string>

#include "config_store.h"

ConfigStore::ConfigStore()
{
	intermediate_display = false;
	cuda_enabled = false;
	opencl_enabled = false;
	display_enabled = true;
	file_write = false;
	verbose = false;

	config_file = "ldws.conf";
	std::cout << "construct CS" << std::endl;
}

ConfigStore *ConfigStore::instance = NULL;

ConfigStore *ConfigStore::GetInstance()
{
	if (!instance)
		instance = new ConfigStore;

	return instance;
}


