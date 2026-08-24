#pragma once

/*
 * Storage Service — NVS weather cache and SD card history logging.
 */

#include "src/core/Types.h"

namespace svc {

void storageInit();
void storageService();

void storageLoadConfig();
void storageSaveConfig(const RuntimeConfig& cfg);

void storageLoadCache();
void storageSaveCache();

void storageLoadHistory();
void storageLogHistory();

}  // namespace svc

