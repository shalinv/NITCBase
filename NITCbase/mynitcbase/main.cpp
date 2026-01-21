#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[]) {
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;

  for(int i=0; i<=1; i++){
    RelCatEntry RelCatBuffer;
    RelCacheTable::getRelCatEntry(i, &RelCatBuffer);
    printf("Relation: %s\n", RelCatBuffer.relName);

    for(int j=0 ; j < RelCatBuffer.numAttrs; j++){
      AttrCatEntry AttrCatBuffer;
      AttrCacheTable::getAttrCatEntry(i, j, &AttrCatBuffer);
      printf(" %s: %s\n", AttrCatBuffer.attrName, (AttrCatBuffer.attrType) ? "STR" : "NUM");
    }
    printf("\n");
  }

  return 0;
}