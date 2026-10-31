/**
 * Bengal Tiger OS - Full FAT12/FAT16 Filesystem Implementation
 *
 * @file fat.c
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#include "fat.h"
#include "disk.h"
#include "common.h"
#include "heap.h"
#include "shell.h"
#include "klog.h"
#include "version.h"

typedef struct {
    uint8_t  jump[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    uint8_t  drive_num;
    uint8_t  reserved1;
    uint8_t  boot_sig;
    uint32_t vol_id;
    char     vol_label[11];
    char     fs_type[8];
} __attribute__((packed)) fat_boot_t;

typedef struct {
    char     name[8];
    char     ext[3];
    uint8_t  attr;
    uint8_t  reserved;
    uint8_t  ctime_ms;
    uint16_t ctime;
    uint16_t cdate;
    uint16_t adate;
    uint16_t cluster_high;
    uint16_t mtime;
    uint16_t mdate;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((packed)) fat_dirent_t;

typedef struct {
    uint8_t  status;
    uint8_t  chs_first[3];
    uint8_t  part_type;
    uint8_t  chs_last[3];
    uint32_t lba_start;
    uint32_t sector_count;
} __attribute__((packed)) mbr_part_t;

static fat_boot_t boot_sector;
static uint32_t vol_lba_offset   = 0;
static uint32_t fat_start_sector = 0;
static uint32_t root_start_sector = 0;
static uint32_t root_sectors     = 0;
static uint32_t data_start_sector = 0;
static uint32_t total_clusters   = 0;
static int      fat_type         = 0;
static int      fs_mounted       = 0;

static inline uint16_t rd_le16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static inline void wr_le16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}

static uint32_t eoc_threshold(void) {
    return (fat_type == 12) ? 0x0FF8U : 0xFFF8U;
}

static uint32_t eoc_marker(void) {
    return (fat_type == 12) ? 0x0FFFU : 0xFFFFU;
}

static int is_valid_bpb(const uint8_t *sec) {
    if (sec[510] != 0x55 || sec[511] != 0xAA) return 0;

    const fat_boot_t *b = (const fat_boot_t*)sec;
    if (b->bytes_per_sector != 512) return 0;
    if (b->sectors_per_cluster == 0 || !IS_POWER_OF_2(b->sectors_per_cluster)) return 0;
    if (b->reserved_sectors == 0) return 0;
    if (b->num_fats < 1 || b->num_fats > 4) return 0;
    if (b->root_entries == 0 || b->fat_size_16 == 0) return 0;

    uint32_t tot = b->total_sectors_16 ? b->total_sectors_16 : b->total_sectors_32;
    uint32_t r_secs = ((uint32_t)b->root_entries * 32 + 511) / 512;
    uint32_t meta = b->reserved_sectors + ((uint32_t)b->num_fats * b->fat_size_16) + r_secs;
    if (tot <= meta) return 0;

    return 1;
}

static int mount_at_lba(uint32_t base_lba) {
    uint8_t buf[512];
    if (ata_read_sector(base_lba, buf) != 0) return -1;
    if (!is_valid_bpb(buf)) return -1;

    memcpy(&boot_sector, buf, sizeof(fat_boot_t));
    vol_lba_offset    = base_lba;
    fat_start_sector  = base_lba + boot_sector.reserved_sectors;
    root_start_sector = fat_start_sector + ((uint32_t)boot_sector.num_fats * boot_sector.fat_size_16);
    root_sectors      = ((uint32_t)boot_sector.root_entries * 32 + 511) / 512;
    data_start_sector = root_start_sector + root_sectors;

    uint32_t tot = boot_sector.total_sectors_16 ? boot_sector.total_sectors_16 : boot_sector.total_sectors_32;
    uint32_t data_secs = tot - (data_start_sector - base_lba);
    total_clusters = data_secs / boot_sector.sectors_per_cluster;

    if (total_clusters == 0 || total_clusters >= 65525) {
        return -1; /* Empty or FAT32 not handled by 16-bit table */
    }

    fat_type   = (total_clusters < 4085) ? 12 : 16;
    fs_mounted = 1;
    return 0;
}

