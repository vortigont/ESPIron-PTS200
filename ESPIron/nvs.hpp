/*
    This file is a part of ESPIron-PTS200 project
    https://github.com/vortigont/ESPIron-PTS200

    Copyright © 2024 Emil Muratov (vortigont)

    ESPIron-PTS200 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/
#pragma once
#include "nvs_handle.hpp"
#include "common.hpp"

// little helpers for reading/writing to NVS

esp_err_t nvs_blob_read(const char* nvsspace, const char* key, void* blob, size_t len);

esp_err_t nvs_blob_write(const char* nvsspace, const char* key, void* blob, size_t len);

