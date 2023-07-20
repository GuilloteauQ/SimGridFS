extern "C" {
#define FUSE_USE_VERSION 311
// #include <fuse_lowlevel.h>
#include <fuse.h>
}

#include <sys/types.h>
#include <time.h>
#include <unistd.h>
// #include <stdlib.h>

#include <cstring>
#include <functional>
#include <iostream>

#include "Fuse-impl.h"

#include <simgrid/plugins/file_system.h>
#include <simgrid/s4u.hpp>
XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");
namespace sg4 = simgrid::s4u;

static int do_getattr(const char *path, struct stat *st,
                      struct fuse_file_info *fi) {
  std::cout << "[getattr] Called" << std::endl;
  std::cout << "\tAttributes of " << path << " requested" << std::endl;
  st->st_uid = getuid(); // The owner of the file/directory is the user who
  st->st_gid = getgid(); // The group of the file/directory is the same as the
  st->st_atime = time(NULL);
  st->st_mtime = time(NULL);

  if (strcmp(path, "/") == 0) {
    st->st_mode = S_IFDIR | 0755;
    st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is
                      // here: http://unix.stackexchange.com/a/101536
  } else {
    st->st_mode = S_IFREG | 0644;
    st->st_nlink = 1;
    st->st_size = 1024;
  }

  return 0;
}

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi,
                      enum fuse_readdir_flags flags) {

  printf("--> Getting The List of Files of %s\n", path);

  filler(buffer, ".", NULL, 0, FUSE_FILL_DIR_PLUS);  // Current Directory
  filler(buffer, "..", NULL, 0, FUSE_FILL_DIR_PLUS); // Parent Directory

  if (strcmp(path, "/") == 0) {
    filler(buffer, "file54", NULL, 0, FUSE_FILL_DIR_PLUS);
    filler(buffer, "file349", NULL, 0, FUSE_FILL_DIR_PLUS);
  }

  return 0;
}

static int do_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
  int res = 0;
  printf("Create a new file!\n");
  if (res == -1)
    return -errno;

  fi->fh = res;
  return 0;
}

static int do_open(const char *path, struct fuse_file_info *fi) {
  int res = 0;

  printf("Openning a new file!\n");
  // res = open(path, fi->flags);
  if (res == -1)
    return -errno;

  fi->fh = res;
  return 0;
}

static int do_read(const char *path, char *buffer, size_t size, off_t offset,
                   struct fuse_file_info *fi) {
  char file54Text[] = "Hello World From File54!";
  char file349Text[] = "Hello World From File349!";
  char *selectedText = NULL;

  if (strcmp(path, "/file54") == 0) {
    selectedText = file54Text;
  } else if (strcmp(path, "/file349") == 0) {
    selectedText = file349Text;
  } else {
    return -1;
  }
  memcpy(buffer, selectedText + offset, size);

  return strlen(selectedText) - offset;
}

class MyHost : public Fusepp::Fuse<MyHost> {
public:
  void show_info(std::vector<sg4::Disk *> const &disks) const {
    XBT_INFO("Storage info on %s:", sg4::Host::current()->get_cname());

    for (auto const &d : disks) {
      // Retrieve disk's information
      XBT_INFO("    %s (%s) Used: %llu; Free: %llu; Total: %llu.",
               d->get_cname(), sg_disk_get_mount_point(d),
               sg_disk_get_size_used(d), sg_disk_get_size_free(d),
               sg_disk_get_size(d));
    }
  }
  static int getattr(const char *path, struct stat *st,
                        struct fuse_file_info *fi) {
    std::cout << "[getattr] Called" << std::endl;
    std::cout << "\tAttributes of " << path << " requested" << std::endl;
    st->st_uid = getuid(); // The owner of the file/directory is the user who
    st->st_gid = getgid(); // The group of the file/directory is the same as the
    st->st_atime = time(NULL);
    st->st_mtime = time(NULL);

    if (strcmp(path, "/") == 0) {
      st->st_mode = S_IFDIR | 0755;
      st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is
                        // here: http://unix.stackexchange.com/a/101536
    } else {
      st->st_mode = S_IFREG | 0644;
      st->st_nlink = 1;
      st->st_size = 1024;
    }

    return 0;
  }

  static int readdir(const char *path, void *buffer, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi,
                        enum fuse_readdir_flags flags) {

    printf("--> Getting The List of Files of %s\n", path);

    filler(buffer, ".", NULL, 0, FUSE_FILL_DIR_PLUS);  // Current Directory
    filler(buffer, "..", NULL, 0, FUSE_FILL_DIR_PLUS); // Parent Directory

    if (strcmp(path, "/") == 0) {
      filler(buffer, "file54", NULL, 0, FUSE_FILL_DIR_PLUS);
      filler(buffer, "file349", NULL, 0, FUSE_FILL_DIR_PLUS);
    }

    return 0;
  }

  static int read(const char *path, char *buffer, size_t size, off_t offset,
                     struct fuse_file_info *fi) {
    char file54Text[] = "Hello World From File54!";
    char file349Text[] = "Hello World From File349!";
    char *selectedText = NULL;

    if (strcmp(path, "/file54") == 0) {
      selectedText = file54Text;
    } else if (strcmp(path, "/file349") == 0) {
      selectedText = file349Text;
    } else {
      return -1;
    }
    memcpy(buffer, selectedText + offset, size);

    return strlen(selectedText) - offset;
  }
};

static void master() {
  char *argv[] = {"plop"};
  struct fuse_args fuseargs = FUSE_ARGS_INIT(1, argv);
  // fuse_main(3, argv, &operations, NULL);
  struct fuse *fuse;

  MyHost fs;

  auto operations = fs.Operations();

  fuse = fuse_new(&fuseargs, operations, sizeof(*operations), &fs);
  if (fuse_mount(fuse, "ici") != 0) {
    fprintf(stderr, "Could not mount\n");
  }

  if (fuse_daemonize(1) != 0) {
    fprintf(stderr, "Could not daemonzie\n");
  }
  struct fuse_session *se = fuse_get_session(fuse);
  fuse_loop(fuse);

  fuse_unmount(fuse);
}

int main(int argc, char *argv[]) {
  umask(0);

  // TODO: have a seperator '--' to pass args to simgrid and to fuse
  sg4::Engine e(&argc, argv);
  sg_storage_file_system_init();
  e.load_platform(argv[1]);
  // sg4::Actor::create("host", e.host_by_name("bob"), MyHost());
  sg4::Actor::create("host", e.host_by_name("bob"), master);
  e.run();

  // return fuse_main(argc, argv, &operations, NULL);
}