static int try_mount_disk(void) {
    /* 1. Try Superfloppy BPB directly at LBA 0 */
    if (mount_at_lba(0) == 0) {
        return 0;
    }

    /* 2. Check MBR Partition Table at LBA 0 (offsets 0x1BE..0x1FD) */
    uint8_t mbr[512];
    if (ata_read_sector(0, mbr) != 0) return -1;
    if (mbr[510] != 0x55 || mbr[511] != 0xAA) return -1;

    const mbr_part_t *parts = (const mbr_part_t*)(mbr + 0x1BE);
    for (int i = 0; i < 4; i++) {
        uint8_t pt = parts[i].part_type;
        if (parts[i].lba_start > 0 && parts[i].sector_count > 0 &&
            (pt == 0x01 || pt == 0x04 || pt == 0x06 || pt == 0x0E)) {
            if (mount_at_lba(parts[i].lba_start) == 0) {
                return 0;
            }
        }
    }
    return -1;
}

static void seed_default_files(void) {
    const char *hello =
        "Welcome to Bengal Tiger OS v" OS_VERSION " (" OS_CODENAME ")!\n"
        "A real-world 32-bit x86 operating system built in C & Assembly.\n";

    const char *readme =
        "=== Bengal Tiger OS v" OS_VERSION " Quick Start ===\n"
        "  help              - Show all available shell commands\n"
        "  edit notes.txt    - Open full-screen text editor (^S Save, ^Q Quit)\n"
        "  sh demo.sh        - Execute a shell script from the filesystem\n"
        "  df / disk / mkfs  - View storage or format an ATA hard disk\n"
        "  ifconfig / dhcp   - Configure Intel E1000 / RTL8139 network\n"
        "  ping 10.0.2.2     - Send real ARP + ICMP Echo Requests\n"
        "  spawn / ps / top  - Test preemptive multitasking & task monitor\n"
        "  gui               - Launch graphical VBE desktop workspace\n";

    const char *demo_sh =
        "# Bengal Tiger OS Demo Script\n"
        "echo Running Bengal Tiger OS automated demo...\n"
        "info\n"
        "uptime\n"
        "calc (21 * 2) + 8\n"
        "df\n";

    fat_write_file("HELLO.TXT", hello, (uint32_t)strlen(hello));
    fat_write_file("README.TXT", readme, (uint32_t)strlen(readme));
    fat_write_file("DEMO.SH", demo_sh, (uint32_t)strlen(demo_sh));
    fat_write_file("CONFIG.CFG", "tiger", 5);
}

