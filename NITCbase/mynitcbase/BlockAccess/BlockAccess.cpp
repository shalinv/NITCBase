#include "BlockAccess.h"

#include <cstring>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    // let block and slot denote the record id of the record being currently checked
    int block, slot;

    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)

        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);

        // block = first record block of the relation
        // slot = 0
        block = relCatEntry.firstBlk;
        slot = 0;
    }
    else
    {
        // (there is a hit from previous search; search should start from
        // the record next to the search index record)
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
        // block = search index's block
        // slot = search index's slot + 1
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
    while (block != -1)
    {
        /* create a RecBuffer object for block (use RecBuffer Constructor for
           existing block) */

        RecBuffer recBuf(block);

        // get the record with id (block, slot) using RecBuffer::getRecord()
        // get header of the block using RecBuffer::getHeader() function
        // get slot map of the block using RecBuffer::getSlotMap() function

        HeadInfo head;
        recBuf.getHeader(&head);

        unsigned char slotMap[head.numSlots];
        recBuf.getSlotMap(slotMap);

        if(slot >= head.numSlots)
        {
            block = head.rblock;
            slot = 0;

            continue;  // continue to the beginning of this while loop
        }

        // if slot is free skip the loop
        if(slotMap[slot] == SLOT_UNOCCUPIED)
        {
            slot++;
            continue;
        }

        //get record
        Attribute record[head.numAttrs];
        recBuf.getRecord(record, slot);

        // compare record's attribute value to the the given attrVal as below:
        /*
            firstly get the attribute offset for the attrName attribute
            from the attribute cache entry of the relation using
            AttrCacheTable::getAttrCatEntry()
        */
       AttrCatEntry attrCatEntry;
       AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);
       int offset = attrCatEntry.offset;
        /* use the attribute offset to get the value of the attribute from
           current record */

        int cmpVal = compareAttrs(record[offset], attrVal, attrCatEntry.attrType);  // will store the difference between the attributes
        // set cmpVal using compareAttrs()

        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)
            */
            RecId foundRecId;
            foundRecId.block = block;
            foundRecId.slot  = slot;

            RelCacheTable::setSearchIndex(relId, &foundRecId);
            return RecId{block, slot};
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);  

    Attribute newRelationName;    // set newRelationName with newName
    strcpy(newRelationName.sVal, newName);

    // search the relation catalog for an entry with "RelName" = newRelationName
    RecId newResult = linearSearch(RELCAT_RELID, (char *)"RelName", newRelationName, EQ);
    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    //    return E_RELEXIST;
    if (newResult.block != -1 && newResult.slot != -1) {
        return E_RELEXIST;
    }   

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID); 

    Attribute oldRelationName;    // set oldRelationName with oldName
    strcpy(oldRelationName.sVal, oldName);

    // search the relation catalog for an entry with "RelName" = oldRelationName
    RecId oldResult = linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, oldRelationName, EQ);
    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    //    return E_RELNOTEXIST;
    if(oldResult.block == -1 && oldResult.slot == -1){
        return E_RELNOTEXIST;
    }

    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    Attribute record[RELCAT_NO_ATTRS];
    RecBuffer recBuff(oldResult.block);
    recBuff.getRecord(record, oldResult.slot);

    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    // set back the record value using RecBuffer.setRecord
    strcpy(record[RELCAT_REL_NAME_INDEX].sVal, newName);
    recBuff.setRecord(record, oldResult.slot);


    /*
    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */


    //for i = 0 to numberOfAttributes :
    //    linearSearch on the attribute catalog for relName = oldRelationName
    //    get the record using RecBuffer.getRecord
    //
    //    update the relName field in the record to newName
    //    set back the record using RecBuffer.setRecord

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numOfAttrs = record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    for(int i=0; i<numOfAttrs; i++){
        RecId recId = BlockAccess::linearSearch(ATTRCAT_RELID, (char *)"RelName", oldRelationName, EQ);
        Attribute attrRecord[ATTRCAT_NO_ATTRS];
        RecBuffer attrBuffer(recId.block);
        attrBuffer.getRecord(attrRecord, recId.slot);

        strcpy(attrRecord[ATTRCAT_REL_NAME_INDEX].sVal, newRelationName.sVal);
        attrBuffer.setRecord(attrRecord, recId.slot);
    }


    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
       RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;    // set relNameAttr to relName
    strcpy(relNameAttr.sVal, relName);

    // Search for the relation with name relName in relation catalog using linearSearch()
    // If relation with name relName does not exist (search returns {-1,-1})
    //    return E_RELNOTEXIST;
    RecId Id = linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttr, EQ);
    if(Id.block == -1 && Id.slot == -1)
        return E_RELNOTEXIST;


    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    /* declare variable attrToRenameRecId used to store the attr-cat recId
    of the attribute to rename */
    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    

    /* iterate over all Attribute Catalog Entry record corresponding to the
       relation to find the required attribute */
    while (true) {
        // linear search on the attribute catalog for RelName = relNameAttr
        Id = linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);
        // if there are no more attributes left to check (linearSearch returned {-1,-1})
        //     break;
        if(Id.block == -1 && Id.slot == -1)
            break;

        /* Get the record from the attribute catalog using RecBuffer.getRecord
          into attrCatEntryRecord */
        RecBuffer AttrCat(Id.block);
        AttrCat.getRecord(attrCatEntryRecord, Id.slot);

        // if attrCatEntryRecord.attrName = oldName
        //     attrToRenameRecId = block and slot of this record
        if(!strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName)){
            attrToRenameRecId.block = Id.block;
            attrToRenameRecId.slot = Id.slot;
        }
        // if attrCatEntryRecord.attrName = newName
        //     return E_ATTREXIST;
        if(!strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName))
            return E_ATTREXIST;
    }

    // if attrToRenameRecId == {-1, -1}
    //     return E_ATTRNOTEXIST;
    if(attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1)
        return E_ATTRNOTEXIST;


    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    RecBuffer Attrcat(attrToRenameRecId.block);
    Attrcat.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);
    //   update the AttrName of the record with newName
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    //   set back the record with RecBuffer.setRecord
    Attrcat.setRecord(attrCatEntryRecord, attrToRenameRecId.slot);

    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record) {
    // get the relation catalog entry from relation cache
    // ( use RelCacheTable::getRelCatEntry() of Cache Layer)
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry); 

    int blockNum = relCatEntry.firstBlk;  // blockNum will be used to traverse the linked list of record blocks of the relation

    // rec_id will be used to store where the new record will be inserted
    RecId rec_id = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;

    int prevBlockNum = -1;  // prevBlockNum will be used to store the block number of the previous block while traversing the linked list of record blocks of the relation

    /*
        Traversing the linked list of existing record blocks of the relation
        until a free slot is found OR
        until the end of the list is reached
    */
    while (blockNum != -1) {
        // create a RecBuffer object for blockNum (using appropriate constructor!)
        RecBuffer recBuf(blockNum);

        // get header of block(blockNum) using RecBuffer::getHeader() function
        HeadInfo head;
        recBuf.getHeader(&head);

        // get slot map of block(blockNum) using RecBuffer::getSlotMap() function
        unsigned char slotMap[head.numSlots];
        recBuf.getSlotMap(slotMap);

        // search for free slot in the block 'blockNum' and store it's rec-id in rec_id
        // (Free slot can be found by iterating over the slot map of the block)
        /* slot map stores SLOT_UNOCCUPIED if slot is free and
           SLOT_OCCUPIED if slot is occupied) */
        for(int i=0; i<numOfSlots; i++){
            if(slotMap[i] == SLOT_UNOCCUPIED){
                rec_id.block = blockNum;
                rec_id.slot = i;
                break;
            }
        }

        /* if a free slot is found, set rec_id and discontinue the traversal
           of the linked list of record blocks (break from the loop) */
        if(rec_id.block != -1 && rec_id.slot != -1){
            break;
        }

        /* otherwise, continue to check the next block by updating the
           block numbers as follows:
              update prevBlockNum = blockNum
              update blockNum = header.rblock (next element in the linked
                                               list of record blocks)
        */
        prevBlockNum = blockNum;
        blockNum = head.rblock;
    }

    
    if(rec_id.block == -1 && rec_id.slot == -1){
        // if relation is RELCAT, do not allocate any more blocks
        //     return E_MAXRELATIONS;
        if(relId == RELCAT_RELID){
            return E_MAXRELATIONS;
        }

        // Otherwise,
        // get a new record block (using the appropriate RecBuffer constructor!)
        // get the block number of the newly allocated block
        // (use BlockBuffer::getBlockNum() function)
        // let ret be the return value of getBlockNum() function call
        RecBuffer newRecBuf;
        int ret = newRecBuf.getBlockNum();

        if (ret == E_DISKFULL) {
            return E_DISKFULL;
        }

        // Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0
        rec_id.block = ret;
        rec_id.slot = 0;

        /*
            set the header of the new record block such that it links with
            existing record blocks of the relation
        */
        struct HeadInfo newHead;
        newHead.blockType = REC;
        newHead.pblock = -1;
        newHead.lblock = prevBlockNum;
        newHead.rblock = -1;
        newHead.numEntries = 0;
        newHead.numSlots = numOfSlots;
        newHead.numAttrs = numOfAttributes;

        newRecBuf.setHeader(&newHead);

        /*
            set block's slot map with all slots marked as free
        */
        unsigned char newSlotMap[numOfSlots];   
        for(int i=0; i<numOfSlots; i++){
            newSlotMap[i] = SLOT_UNOCCUPIED;
        }
        newRecBuf.setSlotMap(newSlotMap);

        if(prevBlockNum != -1)
        {
            // create a RecBuffer object for prevBlockNum
            // get the header of the block prevBlockNum and
            // update the rblock field of the header to the new block
            // number i.e. rec_id.block
                RecBuffer prevRecBuf(prevBlockNum);
                HeadInfo prevHead;
                prevRecBuf.getHeader(&prevHead);
                prevHead.rblock = rec_id.block;
                prevRecBuf.setHeader(&prevHead);
        }
        else
        {
            // update first block field in the relation catalog entry to the
            // new block (using RelCacheTable::setRelCatEntry() function)
            relCatEntry.firstBlk = rec_id.block;
            RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        }

        // update last block field in the relation catalog entry to the
        // new block (using RelCacheTable::setRelCatEntry() function)
        relCatEntry.lastBlk = rec_id.block;
        RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    }

    // create a RecBuffer object for rec_id.block
    // insert the record into rec_id'th slot using RecBuffer.setRecord())
    RecBuffer recBuf(rec_id.block);
    recBuf.setRecord(record, rec_id.slot);

    /* update the slot map of the block by marking entry of the slot to
       which record was inserted as occupied) */
    struct HeadInfo head;
    recBuf.getHeader(&head);
    unsigned char slotMap[relCatEntry.numSlotsPerBlk];
    recBuf.getSlotMap(slotMap);
    slotMap[rec_id.slot] = SLOT_OCCUPIED;
    recBuf.setSlotMap(slotMap);
    // increment the numEntries field in the header of the block to
    // which record was inserted
    // (use BlockBuffer::getHeader() and BlockBuffer::setHeader() functions)
    head.numEntries++;
    recBuf.setHeader(&head);

    // Increment the number of records field in the relation cache entry for
    // the relation. (use RelCacheTable::setRelCatEntry function)
    relCatEntry.numRecs++;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    return SUCCESS;
}