/*
    This file is a part of ESPIron-PTS200 project
    https://github.com/vortigont/ESPIron-PTS200

    Copyright © 2024 Emil Muratov (vortigont)

    ESPIron-PTS200 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "Arduino.h"
#include "nvs.hpp"
#include "log.h"

static constexpr const char* T_NVS = "NVS";

esp_err_t nvs_blob_read(const char* nvsspace, const char* key, void* blob, size_t len){
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> nvs = nvs::open_nvs_handle(nvsspace, NVS_READONLY, &err);

  if (err != ESP_OK) {
    LOGD(T_NVS, printf, "Err opening NVS ns:%s RO, err:%s\n", nvsspace, esp_err_to_name(err));
    return err;
  }

  err = nvs->get_blob(key, blob, len);
  if (err != ESP_OK) {
    LOGD(T_NVS, printf, "Err reading NVS blob ns:%s, key:%s, err:%s\n", nvsspace, key, esp_err_to_name(err));
  }
  return err;
}

esp_err_t nvs_blob_write(const char* nvsspace, const char* key, void* blob, size_t len){
  esp_err_t err;
  std::unique_ptr<nvs::NVSHandle> nvs = nvs::open_nvs_handle(nvsspace, NVS_READWRITE, &err);

  if (err != ESP_OK) {
    LOGD(T_NVS, printf, "Err opening NVS ns:%s RW, err:%s\n", nvsspace, esp_err_to_name(err));
    return err;
  }

  err = nvs->set_blob(key, blob, len);
  if (err != ESP_OK) {
    LOGD(T_NVS, printf, "Err writing NVS ns:%s, key:%s, err:%s\n", nvsspace, key, esp_err_to_name(err));
  }
  return err;
}