int fat_format(const char *label, int use_hardware_ata) {
    fs_mounted = 0;

    if (use_hardware_ata) {
        if (disk_enable_ramdisk(0) != 0) return -1;
    } else {
        if (disk_enable_ramdisk(1) != 0) return -1;
    }

    uint32_t disk_secs = disk_get_total_sectors();
    if (disk_secs < 64) return -1;

    /* Cap single FAT16 volume to 512MB (1,048,576 sectors) */
    if (disk_secs > 1048576U) {
        disk_secs = 1048576U;
    }

    uint8_t spc = 1;
    if (disk_secs >= 524288U)      spc = 16;
    else if (disk_secs >= 262144U) spc = 8;
    else if (disk_secs >= 16384U)  spc = 4;
    else if (disk_secs >= 8192U)   spc = 2;
    else                           spc = 1;

    uint16_t reserved = 1;
    uint8_t  num_fats = 2;
    uint16_t root_ents = (disk_secs <= 2048) ? 112 : 512;
    uint32_t r_secs = ((uint32_t)root_ents * 32 + 511) / 512;

    /* Estimate clusters to determine FAT12 vs FAT16 */
    uint32_t approx_clusters = (disk_secs - reserved - r_secs) / spc;
    int target_fat = (approx_clusters < 4085) ? 12 : 16;

    uint32_t fat_bytes = (target_fat == 12)
        ? (((approx_clusters + 2) * 3 + 1) / 2)
        : ((approx_clusters + 2) * 2);
    uint16_t fat_secs = (uint16_t)((fat_bytes + 511) / 512);
    if (fat_secs == 0) fat_secs = 1;

    /* Recalculate exact cluster count and ensure we don't sit in ambiguous boundary */
    uint32_t data_secs = disk_secs - reserved - ((uint32_t)num_fats * fat_secs) - r_secs;
    uint32_t final_clusters = data_secs / spc;
    if (target_fat == 16 && final_clusters < 4085) {
        target_fat = 12;
    }

    uint8_t sec[512];
    memset(sec, 0, sizeof(sec));
    fat_boot_t *b = (fat_boot_t*)sec;

    b->jump[0] = 0xEB; b->jump[1] = 0x3C; b->jump[2] = 0x90;
    memcpy(b->oem, "BENGALOS", 8);
    b->bytes_per_sector    = 512;
    b->sectors_per_cluster = spc;
    b->reserved_sectors    = reserved;
    b->num_fats            = num_fats;
    b->root_entries        = root_ents;
    if (disk_secs < 65536U) {
        b->total_sectors_16 = (uint16_t)disk_secs;
        b->total_sectors_32 = 0;
    } else {
        b->total_sectors_16 = 0;
        b->total_sectors_32 = disk_secs;
    }
    b->media_type        = 0xF8;
    b->fat_size_16       = fat_secs;
    b->sectors_per_track = 32;
    b->num_heads         = 64;
    b->hidden_sectors    = 0;
    b->drive_num         = 0x80;
    b->boot_sig          = 0x29;
    b->vol_id            = 0x20261031U;

    memset(b->vol_label, ' ', 11);
    if (label && label[0]) {
        for (int i = 0; i < 11 && label[i]; i++) {
            char c = label[i];
            if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
            b->vol_label[i] = c;
        }
    } else {
        memcpy(b->vol_label, "BENGAL OS  ", 11);
    }

    memcpy(b->fs_type, (target_fat == 12) ? "FAT12   " : "FAT16   ", 8);
    sec[510] = 0x55;
    sec[511] = 0xAA;

    if (ata_write_sector(0, sec) != 0) return -1;

    /* Clear and initialize FAT1 & FAT2 */
    uint8_t zero_sec[512];
    memset(zero_sec, 0, sizeof(zero_sec));

    for (uint8_t f = 0; f < num_fats; f++) {
        uint32_t f_base = reserved + (uint32_t)f * fat_secs;
        for (uint16_t s = 0; s < fat_secs; s++) {
            if (s == 0) {
                uint8_t fat0[512];
                memset(fat0, 0, sizeof(fat0));
                if (target_fat == 12) {
                    fat0[0] = 0xF8;
                    fat0[1] = 0xFF;
                    fat0[2] = 0xFF;
                } else {
                    fat0[0] = 0xF8; fat0[1] = 0xFF;
                    fat0[2] = 0xFF; fat0[3] = 0xFF;
                }
                if (ata_write_sector(f_base, fat0) != 0) return -1;
            } else {
                if (ata_write_sector(f_base + s, zero_sec) != 0) return -1;
            }
        }
    }

    /* Zero root directory sectors */
    uint32_t root_base = reserved + (uint32_t)num_fats * fat_secs;
    for (uint32_t s = 0; s < r_secs; s++) {
        if (ata_write_sector(root_base + s, zero_sec) != 0) return -1;
    }

    if (mount_at_lba(0) != 0) return -1;

    seed_default_files();
    return 0;
}

void fat_init(void) {
    fs_mounted = 0;

    if (disk_has_hardware_hdd()) {
        disk_enable_ramdisk(0);
        if (try_mount_disk() == 0) {
            klog_writeln("fat", "Mounted persistent ATA FAT volume");
            return;
        }
        klog_writeln("fat", "ATA disk unformatted; activating Live RAMDisk (run 'mkfs' to format HDD)");
    }

    /* Fallback to 512KB Live FAT RAMDisk so all file ops work out-of-the-box */
    if (fat_format("BENGALLIVE", 0) == 0) {
        klog_writeln("fat", "Initialized 512KB Live FAT RAMDisk");
    } else {
        klog_writeln("fat", "Failed to initialize filesystem");
    }
}

