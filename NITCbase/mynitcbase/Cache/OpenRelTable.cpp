#include "OpenRelTable.h"
#include <cstdlib>
#include <cstring>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable() {

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
    tableMetaInfo[i].free = true;
  }


  /************ Setting up Relation Cache entries ************/
  // (we need to populate relation cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Relation Cache Table****/
  RecBuffer relCatBlock(RELCAT_BLOCK);

  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

  RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[RELCAT_RELID] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

  /**** setting up Attribute Catalog relation in the Relation Cache Table ****/

  // set up the relation cache entry for the attribute catalog similarly
  // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT

  // set the value at RelCacheTable::relCache[ATTRCAT_RELID]

  Attribute attrRelRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(attrRelRecord,RELCAT_SLOTNUM_FOR_ATTRCAT);

  RelCacheEntry relCacheAttrEntry;
  RelCacheTable::recordToRelCatEntry(attrRelRecord, &relCacheAttrEntry.relCatEntry);
  relCacheAttrEntry.recId.block = RELCAT_BLOCK;
  relCacheAttrEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

  RelCacheTable::relCache[ATTRCAT_RELID] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheAttrEntry;


  /************ Setting up Attribute cache entries ************/
  // (we need to populate attribute cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
  RecBuffer attrCatBlock(ATTRCAT_BLOCK);

  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

  // iterate through all the attributes of the relation catalog and create a linked
  // list of AttrCacheEntry (slots 0 to 5)
  // for each of the entries, set
  //    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
  //    attrCacheEntry.recId.slot = i   (0 to 5)
  //    and attrCacheEntry.next appropriately
  // NOTE: allocate each entry dynamically using malloc

  // set the next field in the last entry to nullptr
  AttrCacheEntry *relHead = nullptr;
  AttrCacheEntry *relCurr = nullptr;

  for(int slot = 0; slot <= 5; slot++){
    attrCatBlock.getRecord(attrCatRecord, slot);
    AttrCacheEntry *node = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &node->attrCatEntry);
    node->recId.slot = slot;
    node->recId.block = ATTRCAT_BLOCK;
    node->next = nullptr;

    if(relHead == nullptr){
      relHead = relCurr = node;
    }else{
      relCurr->next = node;
      relCurr = node;
    }
  }

  AttrCacheTable::attrCache[RELCAT_RELID] = relHead;

  /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/

  // set up the attributes of the attribute cache similarly.
  // read slots 6-11 from attrCatBlock and initialise recId appropriately

  // set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]

  AttrCacheEntry *attrHead = nullptr;
  AttrCacheEntry *attrCurr = nullptr;

  for(int slot = 6; slot <= 11; slot++){
    attrCatBlock.getRecord(attrCatRecord, slot);

    AttrCacheEntry *node = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &node->attrCatEntry);

    node->recId.block = ATTRCAT_BLOCK;
    node->recId.slot = slot;
    node->next = nullptr;

    if(attrHead == nullptr){
      attrHead = attrCurr = node;
    }else{
      attrCurr->next = node;
      attrCurr = node;
    }
  }

  AttrCacheTable::attrCache[ATTRCAT_RELID] = attrHead;


  tableMetaInfo[RELCAT_RELID].free = false;
  strcpy(tableMetaInfo[RELCAT_RELID].relName , RELCAT_RELNAME);

  tableMetaInfo[ATTRCAT_RELID].free = false;
  strcpy(tableMetaInfo[ATTRCAT_RELID].relName , ATTRCAT_RELNAME);
  
}

int OpenRelTable::getFreeOpenRelTableEntry() {

  /* traverse through the tableMetaInfo array,
    find a free entry in the Open Relation Table.*/
  for(int i=0; i<MAX_OPEN ; i++){
    if(tableMetaInfo[i].free == true){
      return i;
    }
  }

  // if found return the relation id, else return E_CACHEFULL.
  return E_CACHEFULL;
}


OpenRelTable::~OpenRelTable() {

  // close all open relations (from rel-id = 2 onwards. Why?)
  for (int i = 2; i < MAX_OPEN; ++i) {
    if (!tableMetaInfo[i].free) {
      OpenRelTable::closeRel(i); // we will implement this function later
    }
  }

  // free all the memory that you allocated in the constructor
  for(int i=0; i < 2; i++){
    if(RelCacheTable::relCache[i] != nullptr){
      free(RelCacheTable::relCache[i]);
      RelCacheTable::relCache[i] = nullptr;
    }
  }

  for(int i=0; i < 2; i++){
    AttrCacheEntry *curr = AttrCacheTable::attrCache[i];
    while(curr != nullptr){
      AttrCacheEntry *next = curr->next;
      free(curr);
      curr = next;
    }
    AttrCacheTable::attrCache[i] = nullptr;
  }
}

/* This function will open a relation having name `relName`.
Since we are currently only working with the relation and attribute catalog, we
will just hardcode it. In subsequent stages, we will loop through all the relations
and open the appropriate one.
*/
int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

  if(strcmp(RELCAT_RELNAME, relName) == 0) return RELCAT_RELID;
  if(strcmp(ATTRCAT_RELNAME, relName) == 0) return ATTRCAT_RELID;
  if(strcmp("Students", relName) == 0) return 2;
  else return E_RELNOTOPEN;
}