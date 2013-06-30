#include <stdio.h>

#include "std/memory.h"
#include "system/fileio.h"

#include "json.h"

static void Usage(const char *program) {
  fprintf(stderr, "Usage: %s file.json [path1 path2 ...]\n", program);
  exit(1);
}

int main(int argc, char **argv) {
  char *json;
  int i;

  if (argc < 2)
    Usage(argv[0]);

  if ((json = ReadTextSimple(argv[1]))) {
    JsonNodeT *node = JsonParse(json);

    MemUnref(json);

    if (argc == 2) {
      JsonPrint(node, 0);
    } else {
      for (i = 2; i < argc; i++) {
        JsonNodeT *child = JsonQuery(node, argv[i]);
        printf("%d: result for '%s':\n", i - 1, argv[i]);
        if (child) {
          JsonPrint(child, 2);
          putchar('\n');
        } else {
          puts("(not found)");
        }
      }
    }

    FreeJsonNode(node);
  }

  return 0;
}
