#include <sys/wait.h>
#include <ncurses.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_FILES 1024
#define PATH_MAX_LEN 4096

char *files[MAX_FILES];
int file_count = 0;
int selected = 0;
char current_path[PATH_MAX_LEN];

void load_directory(const char *path) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    file_count = 0;

    if (!dir) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        files[file_count++] = strdup(entry->d_name);
    }
    closedir(dir);
}

int is_directory(const char *path, const char *filename) {
    struct stat st;
    char full_path[PATH_MAX_LEN];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, filename);
    if (stat(full_path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

int is_image(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return 0;
    return strcmp(ext, ".jpg") == 0 || strcmp(ext, ".png") == 0 || strcmp(ext, ".jpeg") == 0;
}

void set_wallpaper(const char *path) {
    pid_t pid = fork();
    if (pid == 0) {
        // In child process
        execlp("swww", "swww", "img",
               "--transition-type", "wipe",
               "--transition-fps", "60",
               "--transition-angle", "30",
               "--transition-duration", "1.2",
               path,
               (char *)NULL);
        _exit(1); // only runs if execlp fails
    } else if (pid > 0) {
        // In parent process: wait for child to finish
        int status;
        waitpid(pid, &status, 0);
    }
}

void free_files() {
    for (int i = 0; i < file_count; i++) {
        free(files[i]);
        files[i] = NULL;
    }
    file_count = 0;
}


int main() {
    strcpy(current_path, "xdg-user-dir PICTURES");

    load_directory(current_path);

    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);

    int ch;

    while (1) {
        clear();

	// Header
	attron(A_BOLD | A_UNDERLINE);
	mvprintw(0, 0, "📁 Wallpaper Picker - %s", current_path);
	attroff(A_BOLD | A_UNDERLINE);

	// File list
	for (int i = 0; i < file_count; i++) {
	    if (i == selected) attron(A_REVERSE);
	    
	    // Show [DIR] before directories
	    if (is_directory(current_path, files[i])) {
	        mvprintw(i + 1, 0, "[DIR] %s", files[i]);
	    } else {
	        mvprintw(i + 1, 0, "      %s", files[i]);
	    }

	    if (i == selected) attroff(A_REVERSE);
	}

        // Footer
        int row, col;
        getmaxyx(stdscr, row, col);
        attron(A_DIM);
        mvprintw(row - 2, 0, "↑↓ to move | Enter to open/set | q to quit");
        attroff(A_DIM);


        refresh();

        ch = getch();
        if (ch == KEY_UP && selected > 0) {
            selected--;
        } else if (ch == KEY_DOWN && selected < file_count - 1) {
            selected++;
        } else if (ch == '\n') {
            char new_path[PATH_MAX_LEN];
            snprintf(new_path, sizeof(new_path), "%s/%s", current_path, files[selected]);

            if (is_directory(current_path, files[selected])) {
                if (strcmp(files[selected], "..") == 0) {
                    char *last_slash = strrchr(current_path, '/');
                    if (last_slash && last_slash != current_path) {
                        *last_slash = '\0';
                    } else {
                        strcpy(current_path, "/");
                    }
                } else {
                    strcat(current_path, "/");
                    strcat(current_path, files[selected]);
                }
                

		// free old file list
		free_files();
                
		load_directory(current_path);
                selected = 0;
            } else if (is_image(files[selected])) {
                char img_path[PATH_MAX_LEN];
                snprintf(img_path, sizeof(img_path), "%s/%s", current_path, files[selected]);
            	set_wallpaper(img_path);
            }
        } else if (ch == 'q') {
            break;
        }
    }

    endwin();
    free_files();
    return 0;
}

