#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ignore.h"
#include "log.h"

const char *evil_hardcoded_ignore_files[] = {
    ".",
    "..",
    NULL
};

// TODO: make this a sorted array so filtering is O(log(n)) instead of O(n)
char **ignore_patterns = NULL;
int ignore_patterns_len = 0;

const int fnmatch_flags = 0 & FNM_PATHNAME;

void add_ignore_pattern(const char* pattern) {
    ignore_patterns = realloc(ignore_patterns, (ignore_patterns_len + 1) * sizeof(char**));
    ignore_patterns[ignore_patterns_len] = strdup(pattern);
    ignore_patterns_len++;
}

void cleanup_ignore_patterns() {
    for(int i = 0; i<ignore_patterns_len; i++) {
        free(ignore_patterns[i]);
    }
    free(ignore_patterns);
}

// For loading git/svn/hg ignore patterns
void load_ignore_patterns(const char *ignore_filename) {
    FILE *fp = NULL;
    fp = fopen(ignore_filename, "r");
    if (fp == NULL) {
        log_debug("Skipping ignore file %s", ignore_filename);
        return;
    }

    char *line = NULL;
    ssize_t line_length = 0;
    size_t line_cap = 0;

    while((line_length = getline(&line, &line_cap, fp)) > 0) {
        line[line_length-1] = '\0'; //kill the \n
        add_ignore_pattern(line);
    }
}

// this function is REALLY HOT. It gets called for every file
int filename_filter(struct dirent *dir) {
/*    if (dir->d_type != DT_REG && dir->d_type != DT_DIR) {
        log_debug("file %s ignored becaused of type", dir->d_name);
        return(0);
    }
*/
    char *filename = dir->d_name;
    char *pattern = NULL;
    // TODO: check if opts want to ignore hidden files
    if (filename[0] == '.') {
        return(0);
    }
    for (int i = 0; evil_hardcoded_ignore_files[i] != NULL; i++) {
        if (strcmp(filename, evil_hardcoded_ignore_files[i]) == 0) {
            log_err("file %s ignored because of name", filename);
            return(0);
        }
    }

    for (int i = 0; i<ignore_patterns_len; i++) {
        pattern = ignore_patterns[i];
        if (fnmatch(pattern, filename, fnmatch_flags) == 0) {
            log_err("file %s ignored because name matches pattern %s", dir->d_name, pattern);
            return(0);
        }
    }

    return(1);
}