int fat_is_mounted(void) {
    return fs_mounted;
}

static uint32_t get_fat_entry(uint32_t cluster) {
    if (!fs_mounted || cluster < 2) return eoc_marker();

    uint32_t fat_offset = (fat_type == 12) ? (cluster + (cluster / 2)) : (cluster * 2);
    uint32_t sector = fat_start_sector + (fat_offset / 512);
    uint32_t offset = fat_offset % 512;

    uint8_t buf[1024];
    if (ata_read_sector(sector, buf) != 0) return eoc_marker();
    if (offset == 511) {
        if (ata_read_sector(sector + 1, buf + 512) != 0) return eoc_marker();
    }

    uint16_t raw = rd_le16(&buf[offset]);
    if (fat_type == 12) {
        return (cluster & 1U) ? (uint32_t)(raw >> 4) : (uint32_t)(raw & 0x0FFFU);
    }
    return (uint32_t)raw;
}

static void set_fat_entry(uint32_t cluster, uint32_t value) {
    if (!fs_mounted || cluster < 2) return;

    uint32_t fat_offset = (fat_type == 12) ? (cluster + (cluster / 2)) : (cluster * 2);
    uint32_t sec_off    = fat_offset / 512;
    uint32_t offset     = fat_offset % 512;

    uint8_t buf[1024];
    if (ata_read_sector(fat_start_sector + sec_off, buf) != 0) return;
    if (offset == 511) {
        if (ata_read_sector(fat_start_sector + sec_off + 1, buf + 512) != 0) return;
    }

    if (fat_type == 12) {
        uint16_t entry = rd_le16(&buf[offset]);
        if (cluster & 1U) {
            entry = (uint16_t)((entry & 0x000FU) | ((value & 0x0FFFU) << 4));
        } else {
            entry = (uint16_t)((entry & 0xF000U) | (value & 0x0FFFU));
        }
        wr_le16(&buf[offset], entry);
    } else {
        wr_le16(&buf[offset], (uint16_t)(value & 0xFFFFU));
    }

    for (uint8_t f = 0; f < boot_sector.num_fats; f++) {
        uint32_t f_sec = fat_start_sector + ((uint32_t)f * boot_sector.fat_size_16) + sec_off;
        ata_write_sector(f_sec, buf);
        if (offset == 511) {
            ata_write_sector(f_sec + 1, buf + 512);
        }
    }
}

static void free_cluster_chain(uint32_t start_cluster) {
    uint32_t c = start_cluster;
    uint32_t guard = total_clusters + 2;
    while (c >= 2 && c < eoc_threshold() && guard-- > 0) {
        uint32_t next = get_fat_entry(c);
        set_fat_entry(c, 0x0000);
        c = next;
    }
}

static uint32_t allocate_cluster(void) {
    uint32_t max_c = total_clusters + 2;
    for (uint32_t c = 2; c < max_c; c++) {
        if (get_fat_entry(c) == 0x0000) {
            set_fat_entry(c, eoc_marker());
            /* Zero cluster data sectors */
            uint8_t zero[512];
            memset(zero, 0, sizeof(zero));
            uint32_t sec = data_start_sector + (c - 2) * boot_sector.sectors_per_cluster;
            for (uint8_t s = 0; s < boot_sector.sectors_per_cluster; s++) {
                ata_write_sector(sec + s, zero);
            }
            return c;
        }
    }
    return 0;
}

