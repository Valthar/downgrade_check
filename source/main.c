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

AM_TitleEntry * titles = NULL;
u32 title_count = 0;

void refresh() {
    gspWaitForVBlank();
    gfxFlushBuffers();
    gfxSwapBuffers();
}

void display_menu() {
    consoleClear();
    printf("Press (A) to check titles\n");
    printf("Press (B) to return on Homebrew Launcher\n");
    refresh();
}

void pause() {
    printf("Press (A) to continue\n");
    refresh();
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_A) {
            break;
        }
    }
}

int load_titles() {
    u8 is_new = 0;
    APT_CheckNew3DS(&is_new);
    OS_VersionBin nver, cver;
    osGetSystemVersionData(&nver, &cver);
    char titles_binary_file[30];
    sprintf(titles_binary_file, "%s_%u.%u.%u-%u%c.bin", is_new ? "n3ds" : "o3ds", cver.mainver, cver.minor, cver.build, nver.mainver, nver.region);
    FILE * titles_binary = fopen(titles_binary_file, "rb");
    if (titles_binary) {
        fread(&title_count, 4, 1, titles_binary);
        titles = malloc(title_count * sizeof(AM_TitleEntry));
        fread(titles, sizeof(AM_TitleEntry), title_count, titles_binary);
        fclose(titles_binary);
        return 1;
    } else {
        consoleClear();
        printf("Unable to open file: %s\n", titles_binary_file);
        refresh();
        return 0;
    }
}

void free_titles() {
    free(titles);
    titles = NULL;
    title_count = 0;
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
        if ((installed_titles[i].titleID & 0xffffffff00000000LL) == 0x0004800400000000LL)
            continue;
        for (u32 j = 0; j < title_count; j++) {
            if (installed_titles[i].titleID == titles[j].titleID) {
                if (installed_titles[i].version != titles[j].version) {
                    printf("  Title ID 0x%016llx\n", titles[j].titleID);
                    printf("    Expected version 0x%04x\n", titles[j].version);
                    printf("    Installed version 0x%04x\n", installed_titles[i].version);
                    refresh();
                    fprintf(output, "Title ID 0x%016llx\n", titles[j].titleID);
                    fprintf(output, "  Expected version 0x%04x\n", titles[j].version);
                    fprintf(output, "  Installed version 0x%04x\n", installed_titles[i].version);
                }
                found = 1;
                break;
            }
        }
        if (found == 0) {
            printf("  Title ID 0x%016llx\n", installed_titles[i].titleID);
            printf("    Extra title\n");
            refresh();
            fprintf(output, "Title ID 0x%016llx\n", installed_titles[i].titleID);
            fprintf(output, "  Extra title\n");
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
            printf("  Title ID 0x%016llx\n", titles[i].titleID);
            printf("    Missing title\n");
            refresh();
            fprintf(output, "Title ID 0x%016llx\n", titles[i].titleID);
            fprintf(output, "  Missing title\n");
        }
        found = 0;
    }
    fclose(output);
    free(installed_titles);
    printf("Complete.\n");
    refresh();
    pause();
}

int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    display_menu();
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_A) {
            if (load_titles()) {
                check_titles();
                free_titles();
            }
            display_menu();
        }
        if (kDown & KEY_B) {
            break;
        }
    }
    gfxExit();
    return 0;
}

