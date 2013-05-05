
#include <CoreFoundation/CFPlatform.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

static const char *__CFProcessPath = NULL;

const char *_CFProcessPath(void) {
  if (__CFProcessPath) return __CFProcessPath;
  __CFProcessPath = getenv("CFProcessPath");
  if (NULL == __CFProcessPath) {
    struct stat sb;
    if (lstat("/proc/self/exe", &sb) == -1) {
      return NULL;
    }

    char *path = malloc(sb.st_size + 1);
    if (NULL == path) {
      return NULL;
    }

    if (readlink("/proc/self/exe", path, sb.st_size + 1) < 0) {
      return NULL;
    }
    path[sb.st_size] = '\0';
    __CFProcessPath = path;
  }
  return __CFProcessPath;
}

