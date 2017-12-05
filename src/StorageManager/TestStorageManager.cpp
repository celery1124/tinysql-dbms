#include<iostream>
#include<fstream>
#include<iterator>
#include<cstdlib>
#include<string>
#include<ctime>
#include "Block.h"
#include "Config.h"
#include "Disk.h"
#include "Field.h"
#include "MainMemory.h"
#include "Relation.h"
#include "Schema.h"
#include "SchemaManager.h"
#include "Tuple.h"

using namespace std;

ofstream fout;

// An example procedure of appending a tuple to the end of a relation
// using memory block "memory_block_index" as output buffer
void appendTupleToRelation(Relation* relation_ptr, MainMemory& mem, int memory_block_index, Tuple& tuple) {
  Block* block_ptr;
  if (relation_ptr->getNumOfBlocks()==0) {
    fout << "The relation is empty" << endl;
    fout << "Get the handle to the memory block " << memory_block_index << " and clear it" << endl;
    block_ptr=mem.getBlock(memory_block_index);
    block_ptr->clear(); //clear the block
    block_ptr->appendTuple(tuple); // append the tuple
    fout << "Write to the first block of the relation" << endl;
    relation_ptr->setBlock(relation_ptr->getNumOfBlocks(),memory_block_index);
  } else {
    fout << "Read the last block of the relation into memory block 5:" << endl;
    relation_ptr->getBlock(relation_ptr->getNumOfBlocks()-1,memory_block_index);
    block_ptr=mem.getBlock(memory_block_index);

    if (block_ptr->isFull()) {
      fout << "(The block is full: Clear the memory block and append the tuple)" << endl;
      block_ptr->clear(); //clear the block
      block_ptr->appendTuple(tuple); // append the tuple
      fout << "Write to a new block at the end of the relation" << endl;
      relation_ptr->setBlock(relation_ptr->getNumOfBlocks(),memory_block_index); //write back to the relation
    } else {
      fout << "(The block is not full: Append it directly)" << endl;
      block_ptr->appendTuple(tuple); // append the tuple
      fout << "Write to the last block of the relation" << endl;
      relation_ptr->setBlock(relation_ptr->getNumOfBlocks()-1,memory_block_index); //write back to the relation
    }
  }  
}

