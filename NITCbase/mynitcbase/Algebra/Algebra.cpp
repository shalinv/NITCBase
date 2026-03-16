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
  int srcRelId = OpenRelTable::getRelId(srcRel);      // we'll implement this later
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

  /*** Selecting records from the source relation ***/

  // Before calling the search function, reset the search to start from the first hit
  // using RelCacheTable::resetSearchIndex()
  RelCacheTable::resetSearchIndex(srcRelId);

  RelCatEntry relCatEntry;
  // get relCatEntry using RelCacheTable::getRelCatEntry()
  RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

  /************************
  The following code prints the contents of a relation directly to the output
  console. Direct console output is not permitted by the actual the NITCbase
  specification and the output can only be inserted into a new relation. We will
  be modifying it in the later stages to match the specification.
  ************************/

  printf("|");
  for (int i = 0; i < relCatEntry.numAttrs; ++i) {
    AttrCatEntry attrCatEntry;
    // get attrCatEntry at offset i using AttrCacheTable::getAttrCatEntry()
    AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);

    printf(" %s |", attrCatEntry.attrName);
  }
  printf("\n");

  while (true) {
    RecId searchRes = BlockAccess::linearSearch(srcRelId, attr, attrVal, op);

    if (searchRes.block != -1 && searchRes.slot != -1) {

      // get the record at searchRes using BlockBuffer.getRecord
      Attribute record[relCatEntry.numAttrs];
      RecBuffer recBuf(searchRes.block);
      recBuf.getRecord(record, searchRes.slot);

      // print the attribute values in the same format as above
      printf("|");
      for (int i = 0; i < relCatEntry.numAttrs; i++) {
        AttrCatEntry tempAttr;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &tempAttr);

        if (tempAttr.attrType == NUMBER) {
          printf(" %.2f |", record[i].nVal);
        } else {
          printf(" %s |", record[i].sVal);
        }
      }
      printf("\n");

    } else {
      // (all records over)
      break;
    }
  }

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