#ifndef RFS_H
#define RFS_H

#define RFS_HEADER_MAGIC 0X4C524653  // LRFS

struct rfs {
  struct rfsHader* info;
  uint32_t hardDiskSize;
  uint32_t filesSize;
  struct rfsFile* currentFile;
  struct rfsFile* root;
};

struct rfsHeader {
  uint32_t magic;
  uint32_t size;
  uint32_t numberOfFile;
};

struct fileTableEntry {
  uint32_t id;
  uint32_t offset;
  uint32_t length;
};

struct rfsFile {
  char* filename;
  uint32_t length;
  uint32_t attributes;
  uint8_t* data;
  uint8_t childrenNumber;
  struct rfsFile* children;
};

void allocRFS(struct multiboot_tag_module*);
void parseRFSData(uint8_t*);
void printRFSInfo();
void loadUserProcess(struct multiboot_tag_module*);

#endif