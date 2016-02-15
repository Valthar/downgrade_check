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
#include "description.h"

AM_TitleEntry * titles = NULL;
u32 title_count = 0;

char *getDesc(u64);

void refresh() {
    gspWaitForVBlank();
    gfxFlushBuffers();
    gfxSwapBuffers();
}

void display_menu() {
    consoleClear();
	printf("\x1b[33;1mdowngrade_check 2.0   \x1b[0mby svanheulen\n\n");
    printf("Press (A) to check titles\n");
    printf("Press (X) to output titles to file\n");
    printf("Press (B) to return on Homebrew Launcher\n");
    refresh();
}

void pause() {
    printf("\nPress (A) to continue\n");
    refresh();
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_A) {
            break;
        }
    }
	svcSleepThread(250*1000*1000);
}

char *getDesc(u64 tid){

	char *d="N/A-undocumented";
	
	for(u32 i=0;i<size_desc;i++){
		if(desc[i].tid==tid){
			return desc[i].name;
		}
	}
	return d;
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
	
	FILE * tits = fopen("installed_titles.log", "wb");
	
	fprintf(tits, "TitleID           Vers   Description\n\n");
	for (u32 i = 0; i < installed_title_count; i++) {
        fprintf(tits, "%016llx  %05d  %s\n", installed_titles[i].titleID, installed_titles[i].version,  getDesc(installed_titles[i].titleID));
    }
	fprintf(tits, "\nTotal installed titles: %lu\n", installed_title_count);
	fprintf(tits, "(dsiware games, tid high 00048004, are included and will be 'N/A-undocumented')");
	fclose(tits);
	
    FILE * output = fopen("title_lists.py", "wb");
    fprintf(output, "title_lists = {\n    'my_title_list.bin': (\n");
    for (u32 i = 0; i < installed_title_count; i++) {
        if ((installed_titles[i].titleID & 0xffffffff00000000LL) == 0x0004800400000000LL)
            continue;
        if ((i % 5) == 0)
            fprintf(output, "        ");
        fprintf(output, "(0x%016llx, 0x%04x),", installed_titles[i].titleID, installed_titles[i].version);
        if (((i + 1) % 5) == 0 || i == (installed_title_count - 1))
            fprintf(output, "\n");
        else
            fprintf(output, " ");
    }
    fprintf(output, "    ),\n}\n");
    fclose(output);
    free(installed_titles);
    printf("complete.\n");
    refresh();
    pause();
}

