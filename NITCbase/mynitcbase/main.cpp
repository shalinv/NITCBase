#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"

#include <iostream>
#include <cstring>

using namespace std;

int main(int argc, char *argv[]) {
  Disk disk_run;

  // create objects for the relation catalog and attribute catalog
  RecBuffer relCatBuffer(RELCAT_BLOCK);

  HeadInfo relCatHeader;

  // load the headers of both the blocks into relCatHeader and attrCatHeader.
  // (we will implement these functions later)
  relCatBuffer.getHeader(&relCatHeader);

  int totalrel = relCatHeader.numEntries;

  for (int i=0; i<totalrel ; i++) {

    Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    int attrBlock = ATTRCAT_BLOCK;

    while(attrBlock != -1){

      RecBuffer attrcatBuffer(attrBlock);
      HeadInfo attrCatHeader;
      attrcatBuffer.getHeader(&attrCatHeader);


      int totalattr = attrCatHeader.numEntries;
      for (int j=0; j<totalattr ; j++) {

        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrcatBuffer.getRecord(attrCatRecord, j);
        // declare attrCatRecord and load the attribute catalog entry into it

        if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0) {
          const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
          printf("  %s: %s\n",  attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
        }
      }
      
      attrBlock = attrCatHeader.rblock;

    }
    printf("\n");
  }

  return 0;
}