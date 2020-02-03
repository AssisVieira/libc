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

static int assets_add(const char *path);

////////////////////////////////////////////////////////////////////////////////

int assets_open(const char *path) {
  DIR *dr = NULL;
  struct dirent *de = NULL;

  strncat(assets.dir, path, sizeof(assets.dir));

  if (hashTable_init(&assets.files, 100)) {
    log_erro("assets", "hashTable_init(): %d - %s\n", errno, strerror(errno));
    goto error;
  }

  dr = opendir(path);

  if (dr == NULL) {
    log_erro("assets", "opendir(): %d - %s\n", errno, strerror(errno));
    goto error;
  }

  while ((de = readdir(dr)) != NULL) {
    if (assets_add(de->d_name)) {
      log_erro("assets", "assets_add(): %d - %s\n", errno, strerror(errno));
      goto error;
    }
  }

  closedir(dr);

  return 0;

error:
  if (dr != NULL) {
    closedir(dr);
  }
  assets_close();
  return -1;
}

////////////////////////////////////////////////////////////////////////////////

const char *assets_get(const char *path) {
  return hashTable_value(&assets.files, path);
}

////////////////////////////////////////////////////////////////////////////////

void assets_close() { hashTable_free(&assets.files); }

////////////////////////////////////////////////////////////////////////////////

static int assets_add(const char *path) {
  File *file = malloc(sizeof(File));
  int fd = -1;

  if (file == NULL) {
    goto error;
  }

  file->path[0] = '\0';
  file->buff = NULL;
  file->size = 0;

  if (strlen(assets.dir) + strlen(path) > PATH_MAX) {
    log_erro("assets", "File path too long.\n");
    goto error;
  }

  strncat(file->path, assets.dir, PATH_MAX);
  strncat(file->path, path, PATH_MAX - strlen(file->path));

  fd = open(file->path, O_RDONLY);

  if (fd < 0) {
    goto error;
  }

  off_t fileSize = lseek(fd, 0, SEEK_END);

  if (fileSize == -1) {
    log_erro("assets", "lseek(): %d - %s\n", errno, strerror(errno));
    goto error;
  }

  if (lseek(fd, 0, SEEK_SET)) {
    log_erro("assets", "lseek(): %d - %s\n", errno, strerror(errno));
    goto error;
  }

  file->buff = malloc(sizeof(fileSize));

  if (file->buff == NULL) {
    log_erro("assets", "malloc(): %d - %s\n", errno, strerror(errno));
    goto error;
  }

  size_t nread;
  while ((nread = read(fd, file->buff + file->size,
                       sizeof(file->buff) - file->size))) {
    if (nread > 0) {
      file->size += nread;
      continue;
    }

    if (nread == EINTR) continue;

    log_erro("assets", "read(): %d - %s\n", errno, strerror(errno));
    goto error;
  }

  if (hashTable_set(assets.files, path, file)) {
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
