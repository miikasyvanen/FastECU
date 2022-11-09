#ifndef KERNELMEMORYMODELS_H
#define KERNELMEMORYMODELS_H

#include <stdint.h>

enum mcu_type {
    MC68HC16Y5,
    SH7051,
    SH7055,
    SH7058,
    SH_INVALID
};

struct flashblock {
    uint32_t start;
    uint32_t len;
};

struct ramblock {
    uint32_t start;
    uint32_t len;
};

struct flashdev_t {
    const char *name;		// like "7058", for UI convenience only
    enum mcu_type mcutype;

    const uint32_t romsize;		//in bytes
    const unsigned numblocks;
    const struct flashblock *fblocks;
    const struct ramblock *rblocks;
};

/* list of all defined flash devices */
//extern const struct flashdev_t flashdevices[];


/* flash block definitions */
const struct flashblock fblocks_SH7058[] = {
    {0x00000000,    0x00001000},
    {0x00001000,    0x00001000},
    {0x00002000,    0x00001000},
    {0x00003000,    0x00001000},
    {0x00004000,    0x00001000},
    {0x00005000,    0x00001000},
    {0x00006000,    0x00001000},
    {0x00007000,    0x00001000},
    {0x00008000,    0x00018000},
    {0x00020000,    0x00020000},
    {0x00040000,    0x00020000},
    {0x00060000,    0x00020000},
    {0x00080000,    0x00020000},
    {0x000A0000,    0x00020000},
    {0x000C0000,    0x00020000},
    {0x000E0000,    0x00020000},
};

const struct ramblock rblocks_SH7058[] = {
    {0xFFFF3000,    0x00009000},//0xFFFFBFFF // 36k
};

const struct flashblock fblocks_SH7055[] = {
    {0x00000000,    0x00001000},
    {0x00001000,    0x00001000},
    {0x00002000,    0x00001000},
    {0x00003000,    0x00001000},
    {0x00004000,    0x00001000},
    {0x00005000,    0x00001000},
    {0x00006000,    0x00001000},
    {0x00007000,    0x00001000},
    {0x00008000,    0x00008000},
    {0x00010000,    0x00010000},
    {0x00020000,    0x00010000},
    {0x00030000,    0x00010000},
    {0x00040000,    0x00010000},
    {0x00050000,    0x00010000},
    {0x00060000,    0x00010000},
    {0x00070000,    0x00010000},
};

const struct ramblock rblocks_SH7055[] = {
    {0xFFFF6000,    0x00006000},//0xFFFFBFFF // 24k
};

const struct flashblock fblocks_SH7051[] = {
    {0x00000000,    0x00008000},
    {0x00008000,    0x00008000},
    {0x00010000,    0x00008000},
    {0x00018000,    0x00008000},
    {0x00020000,    0x00008000},
    {0x00028000,    0x00008000},
    {0x00030000,    0x00008000},
    {0x00038000,    0x00007000},
    {0x0003F000,    0x00000400},
    {0x0003F400,    0x00000400},
    {0x0003F800,    0x00000400},
    {0x0003FC00,    0x00000400},
};

const struct ramblock rblocks_SH7051[] = {
    {0xFFFFD800,    0x00002800},//0xFFFFFFFF  // 10k
};

const struct flashblock fblocks_MC68HC16Y5[] = {
    {0x00000000,    0x0004000},
    {0x00004000,    0x0004000},
    {0x00008000,    0x0004000},
    {0x0000C000,    0x0004000},
    {0x00010000,    0x0004000},
    {0x00014000,    0x0004000},
    {0x00018000,    0x0004000},
    {0x0001C000,    0x0004000},
    {0x00028000,    0x0004000},
    {0x0002C000,    0x0004000},
};

const struct ramblock rblocks_MC68HC16Y5[] = {
    {0x00020000,    0x0008000},
};

const struct flashdev_t flashdevices[] = {
    { "MC68HC16Y5", MC68HC16Y5, 160 * 1024, 10, fblocks_MC68HC16Y5, rblocks_MC68HC16Y5 },
    { "SH7051", SH7051, 256 * 1024, 12, fblocks_SH7051, rblocks_SH7051 },
    { "SH7055", SH7055, 512 * 1024, 16, fblocks_SH7055, rblocks_SH7055 },
    { "SH7058", SH7058, 1024 * 1024, 16, fblocks_SH7058, rblocks_SH7058 },
    { 0, SH_INVALID, 0, 0, 0 },
};

#endif // KERNELMEMORYMODELS_H
