#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"

#include <iostream>
#include <cstring>

using namespace std;

int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  Disk disk_run;

  // StaticBuffer buffer;
  // OpenRelTable cache;

  //write
  unsigned char buffer[BLOCK_SIZE];
  Disk::readBlock(buffer,7000);
  char message[] = "hello";
  memcpy(buffer+20, message, 6);
  Disk::writeBlock(buffer, 7000);

  //read
  unsigned char buffer2[BLOCK_SIZE];
  char message2[6];
  Disk::readBlock(buffer2, 7000);
  memcpy(message2, buffer2+20, 6);
  cout << message2 << '\n';

  //read the first block

  unsigned char buffer3[BLOCK_SIZE];
  Disk::readBlock(buffer3, 0);
  for(int i=0; i<6; i++){
    cout << (int)buffer3[i] << " ";
  }
  cout << "\n";

  return 0;

  //return FrontendInterface::handleFrontend(argc, argv);
}