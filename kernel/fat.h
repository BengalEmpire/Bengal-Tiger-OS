#ifndef FAT_H
#define FAT_H

void fat_init();
void fat_load_file(const char *name, void *buf);
void fat_save_file(const char *name, void *buf);

#endif