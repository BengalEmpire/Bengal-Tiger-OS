/**
 * Bengal Tiger OS - Built-in Full-Screen Text Editor (`edit`)
 *
 * Interactive text editor supporting VBE graphical consoles, VGA 80x25 text
 * mode, and VT100/ANSI serial terminals. Reads and writes files directly on
 * the mounted FAT12/FAT16 volume.
 *
 * Controls:
 *   Arrows / Home / End / PgUp / PgDn - Move cursor
 *   Ctrl+S                            - Save file
 *   Ctrl+Q or ESC                     - Quit editor (warns if unsaved)
 *
 * @file editor.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 1.0.0
 */

#ifndef EDITOR_H
#define EDITOR_H

#include "common.h"

int editor_open(const char *filename);

#endif /* EDITOR_H */
