#include "Algebra.h"
#include <iostream>
#include <cstring>
#include <cstdlib>


/* used to select all the records that satisfy a condition.
the arguments of the function are
- srcRel - the source relation we want to select from
- targetRel - the relation we want to select into. (ignore for now)
- attr - the attribute that the condition is checking
- op - the operator of the condition
- strVal - the value that we want to compare against (represented as a string)
*/
// will return if a string can be parsed as a floating point number
bool isNumber(char *str) {
  int len;
  float ignore;
  /*
    sscanf returns the number of elements read, so if there is no float matching
    the first %f, ret will be 0, else it'll be 1

    %n gets the number of characters read. this scanf sequence will read the
    first float ignoring all the whitespace before and after. and the number of
    characters read that far will be stored in len. if len == strlen(str), then
    the string only contains a float with/without whitespace. else, there's other
    characters.
  */
  int ret = sscanf(str, "%f %n", &ignore, &len);
  return ret == 1 && len == strlen(str);
}


int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
  int srcRelId = OpenRelTable::getRelId(srcRel); 
  if (srcRelId == E_RELNOTOPEN) {
    return E_RELNOTOPEN;
  }

  AttrCatEntry attrCatEntry;
  // get the attribute catalog entry for attr, using AttrCacheTable::getAttrcatEntry()
  int flag = AttrCacheTable::getAttrCatEntry(srcRelId,attr, &attrCatEntry);
  if(flag != SUCCESS){
    return E_ATTRNOTEXIST;
  }
  //    return E_ATTRNOTEXIST if it returns the error


  /*** Convert strVal (string) to an attribute of data type NUMBER or STRING ***/
  int type = attrCatEntry.attrType;
  Attribute attrVal;
  if (type == NUMBER) {
    if (isNumber(strVal)) {       // the isNumber() function is implemented below
      attrVal.nVal = atof(strVal);
    } else {
      return E_ATTRTYPEMISMATCH;
    }
  } else if (type == STRING) {
    strcpy(attrVal.sVal, strVal);
  }

    /*** Creating and opening the target relation ***/
    // Prepare arguments for createRel() in the following way:
    // get RelcatEntry of srcRel using RelCacheTable::getRelCatEntry()
    RelCatEntry srcRelCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &srcRelCatEntry);
    int src_nAttrs = srcRelCatEntry.numAttrs;


    /* let attr_names[src_nAttrs][ATTR_SIZE] be a 2D array of type char
        (will store the attribute names of rel). */
    // let attr_types[src_nAttrs] be an array of type int
    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_types[src_nAttrs];

    for(int i=0; i<src_nAttrs; i++){
      AttrCatEntry srcAttrCatEntry;
      AttrCacheTable::getAttrCatEntry(srcRelId, i, &srcAttrCatEntry);
      strcpy(attr_names[i], srcAttrCatEntry.attrName);
      attr_types[i] = srcAttrCatEntry.attrType;
    }

    /* Create the relation for target relation by calling Schema::createRel()
       by providing appropriate arguments */
    // if the createRel returns an error code, then return that value.
    int ret = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_types);
    if(ret != SUCCESS){
      return ret;
    }

    /* Open the newly created target relation by calling OpenRelTable::openRel()
       method and store the target relid */
    /* If opening fails, delete the target relation by calling Schema::deleteRel()
       and return the error value returned from openRel() */
    int tarRelId = OpenRelTable::openRel(targetRel);
    if(tarRelId < 0 || tarRelId > MAX_OPEN){
      return tarRelId;
    }

    /*** Selecting and inserting records into the target relation ***/
    /* Before calling the search function, reset the search to start from the
       first using RelCacheTable::resetSearchIndex() */

    Attribute record[src_nAttrs];

    /*
        The BlockAccess::search() function can either do a linearSearch or
        a B+ tree search. Hence, reset the search index of the relation in the
        relation cache using RelCacheTable::resetSearchIndex().
        Also, reset the search index in the attribute cache for the select
        condition attribute with name given by the argument `attr`. Use
        AttrCacheTable::resetSearchIndex().
        Both these calls are necessary to ensure that search begins from the
        first record.
    */
    RelCacheTable::resetSearchIndex(srcRelId);
    AttrCacheTable::resetSearchIndex(srcRelId, attr);

    // read every record that satisfies the condition by repeatedly calling
    // BlockAccess::search() until there are no more records to be read
    LScount =0;
    BTScount =0;
    while (BlockAccess::search(srcRelId, record, attr, attrVal, op) == SUCCESS) {
        ret = BlockAccess::insert(tarRelId, record);

        if(ret != SUCCESS){
          Schema::closeRel(targetRel);
          Schema::deleteRel(targetRel);
          return ret;
        }
    }

    Schema::closeRel(targetRel);

    return SUCCESS;
}

