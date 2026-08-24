#pragma once

/*
 * Storage Service — NVS weather cache and SD card history logging.
 */

namespace svc {

void storageInit();
void storageService();

void storageLoadCache();
void storageSaveCache();

void storageLoadHistory();
void storageLogHistory();

}  // namespace svc
