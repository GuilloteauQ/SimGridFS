#include <cstdio>
#include <simgrid/s4u/Mailbox.hpp>
extern "C" {
#define FUSE_USE_VERSION 311
#include <fuse.h>
}

#include <chrono>
#include <sys/types.h>
#include <thread>
#include <time.h>
#include <unistd.h>

#include <cstring>
#include <functional>
#include <iostream>

#include "Fuse-impl.h"

#include <simgrid/plugins/file_system.h>
#include <simgrid/s4u.hpp>

#define MASTER_NAME "master"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "a sample log category");
namespace sg4 = simgrid::s4u;

class MasterFS : public Fusepp::Fuse<MasterFS> {
public:
  MasterFS(std::string master_name) { master_name = master_name; }

  static int getattr(const char *path, struct stat *st,
                     struct fuse_file_info *fi) {

    std::cout << "[getattr] Called" << std::endl;
    std::cout << "\tAttributes of " << path << " requested" << std::endl;
    st->st_uid = getuid(); // The owner of the file/directory is the user who
    st->st_gid = getgid(); // The group of the file/directory is the same as the
    st->st_atime = time(NULL);
    st->st_mtime = time(NULL);
    std::string filename = path;
    sg_file_t file =
        (fi == NULL) ? sg4::File::open(filename, nullptr) : (sg_file_t)(fi->fh);
    const sg_size_t file_size = file->size();

    if (strcmp(path, "/") == 0) {
      st->st_mode = S_IFDIR | 0755;
      st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is
                        // here: http://unix.stackexchange.com/a/101536
    } else {
      st->st_mode = S_IFREG | 0644;
      st->st_nlink = 1;
      st->st_size = file_size;
    }

    return 0;
  }

  static int readdir(const char *path, void *buffer, fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi,
                     enum fuse_readdir_flags flags) {

    sg4::Mailbox *master_mailbox = sg4::Mailbox::by_name("master");
    sg4::Mailbox *my_mailbox = sg4::Mailbox::by_name("masterfs");
    std::string msg = std::string(path);

    master_mailbox->put(new double(0), 0);

    auto *msg_recv = my_mailbox->get<std::set<std::string>>();
    std::set<std::string> filenames = *msg_recv;

    printf("--> Getting The List of Files of %s\n", path);

    filler(buffer, ".", NULL, 0, FUSE_FILL_DIR_PLUS);  // Current Directory
    filler(buffer, "..", NULL, 0, FUSE_FILL_DIR_PLUS); // Parent Directory

    if (strcmp(path, "/") == 0) {
      for (std::string filename : filenames) {
        XBT_INFO("Showing file %s", filename.c_str());
        filler(buffer, filename.c_str(), NULL, 0, FUSE_FILL_DIR_PLUS);
      }
    }

    return 0;
  }

  static int unlink(const char *path) {
    std::string filename = path;
    sg_file_t file = sg4::File::open(filename, nullptr);
    file->unlink();
    return 0;
  }

  static int write(const char *path, const char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi) {
    std::string filename = path;

    int fd;
    int res;
    (void)fi;
    if (fi != NULL) {
      XBT_INFO("Using open file");
    }
    sg4::Engine *engine = sg4::Engine::get_instance();
    double time_before = engine->get_clock();
    XBT_INFO("it's currently %f", engine->get_clock());
    sg_file_t file =
        (fi == NULL) ? sg4::File::open(filename, nullptr) : (sg_file_t)(fi->fh);
    file->seek(offset);
    if (fi == NULL) {
      XBT_INFO("Create a %lu bytes file named '%s' on / (offset %lu)", size,
               filename.c_str(), offset);
    } else {
      XBT_INFO("Adding a %lu bytes file named '%s' on / (offset %lu)", size,
               filename.c_str(), offset);
    }
    XBT_INFO("File %s of size %llu", filename.c_str(), file->size());
    sg_size_t write = file->write(size);
    XBT_INFO("File %s of size %llu", filename.c_str(), file->size());
    if (fi == NULL) {
      file->close();
    }
    double time_after = engine->get_clock();
    XBT_INFO("write took %f (%f -> %f)", time_after - time_before, time_before,
             time_after);
    long int sleep_duration_us =
        (long int)(1000000 * (time_after - time_before));
    std::this_thread::sleep_for(std::chrono::microseconds(sleep_duration_us));
    return write;
  }

  static int open(const char *path, struct fuse_file_info *fi) {
    std::string filename = path;
    sg4::Mailbox *master_mailbox = sg4::Mailbox::by_name("master");
    sg4::Mailbox *my_mailbox = sg4::Mailbox::by_name("masterfs");
    master_mailbox->put(new double(1), 0);
    master_mailbox->put(&filename, 0);
    XBT_INFO("Openning '%s' on /", filename.c_str());
    sg_file_t res = sg4::File::open(filename, nullptr);
    fi->fh = (uint64_t)(res);
    return 0;
  }