static int filename_to_fat(const char *name, char *fat_name) {
    if (!name || !name[0]) return -1;
    while (*name == '/' || *name == ' ') name++;
    if (!*name) return -1;

    memset(fat_name, ' ', 11);
    int i = 0, j = 0;

    while (name[i] && name[i] != '.' && j < 8) {
        char c = name[i++];
        if (c == ' ' || c == '/') return -1;
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        fat_name[j++] = c;
    }
    while (name[i] && name[i] != '.') i++;

    if (name[i] == '.') {
        i++;
        j = 8;
        while (name[i] && j < 11) {
            char c = name[i++];
            if (c == ' ' || c == '/') return -1;
            if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
            fat_name[j++] = c;
        }
    }
    return (fat_name[0] == ' ') ? -1 : 0;
}

static void fat_name_to_str(const fat_dirent_t *ent, char *out) {
    int p = 0;
    for (int k = 0; k < 8; k++) {
        if (ent->name[k] != ' ') out[p++] = ent->name[k];
    }
    if (ent->ext[0] != ' ') {
        out[p++] = '.';
        for (int k = 0; k < 3; k++) {
            if (ent->ext[k] != ' ') out[p++] = ent->ext[k];
        }
    }
    out[p] = '\0';
}

static int find_dirent_val(const char *name, fat_dirent_t *out_ent,
                           uint32_t *out_sector, uint32_t *out_index) {
    if (!fs_mounted) return -1;

    char fat_name[11];
    if (filename_to_fat(name, fat_name) != 0) return -1;

    uint8_t buf[512];
    for (uint32_t s = 0; s < root_sectors; s++) {
        if (ata_read_sector(root_start_sector + s, buf) != 0) break;

        fat_dirent_t *entries = (fat_dirent_t*)buf;
        for (uint32_t i = 0; i < 16; i++) {
            uint8_t first = (uint8_t)entries[i].name[0];
            if (first == 0x00) return -1;
            if (first == 0xE5) continue;
            if (entries[i].attr == FAT_ATTR_LFN) continue;
            if (entries[i].attr & FAT_ATTR_VOLUME_ID) continue;

            if (memcmp(entries[i].name, fat_name, 11) == 0) {
                if (out_ent)    *out_ent    = entries[i];
                if (out_sector) *out_sector = root_start_sector + s;
                if (out_index)  *out_index  = i;
                return 0;
            }
        }
    }
    return -1;
}

int fat_file_exists(const char *name) {
    return (find_dirent_val(name, NULL, NULL, NULL) == 0) ? 1 : 0;
}

int fat_stat_file(const char *name, fat_file_t *out_info) {
    fat_dirent_t ent;
    if (find_dirent_val(name, &ent, NULL, NULL) != 0) return -1;
    if (out_info) {
        fat_name_to_str(&ent, out_info->name);
        out_info->attr          = ent.attr;
        out_info->size          = ent.size;
        out_info->first_cluster = ent.cluster_low | ((uint32_t)ent.cluster_high << 16);
    }
    return 0;
}

int fat_read_file(const char *name, void *buf, uint32_t max_len, uint32_t *out_size) {
    if (!buf && max_len > 0) return -1;

    fat_dirent_t ent;
    if (find_dirent_val(name, &ent, NULL, NULL) != 0) {
        if (out_size) *out_size = 0;
        return -1;
    }

    if (out_size) *out_size = ent.size;
    if (max_len == 0 || ent.size == 0) return 0;

    uint32_t to_read = (ent.size < max_len) ? ent.size : max_len;
    uint32_t cluster = ent.cluster_low | ((uint32_t)ent.cluster_high << 16);
    uint8_t *dst     = (uint8_t*)buf;
    uint32_t left    = to_read;
    uint32_t guard   = total_clusters + 2;

    while (cluster >= 2 && cluster < eoc_threshold() && left > 0 && guard-- > 0) {
        uint32_t sec = data_start_sector + (cluster - 2) * boot_sector.sectors_per_cluster;
        for (uint8_t s = 0; s < boot_sector.sectors_per_cluster && left > 0; s++) {
            uint8_t sec_buf[512];
            if (ata_read_sector(sec + s, sec_buf) != 0) return -1;

            uint32_t chunk = (left > 512) ? 512 : left;
            memcpy(dst, sec_buf, chunk);
            dst  += chunk;
            left -= chunk;
        }
        cluster = get_fat_entry(cluster);
    }

    return 0;
}

