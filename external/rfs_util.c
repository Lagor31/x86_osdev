#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../rfs/rfs.h"

#define NUMFILES 5
int main(int argc, char** args) {
  if (argc <= 1) {
    printf("Invalid number of arguments\n");
    return -1;
  }
  char* filename = args[1];
  size_t reqSize = atoi(args[2]);

  FILE* outFile = fopen(filename, "w+");

  char* baseContent = "ConenutoFile";

  printf("Creating disk file %s of size %lu\n", filename, reqSize);

  struct rfsHeader* header =
      (struct rfsHeader*)malloc(sizeof(struct rfsHeader));

  header->magic = RFS_HEADER_MAGIC;

  struct fileTableEntry fileTableEntries[NUMFILES];
  char* filesContent[NUMFILES];

  size_t fileSize = 0;
  for (uint32_t i = 0; i < NUMFILES; ++i) {
    struct fileTableEntry* file =
        (struct fileTableEntry*)malloc(sizeof(struct fileTableEntry));
    file->id = i;
    file->offset = 0xFFFFFFFF;

    filesContent[i] = (char*)malloc(strlen(baseContent));
    filesContent[i] = baseContent;
    file->length = strlen(baseContent);
    fileTableEntries[i] = *file;
  }

  uint32_t baseOffset = sizeof(struct rfsHeader) + sizeof(fileTableEntries);

  for (uint8_t i; i < NUMFILES; ++i) {
    fileTableEntries[i].offset = baseOffset;
    baseOffset += fileTableEntries[i].length;
  }

  fileSize = sizeof(struct rfsHeader) + sizeof(fileTableEntries) +
             strlen(baseContent) * NUMFILES;
  header->size = fileSize;
  header->numberOfFile = NUMFILES;
  fwrite(header, 1, sizeof(struct rfsHeader), outFile);
  fwrite(fileTableEntries, 1, sizeof(fileTableEntries), outFile);

  for (size_t i = 0; i < NUMFILES; ++i) {
    fwrite(filesContent[i], 1, strlen(filesContent[i]), outFile);
  }

  if (fileSize < reqSize) {
    uint8_t* fill = calloc(reqSize - fileSize, sizeof(uint8_t));
    fwrite(fill, sizeof(uint8_t), reqSize - fileSize, outFile);
  }

  fclose(outFile);
  return 0;
}