  static off_t lseek(const char *path, off_t off, int whence,
                     struct fuse_file_info *fi) {
    XBT_INFO("LSEEK");
    std::string filename = path;
    (void)fi;
    if (fi != NULL) {
      XBT_INFO("Using open file");
    }
    sg_file_t file =
        (fi == NULL) ? sg4::File::open(filename, nullptr) : (sg_file_t)(fi->fh);

    file->seek(off, whence);
    if (fi == NULL)
      file->close();
    return off;
  }

  static int read(const char *path, char *buffer, size_t size, off_t offset,
                  struct fuse_file_info *fi) {
    std::string filename = path;
    (void)fi;
    sg4::Engine *engine = sg4::Engine::get_instance();
    double time_before = engine->get_clock();
    sg_file_t file =
        (fi == NULL) ? sg4::File::open(filename, nullptr) : (sg_file_t)(fi->fh);

    file->seek(offset);
    const sg_size_t read = file->read(size);
    XBT_INFO("Read %llu bytes on %s (offset: %lu)", read, filename.c_str(),
             offset);
    if (fi == NULL) {
      file->close();
    }
    double time_after = engine->get_clock();
    XBT_INFO("read took %f (%f -> %f)", time_after - time_before, time_before,
             time_after);
    long int sleep_duration_us =
        (long int)(1000000 * (time_after - time_before));
    std::this_thread::sleep_for(std::chrono::microseconds(sleep_duration_us));

    return (int)(read);
  }
};

void master(std::vector<std::string> worker_names) {
  std::vector<sg4::Mailbox *> workers;
  for (std::string worker_name : worker_names) {
    workers.push_back(sg4::Mailbox::by_name(worker_name));
  }

  std::set<std::string> filenames;

  sg4::Mailbox *my_mailbox = sg4::Mailbox::by_name("master");
  sg4::Mailbox *masterfs_mailbox = sg4::Mailbox::by_name("masterfs");
  do {

    auto *msg = my_mailbox->get<double>();
    if (*msg == 0) {
      XBT_INFO("Must send all the filenames");
      masterfs_mailbox->put(&filenames, 0);
    } else if (*msg == 1) {
      XBT_INFO("Must add a new filename to the list");
      auto *msg_f = my_mailbox->get<std::string>();
      std::string new_filename = *msg_f;
      std::string trimmed_new_filename =
          new_filename.substr(1, new_filename.length() - 1);
      XBT_INFO("New file alert! %s", trimmed_new_filename.c_str());
      filenames.insert(trimmed_new_filename);
    } else {
      XBT_INFO("weird value !");
    }
  } while (1);
}

void masterfs(std::string master_name, int argc, char *argv[]) {
  struct fuse_args fuseargs = FUSE_ARGS_INIT(1, argv);
  struct fuse *fuse;
  MasterFS fs(master_name);
  auto operations = fs.Operations();
  fuse = fuse_new(&fuseargs, operations, sizeof(*operations), &fs);
  char* mount_point = argv[argc - 1];
  if (fuse_mount(fuse,  mount_point) != 0) {
    fprintf(stderr, "Could not mount\n");
    exit(1);
  }
  if (fuse_daemonize(1) != 0) {
    fprintf(stderr, "Could not daemonzie\n");
    exit(1);
  }
  struct fuse_session *se = fuse_get_session(fuse);
  fuse_loop(fuse);
}

void worker() {
  const sg4::Host *my_host = sg4::this_actor::get_host();
  sg4::Mailbox *mailbox = sg4::Mailbox::by_name(my_host->get_name());

  double compute_cost;
  do {
    auto msg = mailbox->get_unique<double>();
    compute_cost = *msg;

    if (compute_cost >
        0) /* If compute_cost is valid, execute a computation of that cost */
      sg4::this_actor::execute(compute_cost);
  } while (compute_cost > 0); /* Stop when receiving an invalid compute_cost */

  XBT_INFO("Exiting now.");
}

int find_argv_sep(int argc, char *argv[]) {
  int i = 0;
  for (i = 0; i < argc; i++) {
    if (strcmp(argv[i], "--") == 0) {
      return i;
    }
  }
  return i;
}

int main(int argc, char *argv[]) {
  umask(0);

  int sep_position = find_argv_sep(argc, argv);

  int nb_args_simgrid = sep_position;
  int nb_args_fuse = argc - sep_position;
  argv[sep_position] = argv[0];
  assert(nb_args_simgrid >= 2);

  sg4::Engine e(&nb_args_simgrid, argv);
  sg_storage_file_system_init();
  e.load_platform(argv[1]);

  int nb_workers = 1;

  std::string worker_name;
  std::vector<std::string> worker_names;
  for (int i = 1; i <= nb_workers; i++) {
    worker_name = "worker" + std::to_string(i);
    worker_names.push_back(worker_name);
    sg4::Actor::create(worker_name, e.host_by_name("bob"), worker);
  }
  // the dynamic part
  sg4::Actor::create("master", e.host_by_name("bob"), master, worker_names);
  // the fuse part that sends to the dynamic part
  sg4::Actor::create("masterfs", e.host_by_name("bob"), masterfs, "master",
                     nb_args_fuse, argv + nb_args_fuse);
  e.run();
  return 0;
}