int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE]){
    // if relName is equal to "RELATIONCAT" or "ATTRIBUTECAT"
    // return E_NOTPERMITTED;
    if(!strcmp(relName, "RELATIONCAT") || !strcmp(relName, "ATTRIBUTECAT")){
      return E_NOTPERMITTED;
    }

    // get the relation's rel-id using OpenRelTable::getRelId() method
    int relId = OpenRelTable::getRelId(relName);

    // if relation is not open in open relation table, return E_RELNOTOPEN
    // (check if the value returned from getRelId function call = E_RELNOTOPEN)
    // get the relation catalog entry from relation cache
    // (use RelCacheTable::getRelCatEntry() of Cache Layer)
    if(relId == E_RELNOTOPEN){
      return E_RELNOTOPEN;
    }
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    /* if relCatEntry.numAttrs != numberOfAttributes in relation,
       return E_NATTRMISMATCH */
    if(relCatEntry.numAttrs != nAttrs){
      return E_NATTRMISMATCH;
    }

    // let recordValues[numberOfAttributes] be an array of type union Attribute
    union Attribute recordValues[nAttrs];

    /*
        Converting 2D char array of record values to Attribute array recordValues
     */

    // iterate through 0 to nAttrs-1: (let i be the iterator)
    for(int i = 0; i < nAttrs; i++)
    {
        // get the attr-cat entry for the i'th attribute from the attr-cache
        // (use AttrCacheTable::getAttrCatEntry())
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, i, &attrCatEntry);

        // let type = attrCatEntry.attrType;
        int type = attrCatEntry.attrType;

        if (type == NUMBER)
        {
            // if the char array record[i] can be converted to a number
            // (check this using isNumber() function)
              if (isNumber(record[i]))
            {
                /* convert the char array to numeral and store it
                   at recordValues[i].nVal using atof() */
                recordValues[i].nVal = atof(record[i]);
            }
            // else
            else 
            {
                return E_ATTRTYPEMISMATCH;
            }
        }
        else if (type == STRING)
        {
            // copy record[i] to recordValues[i].sVal
            strcpy(recordValues[i].sVal, record[i]);
        }
    }

    // insert the record by calling BlockAccess::insert() function
    // let retVal denote the return value of insert call
    int retVal = BlockAccess::insert(relId, recordValues);

    return retVal;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) {

    int srcRelId = OpenRelTable::getRelId(srcRel);

    if(srcRelId < 0 || srcRelId > MAX_OPEN){
      return E_RELNOTOPEN;
    }

    // get RelCatEntry of srcRel using RelCacheTable::getRelCatEntry()
    RelCatEntry srcRelCatBuf;
    RelCacheTable::getRelCatEntry(srcRelId, &srcRelCatBuf);

    // get the no. of attributes present in relation from the fetched RelCatEntry.
    int numAttrs = srcRelCatBuf.numAttrs;

    // attrNames and attrTypes will be used to store the attribute names
    // and types of the source relation respectively
    char attrNames[numAttrs][ATTR_SIZE];
    int attrTypes[numAttrs];

    for(int i=0; i< numAttrs; i++){
      AttrCatEntry AttrCatBuf;
      AttrCacheTable::getAttrCatEntry(srcRelId, i, &AttrCatBuf);
      attrTypes[i] = AttrCatBuf.attrType;
      strcpy(attrNames[i], AttrCatBuf.attrName);
    }


    /*** Creating and opening the target relation ***/

    // Create a relation for target relation by calling Schema::createRel()
    int ret = Schema::createRel(targetRel, numAttrs, attrNames, attrTypes);

    // if the createRel returns an error code, then return that value.
    if(ret != SUCCESS){
      return ret;
    }

    // Open the newly created target relation by calling OpenRelTable::openRel()
    // and get the target relid

    // If opening fails, delete the target relation by calling Schema::deleteRel() of
    // return the error value returned from openRel().

    int RelId = OpenRelTable::openRel(targetRel);
    if(RelId < 0 || RelId > MAX_OPEN){
      Schema::deleteRel(targetRel);
      return RelId;
    }

    /*** Inserting projected records into the target relation ***/

    // Take care to reset the searchIndex before calling the project function
    // using RelCacheTable::resetSearchIndex()
    RelCacheTable::resetSearchIndex(srcRelId);
    Attribute record[numAttrs];

    while (BlockAccess::project(srcRelId, record) == SUCCESS)
    {
        // record will contain the next record
        ret = BlockAccess::insert(RelId, record);
        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }

    // Close the targetRel by calling Schema::closeRel()
    Schema::closeRel(targetRel);
    return SUCCESS;
}