int fat_write_file(const char *name, const void *buf, uint32_t len) {
    if (!fs_mounted || !name) return -1;
    if (len > 0 && !buf) return -1;

    char fat_name[11];
    if (filename_to_fat(name, fat_name) != 0) return -1;

    fat_dirent_t ent;
    uint32_t dir_sec = 0, dir_idx = 0;
    int exists = (find_dirent_val(name, &ent, &dir_sec, &dir_idx) == 0);

    if (exists) {
        uint32_t old_cluster = ent.cluster_low | ((uint32_t)ent.cluster_high << 16);
        if (old_cluster >= 2) {
            free_cluster_chain(old_cluster);
        }
    }

    /* Allocate new cluster chain for len bytes (0-byte file needs 0 clusters) */
    uint32_t first_cluster = 0;
    if (len > 0) {
        first_cluster = allocate_cluster();
        if (first_cluster < 2) return -1;

        uint32_t written = 0;
        uint32_t cur_c   = first_cluster;
        const uint8_t *src = (const uint8_t*)buf;

        while (written < len) {
            uint32_t sec = data_start_sector + (cur_c - 2) * boot_sector.sectors_per_cluster;
            for (uint8_t s = 0; s < boot_sector.sectors_per_cluster && written < len; s++) {
                uint8_t wbuf[512];
                memset(wbuf, 0, sizeof(wbuf));
                uint32_t chunk = (len - written > 512) ? 512 : (len - written);
                memcpy(wbuf, src + written, chunk);
                if (ata_write_sector(sec + s, wbuf) != 0) return -1;
                written += chunk;
            }
            if (written < len) {
                uint32_t next_c = allocate_cluster();
                if (next_c < 2) return -1;
                set_fat_entry(cur_c, next_c);
                cur_c = next_c;
            } else {
                set_fat_entry(cur_c, eoc_marker());
            }
        }
    }

    if (exists) {
        uint8_t sec_buf[512];
        if (ata_read_sector(dir_sec, sec_buf) != 0) return -1;
        fat_dirent_t *entries = (fat_dirent_t*)sec_buf;
        entries[dir_idx].cluster_low  = (uint16_t)(first_cluster & 0xFFFFU);
        entries[dir_idx].cluster_high = (uint16_t)((first_cluster >> 16) & 0xFFFFU);
        entries[dir_idx].size         = len;
        entries[dir_idx].attr         = FAT_ATTR_ARCHIVE;
        return ata_write_sector(dir_sec, sec_buf);
    } else {
        uint8_t sec_buf[512];
        for (uint32_t s = 0; s < root_sectors; s++) {
            if (ata_read_sector(root_start_sector + s, sec_buf) != 0) break;
            fat_dirent_t *entries = (fat_dirent_t*)sec_buf;
            for (uint32_t i = 0; i < 16; i++) {
                uint8_t first = (uint8_t)entries[i].name[0];
                if (first == 0x00 || first == 0xE5) {
                    memset(&entries[i], 0, sizeof(fat_dirent_t));
                    memcpy(entries[i].name, fat_name, 11);
                    entries[i].attr         = FAT_ATTR_ARCHIVE;
                    entries[i].cluster_low  = (uint16_t)(first_cluster & 0xFFFFU);
                    entries[i].cluster_high = (uint16_t)((first_cluster >> 16) & 0xFFFFU);
                    entries[i].size         = len;
                    return ata_write_sector(root_start_sector + s, sec_buf);
                }
            }
        }
    }

    return -1;
}

int fat_append_file(const char *name, const void *buf, uint32_t len) {
    if (!fs_mounted || !name || (!buf && len > 0)) return -1;

    fat_file_t st;
    if (fat_stat_file(name, &st) != 0) {
        return fat_write_file(name, buf, len);
    }

    if (st.size + len > FAT_MAX_FILE_SIZE) return -1;
    uint32_t total = st.size + len;
    uint8_t *combined = (uint8_t*)kmalloc(total ? total : 1);
    if (!combined) return -1;

    if (st.size > 0) {
        if (fat_read_file(name, combined, st.size, NULL) != 0) {
            kfree(combined);
            return -1;
        }
    }
    if (len > 0) {
        memcpy(combined + st.size, buf, len);
    }

    int rc = fat_write_file(name, combined, total);
    kfree(combined);
    return rc;
}

