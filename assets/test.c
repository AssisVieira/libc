#include <assert.h>
#include <stdio.h>

#include "assets.h"

int main() {
  assert(assets_open("assets/example/") == 0);

  assert(assets_get("assets/example/index.html") != NULL);
  assert(assets_get("assets/example/css/style.css") != NULL);
  assert(assets_get("assets/example/imgs/image-1.jpg") != NULL);
  assert(assets_get("assets/example/imgs/image-2.jpg") != NULL);

  assert(assets_size("assets/example/index.html") == 192);
  assert(assets_size("assets/example/css/style.css") == 84);
  assert(assets_size("assets/example/imgs/image-1.jpg") == 101267);
  assert(assets_size("assets/example/imgs/image-2.jpg") == 50582);

  assert(assets_exists("assets/example/index.html") == true);

  assert(assets_isDir("assets/example/index.html") == false);
  assert(assets_isDir("assets/example/") == true);

  assets_close();

  return 0;
}