int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) {

    int srcRelId = OpenRelTable::getRelId(srcRel);

    // if srcRel is not open in open relation table, return E_RELNOTOPEN
    if(srcRelId < 0 || srcRelId > MAX_OPEN){
      return E_RELNOTOPEN;
    }

    // get RelCatEntry of srcRel using RelCacheTable::getRelCatEntry()
    RelCatEntry srcRelCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &srcRelCatEntry);

    // get the no. of attributes present in relation from the fetched RelCatEntry.
    int srcNumAttrs = srcRelCatEntry.numAttrs;

    // declare attr_offset[tar_nAttrs] an array of type int.
    int attr_offset[tar_nAttrs];
    // where i-th entry will store the offset in a record of srcRel for the
    // i-th attribute in the target relation.

    // let attr_types[tar_nAttrs] be an array of type int.
    int attr_types[tar_nAttrs];
    // where i-th entry will store the type of the i-th attribute in the
    // target relation.


    /*** Checking if attributes of target are present in the source relation
         and storing its offsets and types ***/

    for(int i=0; i<tar_nAttrs; i++){
      AttrCatEntry AttrCatBuffer;
      int ret = AttrCacheTable::getAttrCatEntry(srcRelId, tar_Attrs[i], &AttrCatBuffer);
      if (ret != SUCCESS) {
        return ret;
      }
      // Store offset and type for later use
      attr_offset[i] = AttrCatBuffer.offset;
      attr_types[i] = AttrCatBuffer.attrType;
    }


    /*** Creating and opening the target relation ***/

    // Create a relation for target relation by calling Schema::createRel()
    int ret = Schema::createRel(targetRel, tar_nAttrs, tar_Attrs, attr_types);
    if(ret != SUCCESS){
      return ret;
    }

    // Open the newly created target relation by calling OpenRelTable::openRel()
    // and get the target relid
    int targetRelId = OpenRelTable::openRel(targetRel);

    if(targetRelId < 0 || targetRelId > MAX_OPEN){
      Schema::deleteRel(targetRel);
      return targetRelId;
    }

    /*** Inserting projected records into the target relation ***/

    // Take care to reset the searchIndex before calling the project function
    // using RelCacheTable::resetSearchIndex()
    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[srcNumAttrs];

    while (BlockAccess::project(srcRelId, record) == SUCCESS) {
        // the variable `record` will contain the next record

        Attribute proj_record[tar_nAttrs];

        for(int i=0; i<tar_nAttrs; i++){
          proj_record[i] = record[attr_offset[i]];
        }
        ret = BlockAccess::insert(targetRelId, proj_record);

        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }

    Schema::closeRel(targetRel);

    return SUCCESS; 
}