int fat_delete_file(const char *name) {
    fat_dirent_t ent;
    uint32_t dir_sec = 0, dir_idx = 0;
    if (find_dirent_val(name, &ent, &dir_sec, &dir_idx) != 0) return -1;

    uint32_t first_cluster = ent.cluster_low | ((uint32_t)ent.cluster_high << 16);
    if (first_cluster >= 2) {
        free_cluster_chain(first_cluster);
    }

    uint8_t sec_buf[512];
    if (ata_read_sector(dir_sec, sec_buf) != 0) return -1;
    fat_dirent_t *entries = (fat_dirent_t*)sec_buf;
    entries[dir_idx].name[0] = (char)0xE5;
    return ata_write_sector(dir_sec, sec_buf);
}

int fat_copy_file(const char *src, const char *dst) {
    if (!src || !dst) return -1;
    if (strcasecmp(src, dst) == 0) return 0;

    fat_file_t st;
    if (fat_stat_file(src, &st) != 0) return -1;
    if (st.size > FAT_MAX_FILE_SIZE) return -1;

    if (st.size == 0) {
        return fat_write_file(dst, "", 0);
    }

    uint8_t *buf = (uint8_t*)kmalloc(st.size);
    if (!buf) return -1;

    int rc = -1;
    if (fat_read_file(src, buf, st.size, NULL) == 0) {
        rc = fat_write_file(dst, buf, st.size);
    }
    kfree(buf);
    return rc;
}

int fat_rename_file(const char *old_name, const char *new_name) {
    if (!old_name || !new_name) return -1;
    if (strcasecmp(old_name, new_name) == 0) return 0;
    if (fat_file_exists(new_name)) return -1;

    char new_fat[11];
    if (filename_to_fat(new_name, new_fat) != 0) return -1;

    fat_dirent_t ent;
    uint32_t dir_sec = 0, dir_idx = 0;
    if (find_dirent_val(old_name, &ent, &dir_sec, &dir_idx) != 0) return -1;

    uint8_t sec_buf[512];
    if (ata_read_sector(dir_sec, sec_buf) != 0) return -1;
    fat_dirent_t *entries = (fat_dirent_t*)sec_buf;
    memcpy(entries[dir_idx].name, new_fat, 11);
    return ata_write_sector(dir_sec, sec_buf);
}

void fat_get_fs_info(fat_fs_info_t *out_info) {
    if (!out_info) return;
    memset(out_info, 0, sizeof(fat_fs_info_t));

    if (!fs_mounted) return;

    out_info->mounted           = 1;
    out_info->fat_type          = fat_type;
    out_info->is_ramdisk        = disk_is_ramdisk();
    out_info->volume_start_lba  = vol_lba_offset;
    out_info->bytes_per_cluster = (uint32_t)boot_sector.sectors_per_cluster * 512;
    out_info->total_clusters    = total_clusters;

    uint32_t used = 0;
    for (uint32_t c = 2; c < total_clusters + 2; c++) {
        if (get_fat_entry(c) != 0x0000) used++;
    }
    out_info->used_clusters = used;
    out_info->free_clusters = (total_clusters >= used) ? (total_clusters - used) : 0;
    out_info->total_bytes   = total_clusters * out_info->bytes_per_cluster;
    out_info->used_bytes    = used * out_info->bytes_per_cluster;
    out_info->free_bytes    = out_info->free_clusters * out_info->bytes_per_cluster;

    memcpy(out_info->label, boot_sector.vol_label, 11);
    out_info->label[11] = '\0';
    for (int i = 10; i >= 0 && out_info->label[i] == ' '; i--) {
        out_info->label[i] = '\0';
    }

    uint8_t buf[512];
    uint32_t files = 0;
    for (uint32_t s = 0; s < root_sectors; s++) {
        if (ata_read_sector(root_start_sector + s, buf) != 0) break;
        fat_dirent_t *entries = (fat_dirent_t*)buf;
        for (int i = 0; i < 16; i++) {
            uint8_t first = (uint8_t)entries[i].name[0];
            if (first == 0x00) { out_info->file_count = files; return; }
            if (first == 0xE5 || entries[i].attr == FAT_ATTR_LFN) continue;
            if (entries[i].attr & FAT_ATTR_VOLUME_ID) continue;
            files++;
        }
    }
    out_info->file_count = files;
}