int load_titles() {
    u8 is_new = 0; 
    APT_CheckNew3DS(&is_new);
    OS_VersionBin nver, cver;
    osGetSystemVersionData(&nver, &cver);
    char titles_binary_file[30];
    sprintf(titles_binary_file, "%s_%u.%u.%u-%u%c.bin", is_new ? "n3ds" : "o3ds", cver.mainver, cver.minor, cver.build, nver.mainver, nver.region);
	
	FILE * titles_default = fopen("default.bin", "rb");   //in case we want to force the system to check a title list regardless of matching firmware -- for research
    if (titles_default) {
        fread(&title_count, 4, 1, titles_default);
        titles = malloc(title_count * sizeof(AM_TitleEntry));
        fread(titles, sizeof(AM_TitleEntry), title_count, titles_default);
        fclose(titles_default);
        return 1;
    }
	
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

void clearMissmatchedTitles(){
	consoleClear();
	printf("[MISMATCHED VERSIONS]\n\n");
	printf("Description\n");
	printf("TitleID          Vers: Expected Installed\n\n");
}
void clearMissingTitles(){
	consoleClear();
	printf("[MISSING TITLES]\n\n");
	printf("Description\n");
	printf("TitleID                Vers.\n\n");
}
void clearExtraTitles(){
	consoleClear();
	printf("[EXTRA TITLES]\n\n");
	printf("Description\n");
	printf("TitleID                Vers.\n\n");
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
	u32 line=0;
	u32 linetotal=0;
	u32 change=0;
    FILE * output = fopen("check.log", "wb");
	
	
	clearMissmatchedTitles();
	fprintf(output,"[MISMATCHED VERSIONS]\n\n");
	fprintf(output,"TitleID          Vers: Expected Installed     Description\n\n");

    for (u32 i = 0; i < installed_title_count; i++) {
        if ((installed_titles[i].titleID & 0xffffffff00000000LL) == 0x0004800400000000LL)
            continue;
        for (u32 j = 0; j < title_count; j++) {
            if (installed_titles[i].titleID == titles[j].titleID && installed_titles[i].version != titles[j].version) {
					change = ~change;
					if(change)printf("\x1b[31;1m"); else printf("\x1b[33;1m");
					printf("%s\n", getDesc(titles[j].titleID));
                    printf         ("%016llx       %05d    %05d\n", titles[j].titleID, titles[j].version, installed_titles[i].version);
                    refresh();
                    fprintf(output, "%016llx       %05d    %05d         %s\n", titles[j].titleID, titles[j].version, installed_titles[i].version, getDesc(titles[j].titleID));
					line++;
					break;
            }
        }
		if(line == 10){
			printf("\x1b[0m");
			linetotal+=10;
			printf("\ntitles %lu to %lu", linetotal-10, linetotal-1);
			pause();
			clearMissmatchedTitles();
			line=0;
		}
    }
	
	printf("\x1b[0m");
	printf("\ntitles %lu to %lu", linetotal, linetotal+line);
	pause();
	
	line=0;
	linetotal=0;
	clearExtraTitles();
	fprintf(output,"\n\n[EXTRA TITLES]\n\n");
	fprintf(output,"TitleID                Vers.     Description\n\n");
	
	for (u32 i = 0; i < installed_title_count; i++) {
        if ((installed_titles[i].titleID & 0xffffffff00000000LL) == 0x0004800400000000LL)
            continue;
        for (u32 j = 0; j < title_count; j++) {
            if (installed_titles[i].titleID == titles[j].titleID) {
                    found=1;
					break;
            }
        }
		if(!found){
			change = ~change;
		    if(change)printf("\x1b[31;1m"); else printf("\x1b[33;1m");
			printf("%s\n", getDesc(installed_titles[i].titleID));
			printf("%016llx       %05d\n", installed_titles[i].titleID, installed_titles[i].version);
            refresh();
            fprintf(output, "%016llx       %05d     %s\n", installed_titles[i].titleID, installed_titles[i].version, getDesc(installed_titles[i].titleID));
			line++;
		}
		if(line == 10){
			printf("\x1b[0m");
			linetotal+=10;
			printf("\ntitles %lu to %lu", linetotal-10, linetotal-1);
			pause();
			clearExtraTitles();
			line=0;
		}
		found=0;
    }
	
	printf("\x1b[0m");
	printf("\ntitles %lu to %lu", linetotal, linetotal+line);
	pause();
	
	line=0;
	linetotal=0;
	clearMissingTitles();
	fprintf(output,"\n\n[MISSING TITLES]\n\n");
	fprintf(output,"TitleID                Vers.     Description\n\n");

	for (u32 i = 0; i < title_count; i++) {
  
        for (u32 j = 0; j < installed_title_count; j++) {
            if (titles[i].titleID == installed_titles[j].titleID) {
                    found=1;
					break;
            }
        }
		if(!found){
			change = ~change;
		    if(change)printf("\x1b[31;1m"); else printf("\x1b[33;1m");
			printf("%s\n", getDesc(titles[i].titleID));
			printf("%016llx       %05d\n", titles[i].titleID, titles[i].version);
            refresh();
            fprintf(output, "%016llx       %05d     %s\n", titles[i].titleID, titles[i].version, getDesc(titles[i].titleID));
			line++;
		}
		if(line == 10){
			printf("\x1b[0m");
			linetotal+=10;
			printf("\ntitles %lu to %lu", linetotal-10, linetotal-1);
			pause();
			clearMissingTitles();
			line=0;
		}
		found=0;
    }
	
	printf("\x1b[0m");
	printf("\ntitles %lu to %lu", linetotal, linetotal+line);
	refresh();

    fclose(output);
    free(installed_titles);
    printf("\nComplete.\n");
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
        if (kDown & KEY_X) {
            log_titles();
            display_menu();
        }
        if (kDown & KEY_B) {
            break;
        }
    }
    gfxExit();
    return 0;
}