int Algebra::join(char srcRelation1[ATTR_SIZE], char srcRelation2[ATTR_SIZE], char targetRelation[ATTR_SIZE], char attribute1[ATTR_SIZE], char attribute2[ATTR_SIZE])
{
    // 1. Get relation IDs
    int relId1 = OpenRelTable::getRelId(srcRelation1);
    int relId2 = OpenRelTable::getRelId(srcRelation2);

    if (relId1 < 0 || relId1 >= MAX_OPEN || relId2 < 0 || relId2 >= MAX_OPEN)
        return E_RELNOTOPEN;

    // 2. Get attribute entries
    AttrCatEntry attr1, attr2;
    if (AttrCacheTable::getAttrCatEntry(relId1, attribute1, &attr1) != SUCCESS)
        return E_ATTRNOTEXIST;

    if (AttrCacheTable::getAttrCatEntry(relId2, attribute2, &attr2) != SUCCESS)
        return E_ATTRNOTEXIST;

    // 3. Type check
    if (attr1.attrType != attr2.attrType)
        return E_ATTRTYPEMISMATCH;

    // 4. Get relation metadata
    RelCatEntry rel1, rel2;
    RelCacheTable::getRelCatEntry(relId1, &rel1);
    RelCacheTable::getRelCatEntry(relId2, &rel2);

    int n1 = rel1.numAttrs;
    int n2 = rel2.numAttrs;

    // 5. Duplicate attribute check (excluding join attribute2)
    AttrCatEntry temp1, temp2;
    for (int i = 0; i < n2; i++)
    {
        if (i == attr2.offset) continue;

        AttrCacheTable::getAttrCatEntry(relId2, i, &temp2);

        for (int j = 0; j < n1; j++)
        {
            AttrCacheTable::getAttrCatEntry(relId1, j, &temp1);

            if (strcmp(temp2.attrName, temp1.attrName) == 0)
                return E_DUPLICATEATTR;
        }
    }

    // 6. Create index on relation2 if not present
    if (attr2.rootBlock == -1)
    {
        int ret = BPlusTree::bPlusCreate(relId2, attribute2);
        if (ret != SUCCESS)
            return ret;

        // refresh metadata
        AttrCacheTable::getAttrCatEntry(relId2, attribute2, &attr2);
    }

    // 7. Prepare target schema
    int targetAttrs = n1 + n2 - 1;
    char attrNames[targetAttrs][ATTR_SIZE];
    int attrTypes[targetAttrs];

    // copy rel1 attributes
    for (int i = 0; i < n1; i++)
    {
        AttrCacheTable::getAttrCatEntry(relId1, i, &temp1);
        strcpy(attrNames[i], temp1.attrName);
        attrTypes[i] = temp1.attrType;
    }

    // copy rel2 attributes (skip join attr)
    int idx = n1;
    for (int i = 0; i < n2; i++)
    {
        if (i == attr2.offset) continue;

        AttrCacheTable::getAttrCatEntry(relId2, i, &temp2);
        strcpy(attrNames[idx], temp2.attrName);
        attrTypes[idx] = temp2.attrType;
        idx++;
    }

    // 8. Create target relation
    int ret = Schema::createRel(targetRelation, targetAttrs, attrNames, attrTypes);
    if (ret != SUCCESS)
        return ret;

    int targetRelId = OpenRelTable::openRel(targetRelation);
    if (targetRelId < 0 || targetRelId >= MAX_OPEN)
    {
        Schema::deleteRel(targetRelation);
        return targetRelId;
    }

    // 9. Prepare records
    Attribute rec1[n1], rec2[n2], out[targetAttrs];

    // reset outer relation
    RelCacheTable::resetSearchIndex(relId1);

    // 10. Join logic
    while (BlockAccess::project(relId1, rec1) == SUCCESS)
    {
        RelCacheTable::resetSearchIndex(relId2);
        AttrCacheTable::resetSearchIndex(relId2, attribute2);

        while (BlockAccess::search(relId2, rec2, attribute2,
                                   rec1[attr1.offset], EQ) == SUCCESS)
        {
            // copy rel1
            for (int i = 0; i < n1; i++)
                out[i] = rec1[i];

            // copy rel2 except join attr
            int k = n1;
            for (int i = 0; i < n2; i++)
            {
                if (i == attr2.offset) continue;
                out[k++] = rec2[i];
            }

            ret = BlockAccess::insert(targetRelId, out);
            if (ret != SUCCESS)
            {
                OpenRelTable::closeRel(targetRelId);
                Schema::deleteRel(targetRelation);
                return ret;
            }
        }
    }

    // 11. Close relation
    OpenRelTable::closeRel(targetRelId);

    return SUCCESS;
}