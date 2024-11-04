#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/stat.h>
#include <syslog.h>

#define EOLN		"\n"

char const cEndLine[] = EOLN;

typedef enum {
    SRT_ABSENT,
    SRT_FILE,
    SRT_DIRECTORY,
} stat_ret_t;

static bool my_write_file(char const *const filename, char const *const data)
{
    FILE *f = fopen(filename, "w");
    if(f == NULL) return false;
    fwrite(data, sizeof(char), strlen(data), f);
    fwrite(cEndLine, sizeof(char), strlen(cEndLine), f);
    fclose(f);
    syslog(LOG_DEBUG, "Writing %s to %s", data, filename);
    return true;
}

static stat_ret_t my_stat(char const *const dir)
{
    struct stat sb;
    if(stat(dir, &sb) != 0) return SRT_ABSENT;
    if(S_ISDIR(sb.st_mode)) return SRT_DIRECTORY;
    return SRT_FILE;
}

static bool my_mkdir(char const *const dir, const mode_t mode)
{
    if(dir == NULL) return true;	// it's wrong, but in this application is the right way
    if(strlen(dir) == 0) return true;	// it's wrong, but in this application is the right way
    if(strcmp(dir,".") == 0) return false;	// here it'll fail
    const stat_ret_t res = my_stat(dir);
    if(res == SRT_FILE) return false;		// the file with such name already exists
    if(res == SRT_DIRECTORY) return true;	// already there - it's a success
    return (mkdir(dir, mode) == 0);
}

static bool recursive_mkdir(char const *const dir, const mode_t mode)
{
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
	    if(!my_mkdir(tmp, mode)) return false;
            *p = '/';
        }
    return my_mkdir(tmp, mode);
}

static int finalize(const int retcode)
{
    closelog();
    return retcode;
}

int main(int argc, char *argv[])
{
    openlog(NULL, LOG_PERROR, LOG_USER);

    // arguments count first
    if(argc != 3) {
	
	syslog(LOG_ERR, "Not enough arguments" EOLN);
	return finalize(1);
    }

    // it would be easier to have a variables for filename and a text
    char const *const file = argv[1];
    char const *const data = argv[2];

    // just to check if the filename is not a directory in a filesystem
    if(my_stat(file) == SRT_DIRECTORY) {
	syslog(LOG_ERR, "\"%s\" is a directory" EOLN, file);
	return finalize(1);
    }

    // let's create directories if these are not there yet
    char *dir = strdup(file);
    char *ptr = dir + strlen(file);
    while((ptr != dir) && (*ptr != '/')) ptr--;
    // there is a path in the filename...
    if(ptr != dir) {
	// just truncate it till filename
	*ptr = '\0';
	// ...and make sure the directories are exists
	if(!recursive_mkdir(dir, 0755)) {
	    syslog(LOG_ERR, "Cannot build path to \"%s\"." EOLN, file);
	    free(dir);		// just for a case if I move the code from main, to avoid memory leaks
	    return finalize(1);
	}
    }
    free(dir);

    if(!my_write_file(file, data)) {
	syslog(LOG_ERR, "Cannot write file \"%s\"." EOLN, file);
	return finalize(1);
    }

    return finalize(0);
}
