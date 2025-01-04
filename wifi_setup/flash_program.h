/**
 * This file is part of "Wi-Fi Configure.
 *
 * This software eliminates the need to know the network name, password and,
 * if required, IP address, network mask and default gateway at compile time.
 * These can be set directly on the Pico-W and also changed afterwards.
 *
 * Copyright (c) 2024 Gerhard Schiller gerhard.schiller@pm.me
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FLASH_PROGRAMM_H
#define FLASH_PROGRAMM_H

// The wifi-configuration data goes into page 0
// (Pages go from 0 to FLASH_PAGES_PER_SECTOR)
#define WIFI_CONFIG_PAGE 0

void show_stats();
void flash_erase_page(size_t pageStart, size_t numPages);
void flash_write_page(uint8_t *data, uint16_t buf_len, size_t pageStart);
void flash_read(uint8_t *data,uint16_t len, size_t pageStart);

#endif // FLASH_PROGRAMM_H
