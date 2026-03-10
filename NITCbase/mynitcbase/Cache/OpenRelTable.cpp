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

OpenRelTable::~OpenRelTable() {

    for(int i=2; i < MAX_OPEN; i++)
    {
      if(!tableMetaInfo[i].free)
      {
        OpenRelTable::closeRel(i);
      }
    }

    /**** Closing the catalog relations in the relation cache ****/

    //releasing the relation cache entry of the attribute catalog

    if (RelCacheTable::relCache[ATTRCAT_RELID]->dirty == true){
      RelCatEntry relCatBuffer;
      RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatBuffer);
      Attribute relCatRecord[RELCAT_NO_ATTRS];
      RelCacheTable::relCatEntryToRecord(&relCatBuffer, relCatRecord);
      RecId recId = RelCacheTable::relCache[ATTRCAT_RELID]->recId;
        // declaring an object of RecBuffer class to write back to the buffer
      RecBuffer relCatBlock(recId.block);
        // Write back to the buffer using relCatBlock.setRecord() with recId.slot
      relCatBlock.setRecord(relCatRecord, recId.slot);
    }
    // free the memory dynamically allocated to this RelCacheEntry
    free(RelCacheTable::relCache[ATTRCAT_RELID]);

    //releasing the relation cache entry of the relation catalog

    if(RelCacheTable::relCache[RELCAT_RELID]->dirty == true) {

      /* Get the Relation Catalog entry from RelCacheTable::relCache
      Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
      RelCatEntry relCatBuffer;
      RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatBuffer);
      Attribute relCatRecord[RELCAT_NO_ATTRS];
      RelCacheTable::relCatEntryToRecord(&relCatBuffer, relCatRecord);
      RecId recId = RelCacheTable::relCache[RELCAT_RELID]->recId;
      // declaring an object of RecBuffer class to write back to the buffer
      RecBuffer relCatBlock(recId.block);

        // Write back to the buffer using relCatBlock.setRecord() with recId.slot
      relCatBlock.setRecord(relCatRecord, recId.slot);
    }
    // free the memory dynamically allocated for this RelCacheEntry
    free(RelCacheTable::relCache[RELCAT_RELID]);

    // free the memory allocated for the attribute cache entries of the
    // relation catalog and the attribute catalog
  for(int relID=RELCAT_RELID;relID<=ATTRCAT_RELID;relID++){
		AttrCacheEntry *curr=AttrCacheTable::attrCache[relID],*next=NULL;
		while(curr!=nullptr){
			next=curr->next;
			if(curr->dirty==true){
				AttrCatEntry attrCatEntry=curr->attrCatEntry;
				Attribute AttrCatrecord[ATTRCAT_NO_ATTRS];
				AttrCacheTable::attrCatEntryToRecord(&attrCatEntry,AttrCatrecord);
				RecBuffer attrCatBlock(curr->recId.block);
				attrCatBlock.setRecord(AttrCatrecord,curr->recId.slot);
			}
			free(curr);
			curr=next;
		}
	}
}

/* This function will open a relation having name `relName`.
Since we are currently only working with the relation and attribute catalog, we
will just hardcode it. In subsequent stages, we will loop through all the relations
and open the appropriate one.
*/

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

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

  /* traverse through the tableMetaInfo array,
    find the entry in the Open Relation Table corresponding to relName.*/
  for(int i=0; i < MAX_OPEN ; i++){
    if(!tableMetaInfo[i].free && strcmp(tableMetaInfo[i].relName , relName) == 0){
        return i;
    }
  }
  return E_RELNOTOPEN;
  // if found return the relation id, else indicate that the relation do not
  // have an entry in the Open Relation Table.
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]) {

  int relId = getRelId(relName);
  if(relId != E_RELNOTOPEN){
    return relId;
  }

  /* find a free slot in the Open Relation Table
     using OpenRelTable::getFreeOpenRelTableEntry(). */
  relId = getFreeOpenRelTableEntry();

  if (relId == E_CACHEFULL){
    return E_CACHEFULL;
  }

  /****** Setting up Relation Cache entry for the relation ******/

  /* search for the entry with relation name, relName, in the Relation Catalog using
      BlockAccess::linearSearch().
      Care should be taken to reset the searchIndex of the relation RELCAT_RELID
      before calling linearSearch().*/

  RelCacheTable::resetSearchIndex(RELCAT_RELID);
  union Attribute attrVal;
  strcpy(attrVal.sVal, relName);

  // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.
  RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID, (char *)"RelName", attrVal, EQ );

  if (relcatRecId.block == -1 && relcatRecId.slot == -1 ) {
    // (the relation is not found in the Relation Catalog.)
    return E_RELNOTEXIST;
  }

  /* read the record entry corresponding to relcatRecId and create a relCacheEntry
      on it using RecBuffer::getRecord() and RelCacheTable::recordToRelCatEntry().
      update the recId field of this Relation Cache entry to relcatRecId.
      use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
    NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
  */
  RecBuffer relcatBlock(relcatRecId.block);
  Attribute relcatRecord[RELCAT_NO_ATTRS];
  relcatBlock.getRecord(relcatRecord, relcatRecId.slot);

  RelCacheEntry *relCacheEntry = (RelCacheEntry *)malloc(sizeof(RelCacheEntry));

  RelCacheTable::recordToRelCatEntry(relcatRecord, &relCacheEntry->relCatEntry);
  relCacheEntry->recId = relcatRecId;
  relCacheEntry->searchIndex.block = -1;
  relCacheEntry->searchIndex.slot = -1;
  RelCacheTable::relCache[relId] = relCacheEntry;

  /****** Setting up Attribute Cache entry for the relation ******/

  // let listHead be used to hold the head of the linked list of attrCache entries.
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

  AttrCacheEntry *listHead = nullptr;
  AttrCacheEntry *listTail = nullptr;

  /*iterate over all the entries in the Attribute Catalog corresponding to each
  attribute of the relation relName by multiple calls of BlockAccess::linearSearch()
  care should be taken to reset the searchIndex of the relation, ATTRCAT_RELID,
  corresponding to Attribute Catalog before the first call to linearSearch().*/
  while(true){
      /* let attrcatRecId store a valid record id an entry of the relation, relName,
      in the Attribute Catalog.*/
      union Attribute attrVal;
      strcpy(attrVal.sVal, relName);

      RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, (char *)"RelName", attrVal, EQ);

      if (attrcatRecId.block == -1 && attrcatRecId.slot == -1) {
        break;
      }

      /* read the record entry corresponding to attrcatRecId and create an
      Attribute Cache entry on it using RecBuffer::getRecord() and
      AttrCacheTable::recordToAttrCatEntry().
      update the recId field of this Attribute Cache entry to attrcatRecId.
      add the Attribute Cache entry to the linked list of listHead .*/
      // NOTE: make sure to allocate memory for the AttrCacheEntry using malloc()
    RecBuffer attrcatBlock(attrcatRecId.block);
    Attribute attrcatRecord[ATTRCAT_NO_ATTRS];
    attrcatBlock.getRecord(attrcatRecord, attrcatRecId.slot);

    // create Attribute Cache entry
    AttrCacheEntry *attrCacheEntry = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));

    AttrCacheTable::recordToAttrCatEntry(attrcatRecord, &attrCacheEntry->attrCatEntry);
    attrCacheEntry->recId = attrcatRecId;
    attrCacheEntry->next = nullptr;

    if (listHead == nullptr) {
      listHead = listTail = attrCacheEntry;
    } else {
      listTail->next = attrCacheEntry;
      listTail = attrCacheEntry;
    }
  }

  // set the relIdth entry of the AttrCacheTable to listHead.
  AttrCacheTable::attrCache[relId] = listHead;

  /****** Setting up metadata in the Open Relation Table for the relation******/

  // update the relIdth entry of the tableMetaInfo with free as false and
  // relName as the input.
  tableMetaInfo[relId].free = false;
  strcpy(tableMetaInfo[relId].relName, relName);

  return relId;
}



