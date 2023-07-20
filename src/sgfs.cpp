extern "C" {
#define FUSE_USE_VERSION 311
#include <fuse_lowlevel.h>
}

#include <iostream>


int main() {
  std::cout << "ok\n";
  return 0;
}