int main(){
  fout.open("test_log");
  if (!fout) {
    cerr << "The file test_log is not opened" << endl;
    return -1;
  }

  //=======================Initialization=========================
  fout << "=======================Initialization=========================" << endl;
  
  // Initialize the memory, disk and the schema manager
  MainMemory mem;
  Disk disk;
  fout << "The memory contains " << mem.getMemorySize() << " blocks" << endl;
  fout << mem << endl << endl;
  SchemaManager schema_manager(&mem,&disk);

  disk.resetDiskIOs();
  disk.resetDiskTimer();

  // Another way to time
  clock_t start_time;
  start_time=clock();

  //=======================Schema=========================
  fout << "=======================Schema=========================" << endl;
  
  // Create a schema
  fout << "Creating a schema" << endl;
  vector<string> field_names;
  vector<enum FIELD_TYPE> field_types;
  field_names.push_back("f1");
  field_names.push_back("f2");
  field_names.push_back("f3");
  field_names.push_back("f4");
  field_types.push_back(INT);
  field_types.push_back(STR20);
  field_types.push_back(INT);
  field_types.push_back(STR20);
  Schema schema(field_names,field_types);

  // Print the information about the schema
  fout << schema << endl;
  fout << "The schema has " << schema.getNumOfFields() << " fields" << endl;
  fout << "The schema allows " << schema.getTuplesPerBlock() << " tuples per block" << endl;
  fout << "The schema has field names: " << endl;
  field_names=schema.getFieldNames();
  copy(field_names.begin(),field_names.end(),ostream_iterator<string,char>(fout," "));
  fout << endl;
  fout << "The schema has field types: " << endl;
  field_types=schema.getFieldTypes();
  for (int i=0;i<schema.getNumOfFields();i++) {
    fout << (field_types[i]==0?"INT":"STR20") << "\t";
  }
  fout << endl;  
  fout << "The first field is of name " << schema.getFieldName(0) << endl;
  fout << "The second field is of type " << (schema.getFieldType(1)==0?"INT":"STR20") << endl;
  fout << "The field f3 is of type " << (schema.getFieldType("f3")==0?"INT":"STR20") << endl;
  fout << "The field f4 is at offset " << schema.getFieldOffset("f4") << endl << endl;

  //Error testing
  fout << "Error testing: " << endl;
  schema.getFieldName(-1); // out of bound
  schema.getFieldName(schema.getNumOfFields()); // out of bound
  schema.getFieldType(-1); // out of bound
  schema.getFieldType(schema.getNumOfFields()); // out of bound
  schema.getFieldType("test"); // field name does not exist
  schema.getFieldOffset("test"); // field name does not exist
  
  field_names.push_back("f4"); // same field name
  field_types.push_back(STR20);
  Schema schema_error(field_names,field_types);

  field_names[4]=""; // empty field name
  Schema schema_error2(field_names,field_types);
  
  field_names[4]="f5"; // corrects field name
  field_names.push_back("f6");
  field_names.push_back("f7");
  field_names.push_back("f8");
  field_names.push_back("f9");
  field_types.push_back(STR20);
  field_types.push_back(STR20);
  field_types.push_back(STR20);
  field_types.push_back(STR20);
  Schema schema_error3(field_names,field_types);
  
  field_types.pop_back();
  Schema schema_error4(field_names,field_types); // vector sizes unmatched

  vector<string> vs;
  vector<enum FIELD_TYPE> vf;
  Schema schema_error5(vs,vf); // empty vector

  //restore the fields for later use
  field_names.pop_back();
  for (int i=0;i<4;i++) {
    field_names.pop_back();
    field_types.pop_back();
  }
  fout << endl;
  
  //=====================Relation & SchemaManager=========================
  fout << "=====================Relation & SchemaManager=========================" << endl;
  
  // Create a relation with the created schema through the schema manager
  string relation_name="ExampleTable1";
  fout << "Creating table " << relation_name << endl;  
  Relation* relation_ptr=schema_manager.createRelation(relation_name,schema);

  // Print the information about the Relation
  fout << "The table has name " << relation_ptr->getRelationName() << endl;
  fout << "The table has schema:" << endl;
  fout << relation_ptr->getSchema() << endl;
  fout << "The table currently have " << relation_ptr->getNumOfBlocks() << " blocks" << endl;
  fout << "The table currently have " << relation_ptr->getNumOfTuples() << " tuples" << endl << endl;

  // Error testing
  fout << "Error testing: " << endl;
  schema_manager.createRelation(relation_name,schema); // create a relation with the same name
  schema_manager.createRelation("test",Schema()); // create a relation with empty schema
  fout << endl;
  
  // Print the information provided by the schema manager
  fout << "Current schemas and relations: " << endl;
  fout << schema_manager << endl;
  fout << "From the schema manager, the table " << relation_name << " exists: "
       << (schema_manager.relationExists(relation_name)?"TRUE":"FALSE") << endl;
  fout << "From the schema manager, the table " << relation_name << " has schema:" << endl;
  fout << schema_manager.getSchema(relation_name) << endl;
  fout << "From the schema manager, the table " << relation_name << " has schema:" << endl;
  fout << schema_manager.getRelation(relation_name)->getSchema() << endl;

  fout << "Creating table ExampleTable2 with the same schema" << endl;
  schema_manager.createRelation("ExampleTable2",schema);
  fout << "After creating a realtion, current schemas and relations: " << endl;
  fout << schema_manager << endl;

  fout << "Creating table ExampleTable3 with a different schema" << endl;
  field_types[1]=INT;
  Schema schema3(field_names,field_types);  
  fout << "The schema has field names: " << endl;
  field_names=schema3.getFieldNames();
  copy(field_names.begin(),field_names.end(),ostream_iterator<string,char>(fout," "));
  fout << endl;
  fout << "The schema has field types: " << endl;
  field_types=schema3.getFieldTypes();
  for (int i=0;i<schema.getNumOfFields();i++) {
    fout << (field_types[i]==0?"INT":"STR20") << "\t";
  }  
  relation_ptr = schema_manager.createRelation("ExampleTable3",schema3);
  fout << "After creating a realtion, current schemas and relations: " << endl;
  fout << schema_manager << endl;  

  fout << "Deleting table ExampleTable2" << endl;
  schema_manager.deleteRelation("ExampleTable2");
  fout << "After deleting a realtion, current schemas and relations: " << endl;
  fout << schema_manager << endl << endl;

  //Error testing
  fout << "Error testing: " << endl;
  fout << "The table ExampleTable2 exists: " << (schema_manager.relationExists("ExampleTable2")?"TRUE":"FALSE") << endl;
  schema_manager.createRelation("",schema); // empty relation name
  schema_manager.getSchema("ExampleTable2"); // relation does not exist
  schema_manager.getRelation("ExampleTable2"); // relation does not exist
  schema_manager.deleteRelation("ExampleTable2"); // relation does not exist
  fout << endl;
  
  //====================Tuple=============================
  fout << "====================Tuple=============================" << endl;

  // Set up the first tuple
  Tuple tuple = relation_ptr->createTuple(); //The only way to create a tuple is to call "Relation"
  tuple.setField(0,"v11");
  tuple.setField(1,21);
  tuple.setField(2,31);
  tuple.setField(3,"v41");

  // Another way of setting the tuples
  tuple.setField("f1","v11");
  tuple.setField("f2",21);
  tuple.setField("f3",31);
  tuple.setField("f4","v41");
  
  // Print the information about the tuple
  fout << "Created a tuple " << tuple << " through the relation" << endl;
  fout << "The tuple is invalid? " << (tuple.isNull()?"TRUE":"FALSE") << endl;
  Schema tuple_schema = tuple.getSchema();
  fout << "The tuple has schema" << endl;
  fout << tuple_schema << endl;
  fout << "A block can allow at most " << tuple.getTuplesPerBlock() << " such tuples" << endl;
  
  fout << "The tuple has fields: " << endl;
  for (int i=0; i<tuple.getNumOfFields(); i++) {
    if (tuple_schema.getFieldType(i)==INT)
      fout << tuple.getField(i).integer << "\t";
    else
      fout << *(tuple.getField(i).str) << "\t";
  }
  fout << endl;

  fout << "The tuple has fields: " << endl;
  fout << *(tuple.getField("f1").str) << "\t";
  fout << tuple.getField("f2").integer << "\t";
  fout << tuple.getField("f3").integer << "\t";
  fout << *(tuple.getField("f4").str) << "\t";
  fout << endl << endl;

  //Error testing
  fout << "Error testing: " << endl;
  tuple.setField(0,11); // wrong type of value
  tuple.setField(-1,"v11"); // out of bound
  tuple.setField(tuple.getNumOfFields(),"v11"); // out of bound
  tuple.setField("f2","v21"); // wrong type of value
  tuple.setField("f5",21); // field does not exist
  tuple.getField(-1); // out of bound
  tuple.getField(tuple.getNumOfFields()); // out of bound
  fout << endl;
  
  //===================Block=============================
  fout << "===================Block=============================" << endl;
   
  // Set up a block in the memory
  fout << "Clear the memory block 0" << endl;
  Block* block_ptr=mem.getBlock(0); //access to memory block 0
  block_ptr->clear(); //clear the block

  // A block stores at most 2 tuples in this case
  // -----------first tuple-----------
  fout << "Set the tuple at offset 0 of the memory block 0" << endl;
  block_ptr->setTuple(0,tuple); // You can also use appendTuple()
  fout << "Now the memory block 0 contains:" << endl;
  fout << *block_ptr << endl;

  fout << "The block is full? " << (block_ptr->isFull()==1?"true":"false") << endl;
  fout << "The block currently has " << block_ptr->getNumTuples() << " tuples" << endl;
  fout << "The tuple at offset 0 of the block is:" << endl;
  fout << block_ptr->getTuple(0) << endl << endl;

  // -----------second tuple-----------
  fout << "Append the same tuple to the memory block 0" << endl;
  block_ptr->appendTuple(tuple);
  fout << "Now the memory block 0 contains:" << endl;
  fout << *block_ptr << endl;
  
  fout << "The block is full? " << (block_ptr->isFull()==1?"true":"false") << endl;
  fout << "The block currently has " << block_ptr->getNumTuples() << " tuples" << endl;
  fout << "The tuple at offset 0 of the block is:" << endl;
  fout << block_ptr->getTuple(0) << endl;

  vector<Tuple> tuples=block_ptr->getTuples();
  fout << "Again the tuples in the memory block 0 are:" << endl;
  copy(tuples.begin(),tuples.end(),ostream_iterator<Tuple,char>(fout,"\n"));
  fout << endl;

  // -----------erase and add-----------
  fout << "Erase the first tuple" << endl;
  block_ptr->nullTuple(0);
  fout << "Now the memory block 0 contains:" << endl;
  fout << *block_ptr << endl;

  fout << "Erase all the tuples in the block" << endl;
  block_ptr->nullTuples();
  fout << "Now the memory block 0 contains:" << endl;
  fout << *block_ptr << endl;

  fout << "(Remove all tuples;) Set only the first tuple" << endl;
  block_ptr->setTuples(tuples.begin(),tuples.begin()+1);
  fout << "Now the memory block 0 contains:" << endl;
  fout << *block_ptr << endl << endl;

  fout << "(Remove all tuples;) Set the same two tuples again" << endl;
  block_ptr->setTuples(tuples);
  fout << "Now the memory block 0 contains:" << endl;
  fout << *block_ptr << endl;
  
  //Error testing
  fout << "Error testing: " << endl;
  Tuple tuple2 = schema_manager.getRelation("ExampleTable1")->createTuple();
  tuple2.setField(0,"v11");
  tuple2.setField(1,"v21");
  tuple2.setField(2,31);
  tuple2.setField(3,"v41");
  block_ptr->setTuple(1,tuple2); // different schemas
  
  block_ptr->setTuple(-1,tuple); //out of bound
  block_ptr->setTuple(tuple.getTuplesPerBlock(),tuple); //out of bound
  block_ptr->getTuple(-1); //out of bound
  block_ptr->getTuple(tuple.getTuplesPerBlock()); //out of bound
  block_ptr->nullTuple(-1); //out of bound
  block_ptr->nullTuple(tuple.getTuplesPerBlock()); //out of bound
  block_ptr->appendTuple(tuple); // append a tuple to a full block
  fout << endl;

  //======How to append tuples to the end of the relation======
  fout << endl << "======How to append tuples to the end of the relation======" << endl;

  // ---------Append the fisrt tuple---------
  
  fout << "Now memory contains: " << endl;
  fout << mem << endl;

  // append the tuple to the end of the ExampleTable3 using memory block 5 as output buffer
  appendTupleToRelation(relation_ptr,mem,5,tuple);
  fout << "Now the memory contains: " << endl;
  fout << mem << endl;
  fout << "Now the relation contains: " << endl;
  fout << *relation_ptr << endl << endl;
  
  // ---------Set the second tuple---------
  
  fout << "Create the second tuple " << endl;
  tuple.setField("f1","v12");
  tuple.setField("f2",22);
  tuple.setField("f3",32);
  tuple.setField("f4","v42");
  fout << tuple << endl;

  // append the tuple to the end of the ExampleTable3 using memory block 5 as output buffer
  appendTupleToRelation(relation_ptr,mem,5,tuple);
  fout << "*NOTE: The example here does not consider empty tuples (if any) in the block." << endl;
  fout << "(The holes left after tuple deletion)" << endl;
  
  fout << "Now the memory contains: " << endl;
  fout << mem << endl;
  fout << "Now the relation contains: " << endl;
  fout << *relation_ptr << endl << endl;

  // ---------Set the thrid tuple---------
  
  fout << "Create the third tuple " << endl;
  tuple.setField("f1","v13");
  tuple.setField("f2",23);
  tuple.setField("f3",33);
  tuple.setField("f4","v43");
  fout << tuple << endl;

  // append the tuple to the end of the ExampleTable3 using memory block 5 as output buffer
  appendTupleToRelation(relation_ptr,mem,5,tuple);
  fout << "*NOTE: The example here does not consider empty tuples (if any) in the block." << endl;
  fout << "(The holes left after tuple deletion)" << endl;

  fout << "Now the memory contains: " << endl;
  fout << mem << endl;
  fout << "Now the relation contains: " << endl;
  fout << *relation_ptr << endl << endl;

  //======How to handle bulk blocks======
  fout << endl << "======How to read and write bulk blocks======" << endl;

  fout << "First fill the relations with 10 more tuples" << endl;
  for (int i=0;i<10;i++) {
    appendTupleToRelation(relation_ptr,mem,5,tuple);
  }
  fout << "Now the relation contains: " << endl;
  fout << *relation_ptr << endl << endl;
  fout << "Now the memory contains: " << endl;
  fout << mem << endl;
  
  fout << "Read bulk blocks from the relation to memory block 3-9" << endl;
  relation_ptr->getBlocks(0,3,relation_ptr->getNumOfBlocks());
  fout << "Now the memory contains: " << endl;
  fout << mem << endl;

  fout << "Write bulk blocks from the memory block 3-9 to the end of the relation" << endl;
  fout << "(May result in 'holes' in the relation)" << endl;
  relation_ptr->setBlocks(relation_ptr->getNumOfBlocks(),3,9-3+1);
  fout << "Now the relation contains: " << endl;
  fout << *relation_ptr << endl;

  fout << "Deleting the last 7 blocks of the relation" << endl;
  relation_ptr->deleteBlocks(7);
  fout << "Now the relation contains: " << endl;
  fout << *relation_ptr << endl << endl;
  
  //======How to delete tuples from the relation======
  fout << endl << "======How to delete tuples from the relation======" << endl;

  // ---------Erase one tuple---------
  
  fout << "Reading the first block of the relation into memory block 1:" << endl;
  relation_ptr->getBlock(0,1);
  fout << "Now the memory contains: " << endl;
  fout << mem << endl;

  fout << "Deleting the tuple at offset 0 of the memory block 1" << endl;
  block_ptr=mem.getBlock(1);
  block_ptr->nullTuple(0);
  fout << "Now the memory contains: " << endl;
  fout << mem << endl;

  fout << "Writing memory block 1 back to the first block of the relation" << endl;
  relation_ptr->setBlock(0,1);
  fout << "Now the relation contains: " << endl;
  fout << *relation_ptr << endl;

  // ---------Erase another tuple---------
  
  fout << "Reading the last block of the relation into memory block 1:" << endl;
  relation_ptr->getBlock(relation_ptr->getNumOfBlocks()-1,1);
  fout << "Now the memory contains: " << endl;
  fout << mem << endl;

  fout << "Emptying the tuples at the memory block 1" << endl;
  block_ptr=mem.getBlock(1);
  block_ptr->nullTuples();
  fout << "Now the memory contains: " << endl;
  fout << mem << endl;

  fout << "Writing memory block 1 back to the last block of the relation" << endl;
  relation_ptr->setBlock(relation_ptr->getNumOfBlocks()-1,1);
  fout << "Now the relation contains: " << endl;
  fout << *relation_ptr << endl;
  
  // ---------Delete Block---------
  
  fout << "Deleting the last block of the relation to remove trailing space" << endl;
  relation_ptr->deleteBlocks(relation_ptr->getNumOfBlocks()-1);
  fout << "Now the relation contains: " << endl;
  fout << *relation_ptr << endl << endl;
  
  //Error testing of Relation
  fout << "Error testing of Relation: " << endl;
  relation_ptr->getBlock(-1,5); //out of bound
  relation_ptr->getBlock(relation_ptr->getNumOfBlocks(),5); //out of bound
  relation_ptr->getBlock(0,-1); //out of bound
  relation_ptr->getBlock(0,mem.getMemorySize()); //out of bound
  relation_ptr->getBlocks(-1,5,1); //out of bound
  relation_ptr->getBlocks(relation_ptr->getNumOfBlocks(),5,1); //out of bound
  relation_ptr->getBlocks(0,-1,1); //out of bound
  relation_ptr->getBlocks(0,mem.getMemorySize(),1); //out of bound
  relation_ptr->getBlocks(0,5,6); //out of memory bound
  relation_ptr->getBlocks(6,5,2); //out of relation bound 
  relation_ptr->setBlock(-1,5); //out of bound
  relation_ptr->setBlock(0,-1); //out of bound
  relation_ptr->setBlock(0,mem.getMemorySize()); //out of bound
  relation_ptr->setBlocks(-1,5,1); //out of bound
  relation_ptr->setBlocks(0,-1,1); //out of bound
  relation_ptr->setBlocks(0,mem.getMemorySize(),1); //out of bound
  relation_ptr->setBlocks(0,5,6); //out of memory bound
  
  relation_ptr->deleteBlocks(-1); //out of bound
  relation_ptr->deleteBlocks(relation_ptr->getNumOfBlocks()); //out of bound
  schema_manager.getRelation("ExampleTable1")->setBlock(0,0); // different table schemas
  schema_manager.getRelation("ExampleTable1")->setBlocks(0,0,1); // different table schemas
  fout << endl;

  //===================Memory=============================
  fout << "===================Memory=============================" << endl;
  
  fout << "Reading the first block of the relation into memory block 9:" << endl;
  relation_ptr->getBlock(0,9);
  fout << "Now the memory contains: " << endl;
  fout << mem << endl;
  
  fout << "Copy the memory block 9 to memory block 6-8:" << endl;
  fout << "(You might not need this function)" << endl;
  mem.setBlock(6,*mem.getBlock(9));
  mem.setBlock(7,*mem.getBlock(9));
  mem.setBlock(8,*mem.getBlock(9));
  fout << "Now the memory contains: " << endl;
  fout << mem << endl;

  fout << "Get the tuples in memory block 6-9" << endl;
  fout << "(Can apply sorting and heap building to the tuples later):" << endl;
  tuples=mem.getTuples(6,4);
  copy(tuples.begin(),tuples.end(),ostream_iterator<Tuple,char>(fout,"\n"));
  fout << endl;

  fout << "Write the 'condensed' tuples to memory block 1-2:" << endl;
  mem.setTuples(1,tuples);
  fout << "Now the memory contains: " << endl;
  fout << mem << endl;

  //Error testing
  fout << "Error testing: " << endl;
  mem.getBlock(-1); //out of bound
  mem.getBlock(mem.getMemorySize()); //out of bound
  mem.setBlock(-1,*mem.getBlock(9)); //out of bound
  mem.setBlock(mem.getMemorySize(),*mem.getBlock(9)); //out of bound
  mem.getTuples(-1,4); //out of bound
  mem.getTuples(mem.getMemorySize(),4); //out of bound
  mem.getTuples(0,0); // get 0 block
  mem.getTuples(6,5); // get too many blocks
  mem.setTuples(-1,tuples); //out of bound
  mem.setTuples(mem.getMemorySize(),tuples); //out of bound

  //Store a tuple of ExampleTable2 at memory block 2
  mem.getBlock(2)->clear();
  mem.getBlock(2)->setTuple(0,tuple2);
  mem.getTuples(1,2); //Error: get memory block 1-2, which contain tuples of different tables
  
  fout << "Real elapse time = " << ((double)(clock()-start_time)/CLOCKS_PER_SEC*1000) << " ms" << endl;
  fout << "Calculated elapse time = " << disk.getDiskTimer() << " ms" << endl;
  fout << "Calculated Disk I/Os = " << disk.getDiskIOs() << endl;

  fout.close();
  fout.clear();
  return EXIT_SUCCESS;
}