int fat_enumerate_files(char names[][16], int max_files) {
    if (!fs_mounted || !names || max_files <= 0) return 0;

    uint8_t buf[512];
    int count = 0;
    for (uint32_t s = 0; s < root_sectors && count < max_files; s++) {
        if (ata_read_sector(root_start_sector + s, buf) != 0) break;
        fat_dirent_t *entries = (fat_dirent_t*)buf;
        for (int i = 0; i < 16 && count < max_files; i++) {
            uint8_t first = (uint8_t)entries[i].name[0];
            if (first == 0x00) return count;
            if (first == 0xE5 || entries[i].attr == FAT_ATTR_LFN) continue;
            if (entries[i].attr & FAT_ATTR_VOLUME_ID) continue;
            fat_name_to_str(&entries[i], names[count++]);
        }
    }
    return count;
}

void fat_load_file(const char *name, void *buf) {
    if (!buf) return;
    memset(buf, 0, 512);
    fat_read_file(name, buf, 511, NULL);
}

void fat_save_file(const char *name, const void *buf) {
    if (!name || !buf) return;
    fat_write_file(name, buf, (uint32_t)strlen((const char*)buf));
}

void fat_list_files(void) {
    if (!fs_mounted) {
        shell_print("No filesystem mounted. Run 'mkfs' to format disk.\n");
        return;
    }

    fat_fs_info_t info;
    fat_get_fs_info(&info);

    shell_print_color("\nVolume: ", 0x0B);
    shell_print(info.label[0] ? info.label : "NO NAME");
    shell_print(" (FAT");
    shell_print_int(info.fat_type);
    shell_print(info.is_ramdisk ? ", Live RAMDisk)\n" : ", Persistent ATA Disk)\n");

    shell_print("Name          Size        Type\n");
    shell_print("------------  ----------  ----\n");

    uint8_t buf[512];
    int shown = 0;
    for (uint32_t s = 0; s < root_sectors; s++) {
        if (ata_read_sector(root_start_sector + s, buf) != 0) break;

        fat_dirent_t *entries = (fat_dirent_t*)buf;
        for (int i = 0; i < 16; i++) {
            uint8_t first = (uint8_t)entries[i].name[0];
            if (first == 0x00) goto done;
            if (first == 0xE5 || entries[i].attr == FAT_ATTR_LFN) continue;
            if (entries[i].attr & FAT_ATTR_VOLUME_ID) continue;

            char name[13];
            fat_name_to_str(&entries[i], name);

            shell_print_color(name, (entries[i].attr & FAT_ATTR_DIRECTORY) ? 0x0B : 0x0E);
            for (int k = strlen(name); k < 14; k++) shell_print(" ");

            char sbuf[16];
            k_utoa(entries[i].size, sbuf, 10);
            shell_print(sbuf);
            shell_print(" B");
            for (int k = strlen(sbuf) + 2; k < 12; k++) shell_print(" ");

            shell_print((entries[i].attr & FAT_ATTR_DIRECTORY) ? "DIR\n" : "FILE\n");
            shown++;
        }
    }

done:
    shell_print("------------  ----------  ----\n");
    shell_print_int(shown);
    shell_print(" file(s), ");
    shell_print_int((int)(info.free_bytes / 1024));
    shell_print(" KB free / ");
    shell_print_int((int)(info.total_bytes / 1024));
    shell_print(" KB total\n\n");
}
