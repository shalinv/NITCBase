#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) {
  Disk disk_run;
  StaticBuffer buffer;
  
  int attrBlockNum = ATTRCAT_BLOCK;
  while (attrBlockNum != -1) {
    RecBuffer attrCatBuffer(attrBlockNum);
    HeadInfo attrCatHeader;
    attrCatBuffer.getHeader(&attrCatHeader);

    for (int j = 0; j < attrCatHeader.numEntries; j++) {
      Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
      attrCatBuffer.getRecord(attrCatRecord, j);

      if (!strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, "Students") &&
          !strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Mark")) {

        strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Grade");

        unsigned char buffer[BLOCK_SIZE];
        Disk::readBlock(buffer, attrBlockNum);

        int recSize = attrCatHeader.numAttrs * ATTR_SIZE;
        int offset = HEADER_SIZE + SLOTMAP_SIZE_RELCAT_ATTRCAT + (recSize * j);

        memcpy(buffer + offset, attrCatRecord, recSize);
        Disk::writeBlock(buffer, attrBlockNum);
      }
    }
    attrBlockNum = attrCatHeader.rblock;
  }

  
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  HeadInfo relCatHeader;
  relCatBuffer.getHeader(&relCatHeader);

  for (int i = 0; i < relCatHeader.numEntries; i++) {
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    int attrBlockNum = ATTRCAT_BLOCK;
    while (attrBlockNum != -1) {
      RecBuffer attrCatBuffer(attrBlockNum);
      HeadInfo attrCatHeader;
      attrCatBuffer.getHeader(&attrCatHeader);

      for (int j = 0; j < attrCatHeader.numEntries; j++) {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, j);

        if (!strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal,
                     attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal)) {

          const char *attrType =
              attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";

          printf("  %s: %s\n",
                 attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,
                 attrType);
        }
      }
      attrBlockNum = attrCatHeader.rblock;
    }

    
    printf("\n");
  }

  return 0;
}