int OpenRelTable::closeRel(int relId) {
  // confirm that rel-id fits the following conditions
  //     2 <=relId < MAX_OPEN
  //     does not correspond to a free slot
  if (relId == RELCAT_RELID || relId == ATTRCAT_RELID) {
    return E_NOTPERMITTED;
  }

  if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if (OpenRelTable::tableMetaInfo[relId].free == true) {
    return E_RELNOTOPEN;
  }


  /****** Releasing the Relation Cache entry of the relation ******/
  if (RelCacheTable::relCache[relId]->dirty==true)
  {
    /* Get the Relation Catalog entry from RelCacheTable::relCache
    Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&RelCacheTable::relCache[relId]->relCatEntry, relCatRecord);
    // declaring an object of RecBuffer class to write back to the buffer
    RecBuffer relCatBlock(RelCacheTable::relCache[relId]->recId.block);

    relCatBlock.setRecord(relCatRecord,RelCacheTable::relCache[relId]->recId.slot);
    // Write back to the buffer using relCatBlock.setRecord() with recId.slot
  }
  // free the memory allocated in the relation and attribute caches which was
  // allocated in the OpenRelTable::openRel() functioy
  free(RelCacheTable::relCache[relId]);
   
  AttrCacheEntry *entry,*temp;
  entry=AttrCacheTable::attrCache[relId];
  while(entry!=nullptr){
    temp=entry;
    entry=entry->next;
    free(temp);
  }
  

  tableMetaInfo[relId].free = true;
  tableMetaInfo[relId].relName[0] = '\0';
  RelCacheTable::relCache[relId] = nullptr;
   AttrCacheTable::attrCache[relId] = nullptr;
  return SUCCESS;
}