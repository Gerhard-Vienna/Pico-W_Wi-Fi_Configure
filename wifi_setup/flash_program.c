/**
 * This file is part of "Wi-Fi Configure.
 *
 * This software makes it unnecessary to know the network name, password
 * and - if required - IP address, network mask and default gateway when
 * compiling. These can be set directly on the Pico-W and also modified
 * afterwards.
 *
 * Copyright (c) 2024 Gerhard Schiller gerhard.schiller@pm.me
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * It is based on code from the "pico-sdk".
 * See the copyright notice below.
 */

/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

#include "access_point.h"
#include "flash_program.h"

// The next 2 lines are from the header files in pico-sdk
// #define FLASH_SECTOR_SIZE (1u << 12) -> 4096 = 16 x 256
// #define FLASH_PAGE_SIZE (1u << 8) -> 256

// We use the last sector of flash to store the configuration.
// Once done, we can access this at XIP_BASE + FLASH_TARGET_OFFSET.
#define FLASH_TARGET_OFFSET ((PICO_FLASH_SIZE_BYTES) - FLASH_SECTOR_SIZE)
#define FLASH_PAGES_PER_SECTOR (FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE)
const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

void show_stats()
{
    printf("Statistics:\n");
    printf("\tTotal: Flash %d kB, %d Sectors (@ %d kB)\n",
           PICO_FLASH_SIZE_BYTES / 1024,
           PICO_FLASH_SIZE_BYTES / FLASH_SECTOR_SIZE,
           FLASH_SECTOR_SIZE / 1024);

    extern char __flash_binary_end;
    uintptr_t program_end = ((uintptr_t)&__flash_binary_end - XIP_BASE);
    uint16_t progsizekB = (program_end / 1024) +
                (program_end % 1024 ? 1 : 0);
    uint16_t progsizeSec = (program_end / FLASH_SECTOR_SIZE) +
                (program_end % FLASH_SECTOR_SIZE ? 1 : 0);

                printf("\tUsed by this programm: %d kB, %d Sectors (@ %d kB)\n\n",
                       progsizekB, progsizeSec, FLASH_SECTOR_SIZE / 1024);
}

void flash_erase_page(size_t pageStart, size_t numPages)
{
    uint8_t *buf;

    // We can only erase a whole sector, which has 16 pages
    // So we read the sector (all 16 pages) first and save it
    buf = (uint8_t *)malloc(FLASH_SECTOR_SIZE);
    if(!buf)
        DEBUG_printf("malloc failed\n");
    memcpy(buf, flash_target_contents, FLASH_SECTOR_SIZE);

    uint32_t interrupts = save_and_disable_interrupts();        // now we erase the sector
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);

    //next set numPages, starting at pageStart to 0xFF
    for(int i = 0; i < (numPages * FLASH_PAGE_SIZE); i++){
        buf[(pageStart * FLASH_PAGE_SIZE) + i] = 0xFF;
    }

    //and write it back to the flash
    flash_range_program(FLASH_TARGET_OFFSET, buf, FLASH_SECTOR_SIZE);
    restore_interrupts(interrupts);

    free(buf);
}

// Writes "data" to flash, starting at "pageStart"
// Flash after the end of "data" up to the end of the page is set to 0XFF
void flash_write_page(uint8_t *data, uint16_t buf_len, size_t pageStart)
{
    uint8_t *buf;
    uint8_t numPages;

    numPages = (buf_len / FLASH_PAGE_SIZE) + (buf_len % FLASH_PAGE_SIZE ? 1 : 0);


    buf = (uint8_t *)malloc(FLASH_PAGE_SIZE * numPages);
    memset(buf, 0XFF, (FLASH_PAGE_SIZE * numPages));
    memcpy((void *)buf, data, buf_len);

    flash_erase_page(pageStart, numPages);

    uint32_t interrupts = save_and_disable_interrupts();        flash_range_program(FLASH_TARGET_OFFSET + (FLASH_PAGE_SIZE * pageStart),
                        buf, FLASH_PAGE_SIZE * numPages);
    restore_interrupts(interrupts);
    free(buf);
}

void flash_read(uint8_t *data,uint16_t len, size_t pageStart)
{
    memcpy((void *)data,
           flash_target_contents + (FLASH_PAGE_SIZE * pageStart),
           len);
}

