/*
Copyright 2016 Seth VanHeulen

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <3ds.h>
#include <stdlib.h>
#include <stdio.h>
#include "titles.h"

void refresh() {
    gspWaitForVBlank();
    gfxFlushBuffers();
    gfxSwapBuffers();
}

void display_menu() {
    consoleClear();
    printf("press (A) to check titles\n");
    printf("press (X) to output titles to file\n");
    printf("press (B) to return on Homebrew Launcher\n");
    refresh();
}

void log_titles() {
    consoleClear();
    printf("Writing installed titles to file... ");
    refresh();
    amInit();
    u32 installed_title_count = 0;
    AM_GetTitleCount(MEDIATYPE_NAND, &installed_title_count);
    u64 * installed_title_ids = malloc(installed_title_count * sizeof(u64));
    AM_GetTitleIdList(MEDIATYPE_NAND, installed_title_count, installed_title_ids);
    AM_TitleEntry * installed_titles = malloc(installed_title_count * sizeof(AM_TitleEntry));
    AM_ListTitles(MEDIATYPE_NAND, installed_title_count, installed_title_ids, installed_titles);
    amExit();
    free(installed_title_ids);
    FILE * output = fopen("titles.h", "wb");
    fprintf(output, "#include <3ds.h>\n\nconst AM_TitleEntry titles[] = {\n");
    for (u32 i = 0; i < installed_title_count; i++) {
        fprintf(output, "    {0x%016llxLL, 0x0LL, 0x%04x},\n", installed_titles[i].titleID, installed_titles[i].version);
    }
    fprintf(output, "};\n\n");
    fprintf(output, "const u32 title_count = %lu;\n\n", installed_title_count);
    fclose(output);
    free(installed_titles);
    printf("complete.\n");
    refresh();
}

void check_titles() {
    consoleClear();
    printf("Checking installed titles...\n");
    refresh();
    amInit();
    u32 installed_title_count = 0;
    AM_GetTitleCount(MEDIATYPE_NAND, &installed_title_count);
    u64 * installed_title_ids = malloc(installed_title_count * sizeof(u64));
    AM_GetTitleIdList(MEDIATYPE_NAND, installed_title_count, installed_title_ids);
    AM_TitleEntry * installed_titles = malloc(installed_title_count * sizeof(AM_TitleEntry));
    AM_ListTitles(MEDIATYPE_NAND, installed_title_count, installed_title_ids, installed_titles);
    amExit();
    free(installed_title_ids);
    u8 found = 0;
    FILE * output = fopen("check.log", "wb");
    for (u32 i = 0; i < installed_title_count; i++) {
        for (u32 j = 0; j < title_count; j++) {
            if (installed_titles[i].titleID == titles[j].titleID) {
                if (installed_titles[i].version != titles[j].version) {
                    printf("    Mismatched title ID 0x%016llx\n", titles[j].titleID);
                    refresh();
                    fprintf(output, "Misatched title ID 0x%016llx\n", titles[j].titleID);
                }
                found = 1;
                break;
            }
        }
        if (found == 0) {
            printf("    Extra title ID 0x%016llx\n", installed_titles[i].titleID);
            refresh();
            fprintf(output, "Extra title ID 0x%016llx\n", installed_titles[i].titleID);
        }
        found = 0;
    }
    printf("Complete.\n");
    printf("Checking missing titles...\n");
    refresh();
    for (u32 i = 0; i < title_count; i++) {
        for (u32 j = 0; j < installed_title_count; j++) {
            if (titles[i].titleID == installed_titles[j].titleID) {
                found = 1;
                break;
            }
        }
        if (found == 0) {
            printf("    Missing title ID 0x%016llx\n", titles[i].titleID);
            refresh();
            fprintf(output, "Missing title ID 0x%016llx\n", titles[i].titleID);
        }
        found = 0;
    }
    fclose(output);
    free(installed_titles);
    printf("Complete.\n");
    refresh();
}

int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    display_menu();
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_A) {
            check_titles();
            svcSleepThread(5000000000);
            display_menu();
        }
        if (kDown & KEY_X) {
            log_titles();
            svcSleepThread(5000000000);
            display_menu();
        }
        if (kDown & KEY_B) {
            break;
        }
    }
    gfxExit();
    return 0;
}

