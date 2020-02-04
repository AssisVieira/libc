#include "assets.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "hashTable/hashTable.h"
#include "log/log.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct File {
  char *buff;
  size_t size;
  char path[PATH_MAX];
} File;

////////////////////////////////////////////////////////////////////////////////

typedef struct Assets {
  char dir[PATH_MAX];
  HashTable files;
} Assets;

////////////////////////////////////////////////////////////////////////////////

static Assets assets = {
    .dir = {0},
};

////////////////////////////////////////////////////////////////////////////////

static int assets_addFile(const char *path);
static int assets_addDir(const char *dirPath);
static bool assets_isFile(const char *path);
static bool assets_isDir(const char *path);
static char *assets_makePath(char *path, size_t pathSize, const char *dirPath,
                             const char *filename);

////////////////////////////////////////////////////////////////////////////////

int assets_open(const char *dirPath) {
  strncat(assets.dir, dirPath, sizeof(assets.dir));

  if (hashTable_init(&assets.files, 100)) {
    log_erro("assets", "hashTable_init(): %d - %s\n", errno, strerror(errno));
    goto error;
  }

  if (assets_addDir(dirPath)) {
    log_erro("assets", "assets_addDir().\n");
    goto error;
  }

  return 0;

error:
  assets_close();
  return -1;
}

////////////////////////////////////////////////////////////////////////////////

const char *assets_get(const char *path) {
  File *file = hashTable_value(&assets.files, path);
  if (file == NULL) {
    return NULL;
  }
  return file->buff;
}

////////////////////////////////////////////////////////////////////////////////

size_t assets_size(const char *path) {
  File *file = hashTable_value(&assets.files, path);
  if (file == NULL) {
    return -1;
  }
  return file->size;
}

////////////////////////////////////////////////////////////////////////////////

void assets_close() {
  HashTableIt it;

  hashTable_it(&assets.files, &it);
  while (hashTable_itNext(&it)) {
    File *file = hashTable_itValue(&it);
    if (file != NULL) {
      free(file->buff);
      free(file);
    }
  }

  hashTable_free(&assets.files);
}

////////////////////////////////////////////////////////////////////////////////

static int assets_addFile(const char *path) {
  File *file = malloc(sizeof(File));
  int fd = -1;

  log_dbug("assets", "Adding: %s\n", path);

  if (file == NULL) {
    goto error;
  }

  file->path[0] = '\0';
  file->buff = NULL;
  file->size = 0;

  strncat(file->path, path, PATH_MAX);

  fd = open(file->path, O_RDONLY);

  if (fd < 0) {
    goto error;
  }

  off_t buffSize = lseek(fd, 0, SEEK_END);

  if (buffSize == -1) {
    log_erro("assets", "lseek(): %d - %s\n", errno, strerror(errno));
    goto error;
  }

  if (lseek(fd, 0, SEEK_SET)) {
    log_erro("assets", "lseek(): %d - %s\n", errno, strerror(errno));
    goto error;
  }

  file->buff = malloc(buffSize);

  if (file->buff == NULL) {
    log_erro("assets", "malloc(): %d - %s\n", errno, strerror(errno));
    goto error;
  }

  size_t nread;
  while ((nread = read(fd, file->buff + file->size, buffSize - file->size))) {
    if (nread > 0) {
      file->size += nread;
      continue;
    }

    if (nread == EINTR) continue;

    log_erro("assets", "read(): %d - %s\n", errno, strerror(errno));
    goto error;
  }

  if (hashTable_set(&assets.files, path, file)) {
    log_erro("assets", "hashTable_set(): %d - %s\n", errno, strerror(errno));
    goto error;
  }

  close(fd);

  return 0;

error:
  if (file != NULL) {
    free(file->buff);
    free(file);
  }
  if (fd >= 0) {
    close(fd);
  }
  return -1;
}

static int assets_addDir(const char *dirPath) {
  DIR *dr = NULL;
  struct dirent *de = NULL;
  char filePath[PATH_MAX] = {0};

  dr = opendir(dirPath);

  if (dr == NULL) {
    log_erro("assets", "opendir(): %d - %s\n", errno, strerror(errno));
    goto error;
  }

  while ((de = readdir(dr)) != NULL) {
    if (assets_makePath(filePath, PATH_MAX, dirPath, de->d_name) == NULL) {
      log_erro("assets", "File path too long. Dir: %s; file: %s\n", dirPath,
               de->d_name);
      continue;
    }

    if (de->d_type == DT_REG ||
        (de->d_type == DT_UNKNOWN && assets_isFile(filePath))) {
      if (assets_addFile(filePath)) {
        log_erro("assets", "assets_addFile(): %d - %s\n", errno,
                 strerror(errno));
      }

      continue;
    }

    if (de->d_type == DT_DIR ||
        (de->d_type == DT_UNKNOWN && assets_isDir(filePath))) {
      if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
        continue;
      }

      if (assets_addDir(filePath)) {
        log_erro("assets", "assets_addDir(): %d - %s\n", errno,
                 strerror(errno));
      }

      continue;
    }
  }

  closedir(dr);
  return 0;

error:
  closedir(dr);
  return -1;
}

static bool assets_isFile(const char *path) {
  struct stat st_buf;
  int status = stat(path, &st_buf);

  if (status != 0) {
    log_erro("assets", "stat(): %d - %s\n", errno, strerror(errno));
    return false;
  }

  if (S_ISREG(st_buf.st_mode)) return true;

  return false;
}

static bool assets_isDir(const char *path) {
  struct stat st_buf;
  int status = stat(path, &st_buf);

  if (status != 0) {
    log_erro("assets", "stat(): %d - %s\n", errno, strerror(errno));
    return false;
  }

  if (S_ISDIR(st_buf.st_mode)) return true;

  return false;
}

static char *assets_makePath(char *path, size_t pathSize, const char *dirPath,
                             const char *filename) {
  size_t pathLen;
  size_t dirPathLen;
  size_t filenameLen;
  bool endsWithDivisor;

  if (path == NULL || pathSize <= 0) {
    return NULL;
  }

  dirPathLen = strlen(dirPath);
  filenameLen = strlen(filename);
  endsWithDivisor = (dirPath[dirPathLen - 1] == '/') ? true : false;

  pathLen = dirPathLen;
  pathLen += (endsWithDivisor) ? 0 : 1;  // divisor /
  pathLen += filenameLen;
  pathLen += 1;  // null byte

  if (pathLen > pathSize) {
    path[0] = '\0';
  }

  strcpy(path, dirPath);

  if (!endsWithDivisor) strcat(path, "/");

  strcat(path, filename);

  return path;
}