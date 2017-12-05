#include <iostream>
#include <ctime>
#include <climits>
#include <string>
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

Disk::Disk() { resetDiskIOs(); resetDiskTimer(); }

/*
bool Disk::extendTrack(int schema_index, int block_index, const Tuple& t) {
  if (block_index<0) {
    cerr << "extendTrack ERROR: block index " << block_index << " out of disk bound" << endl;
    return false;
  }   
  vector<Block>& track=tracks[schema_index];
  int j=track.size();
  if (block_index>=j) {
    if (j>0) {
      while (!track[j-1].isFull()) { // first fill the last block with invalid tuples
        track[j-1].appendTuple(t);
      }
    }
    // fill the gap with invalid tuples
    for (int i=j;i<block_index;i++) {
      track.push_back(Block::getDummyBlock());
      while (!track[i].isFull()) {
        track[i].appendTuple(t);
      }
    }
  }
  return true;
}
*/

bool Disk::extendTrack(int schema_index, int block_index, const Tuple& t) {
  if (block_index<0) {
    cerr << "extendTrack ERROR: block index " << block_index << " out of disk bound" << endl;
    return false;
  }
  vector<Block>& track=tracks[schema_index];
  int j=track.size();
  if (block_index>j) {
    if (j>0) {
      while (!track[j-1].isFull()) { // first fill the last block with invalid tuples
        track[j-1].appendTuple(t);
      }
    }
    // fill the gap with invalid tuples
    for (int i=j;i<block_index-1;i++) {
      track.push_back(Block::getDummyBlock());
      while (!track[i].isFull()) {
        track[i].appendTuple(t);
      }
    }
    // fill the last block with only one invalid tuple
    track.push_back(Block::getDummyBlock());
    track[block_index-1].appendTuple(t);
  }
  return true;
}

bool Disk::shrinkTrack(int schema_index, int block_index) {
  if (block_index<0 || block_index >= tracks[schema_index].size()) {
    cerr << "shrinkTrack ERROR: block index " << block_index << " out of disk bound" << endl;
    return false;
  }  
  tracks[schema_index].erase(tracks[schema_index].begin()+block_index,tracks[schema_index].end());
  return true;
}

Block Disk::getBlock(int schema_index, int block_index) {
  if (block_index<0 || block_index>=tracks[schema_index].size())  {
    cerr << "getBlock ERROR: block index " << block_index << " out of disk bound" << endl;
    return Block::getDummyBlock();
  }
  incrementDiskIOs(1);
  incrementDiskTimer(1);
  
  return tracks[schema_index][block_index];
}

vector<Block> Disk::getBlocks(int schema_index, int block_index, int num_blocks) {
  if (block_index<0 || block_index>=tracks[schema_index].size())  {
    cerr << "getBlocks ERROR: block index " << block_index << " out of disk bound" << endl;
    return vector<Block>();
  }
  int i;
  if ((i=block_index+num_blocks-1)>=tracks[schema_index].size()) {
    cerr << "getBlocks ERROR: num of blocks out of disk bound: " << i << endl;
    return vector<Block>();
  }
  incrementDiskIOs(num_blocks);
  incrementDiskTimer(num_blocks);

  vector<Block> v(tracks[schema_index].begin()+block_index,
                  tracks[schema_index].begin()+block_index+num_blocks);
  return v;
}

bool Disk::setBlock(int schema_index, int block_index, const Block& b) {
  if (block_index<0)  {
    cerr << "setBlock ERROR: block index " << block_index << " out of disk bound" << endl;
    return false;
  }
  incrementDiskIOs(1);
  incrementDiskTimer(1);
  tracks[schema_index][block_index]=b;
  return true;
}

bool Disk::setBlocks(int schema_index, int block_index, const vector<Block>& vb) {
  if (block_index<0)  {
    cerr << "setBlocks ERROR: block index " << block_index << " out of disk bound" << endl;
    return false;
  }
  incrementDiskIOs(vb.size());
  incrementDiskTimer(vb.size());
  copy< vector<Block>::const_iterator, vector<Block>::iterator >
    (vb.begin(),vb.end(),tracks[schema_index].begin()+block_index);
  return true;
}

void Disk::incrementDiskIOs(int count) {
  if (DISK_I_O_DEBUG)
    cerr << "DEBUG: Disk I/O is incremented by " << count << endl;
  diskIOs+=count;
}

void Disk::incrementDiskTimer(int num_blocks) {
  if (SIMULATED_DISK_LATENCY_ON==1) {
    clock_t start_time;
    start_time=clock();
    clock_t delay=(clock_t)(avg_seek_time+avg_rotation_latency+avg_transfer_time_per_block*num_blocks)
                  *CLOCKS_PER_SEC/1000;
    while (clock()-start_time < delay){
    ;
    }
  }

  timer+=avg_seek_time+avg_rotation_latency+avg_transfer_time_per_block*num_blocks;
}

void Disk::resetDiskIOs() {
  diskIOs=0;
}

unsigned long int Disk::getDiskIOs() const {
  return diskIOs;
}

void Disk::resetDiskTimer() {
  timer=0;
}

double Disk::getDiskTimer() const {
  return timer;
}
    
Schema::Schema() {}

Schema::Schema(const vector<string>& field_names, const vector<enum FIELD_TYPE>& field_types){
  if(field_names.size()!=field_types.size()){
    cerr<<"Schema ERROR: size of field_names and size of field_types do not match"<<endl;
    return;
  }
  if (field_names.size()==0) {
    cerr<<"Schema ERROR: empty fields"<<endl;
    return;
  } else if (field_names.size()>MAX_NUM_OF_FIELDS_IN_RELATION){
    cerr<<"Schema ERROR: at most "<<MAX_NUM_OF_FIELDS_IN_RELATION<<" fields are allowed"<<endl;
    return;
  }
  for (int i=0;i<field_names.size()-1;i++) {
    if (field_names[i]=="") {
      cerr<<"Schema ERROR: empty field name at offset " << i << endl;
      return;
    }
    for (int j=i+1;j<field_names.size();j++) {
      if (field_names[i]==field_names[j]) {
	cerr<<"Schema ERROR: repeated field names " << field_names[i]
        << " at offset " << i << " and " << j << endl;
	return;
      }
    }
  }
  if (field_names[field_names.size()-1]=="") {
    cerr<<"Schema ERROR: empty field name at offset " << (field_names.size()-1) << endl;
    return;
  }
  this->field_names = field_names;
  this->field_types = field_types;
  for(int i=0;i<field_names.size();i++){
     field_offsets[field_names[i]] = i;
     if (field_types[i]!=INT && field_types[i]!=STR20) {
       cerr<<"Schema ERROR: "<<field_types[i]<<" is not supported"<<endl;
       clear();
       return;
     }
  }
}

bool Schema::operator==(const Schema& s) const {
  return (field_names==s.field_names && field_types==s.field_types && field_offsets==s.field_offsets);
}

bool Schema::operator!=(const Schema& s) const {
  return (field_names!=s.field_names || field_types!=s.field_types || field_offsets!=s.field_offsets);
}

bool Schema::isEmpty() const {
  if (field_names.empty() || field_types.empty() || field_offsets.empty()) return true;
  return false;
}

bool Schema::fieldNameExists(string field_name) const {
  map<string,int >::const_iterator mit;
  if ((mit=field_offsets.find(field_name))==field_offsets.end()) {
    return false;
  }
  return true;
}

void Schema::clear() {
  field_offsets.clear();
  this->field_names.clear();
  this->field_types.clear();
}

//returns the field names in defined order
vector<string> Schema::getFieldNames() const {
  return field_names;
}

//returns field types in defined order
vector<enum FIELD_TYPE> Schema::getFieldTypes() const {
  return field_types;  
}

//returns the field name at the offset
string Schema::getFieldName(int offset) const {
  if (offset<0 || offset>=getNumOfFields()) {
    cerr<<"getFieldName ERROR: offset " << offset << " out of bound"<<endl;
    return "";
  }
  return field_names[offset];
}

//returns the field type at the offset
enum FIELD_TYPE Schema::getFieldType(int offset) const {
  if (offset<0 || offset>=getNumOfFields()) {
    cerr<<"getFieldType ERROR: offset " << offset << " out of bound"<<endl;
    return FIELD_TYPE();
  }  
  return field_types[offset];
}

//returns the field type corresponding to the field name
enum FIELD_TYPE Schema::getFieldType(string field_name) const {
  map<string,int >::const_iterator mit;
  if ((mit=field_offsets.find(field_name))==field_offsets.end()) {
    cerr<<"getFieldOffset ERROR: field name "<<field_name<<" is not found"<<endl;
    return FIELD_TYPE();
  }
  return field_types[mit->second];
}

int Schema::getFieldOffset(string field_name) const {
  map<string,int >::const_iterator mit;
  if ((mit=field_offsets.find(field_name))==field_offsets.end()) {
    cerr<<"getFieldOffset ERROR: field name "<<field_name<<" is not found"<<endl;
    return -1;
  }
  return mit->second;
}

int Schema::getNumOfFields() const {
  return field_names.size();
}

int Schema::getTuplesPerBlock() const {
  return FIELDS_PER_BLOCK/field_names.size();
}

void Schema::printSchema() const {
  printSchema(cout);
  cout << endl;
}

void Schema::printSchema(ostream &out) const {
  if (field_names.size()>0) {
    out << field_names[0] << " " << (field_types[0]==0?"INT":"STR20") << ";";
    for (int i=1;i<field_names.size();i++) {
      out << endl << field_names[i] << " " << (field_types[i]==0?"INT":"STR20") << ";";
    }
  }
}

void Schema::printFieldNames() const {
  printFieldNames(cout);
  cout << endl;
}

void Schema::printFieldNames(ostream &out) const {
  for (int i=0;i<field_names.size();i++) {
    out << field_names[i] << "\t";
  }
}

ostream &operator<<(ostream &out, const Schema &s) {
  s.printSchema(out);
  return out;
}

Tuple::Tuple(SchemaManager* schema_manager, int schema_index){
  this->schema_manager=schema_manager;
  this->schema_index=schema_index;
  if (this->schema_manager!=NULL) {
    Schema& schema=schema_manager->schemas[schema_index];
    int numberOfFields=schema.getNumOfFields();
    for (int i=0;i<numberOfFields;i++) {
      fields.push_back(Field());
    }
  }
}

Tuple Tuple::getDummyTuple() {
  return Tuple(NULL,-1);
}

bool Tuple::isNull() const {
  return fields.size()==0;
}

Schema Tuple::getSchema() const {
  return schema_manager->schemas[schema_index];
}

int Tuple::getNumOfFields() const {
  Schema& schema=schema_manager->schemas[schema_index];
  return schema.getNumOfFields();
}

int Tuple::getTuplesPerBlock() const {
  Schema& schema=schema_manager->schemas[schema_index];
  return schema.getTuplesPerBlock();
}

void Tuple::null() {
  fields.clear();
}

bool Tuple::setField(int offset,string s){
  Schema& schema=schema_manager->schemas[schema_index];
  if (offset>=schema.getNumOfFields() || offset<0){
    cerr<<"setField ERROR: offset "<<offset<<" is out of bound!"<<endl;
    return false;
  } else if (schema.getFieldType(offset)!=STR20) {
    cerr<<"setField ERROR: field type not STR20!"<<endl;
    return false;
  } else {
    fields[offset].str=new string(s);
  }
  return true;
}

bool Tuple::setField(int offset,int i){
  Schema& schema=schema_manager->schemas[schema_index];
  if (offset>=schema.getNumOfFields() || offset<0){
    cerr<<"setField ERROR: offset "<<offset<<" is out of bound!"<<endl;
    return false;
  } else if (schema.getFieldType(offset)!=INT) {
    cerr<<"setField ERROR: field type not INT!"<<endl;
    return false;
  } else {
    fields[offset].integer=i;
  }
  return true;
}

bool Tuple::setField(string field_name,string s){
  Schema& schema=schema_manager->schemas[schema_index];
  if (!schema.fieldNameExists(field_name)) {
    cerr<<"setField ERROR: field name " << field_name << " not found"<<endl;
    return false;
  }
  int offset=schema.getFieldOffset(field_name);
  if (schema.getFieldType(offset)!=STR20) {
    cerr<<"setField ERROR: field type not STR20!"<<endl;
    return false;
  } else {
    fields[offset].str=new string(s);
  }
  return true;
}

bool Tuple::setField(string field_name,int i){
  Schema& schema=schema_manager->schemas[schema_index];
  if (!schema.fieldNameExists(field_name)) {
    cerr<<"setField ERROR: field name " << field_name << " not found"<<endl;
    return false;
  }
  int offset=schema.getFieldOffset(field_name);
  if (schema.getFieldType(offset)!=INT) {
    cerr<<"setField ERROR: field type not INT!"<<endl;
    return false;
  } else {
    fields[offset].integer=i;
  }
  return true;
}

union Field Tuple::getField(int offset) const{
  if(offset<fields.size() && offset>=0){
    return fields[offset];
  } else {
    cerr<<"getField ERROR: offset "<<offset<<" is out of bound!"<<endl;
    return Field();
  }
}

union Field Tuple::getField(string field_name) const{
  Schema& schema=schema_manager->schemas[schema_index];
  int offset=schema.getFieldOffset(field_name);
  if(offset<fields.size() && offset>=0){
    return fields[offset];
  } else {
    cerr<<"getField ERROR: offset "<<offset<<" is out of bound!"<<endl;
    return Field();
  }
}

void Tuple::printTuple() const{
  printTuple(false);
}

void Tuple::printTuple(ostream &out) const{
  printTuple(false, out);
}

void Tuple::printTuple(bool print_field_names) const {
  printTuple(print_field_names,cout);
  cout << endl;
}

void Tuple::printTuple(bool print_field_names, ostream &out) const {
  Schema& schema=schema_manager->schemas[schema_index];
  if (print_field_names) {
    schema.printFieldNames(out);
    out << endl;
  }
  for (int i=0;i<fields.size();i++) {
    if (schema.getFieldType(i)==INT)
      out << fields[i].integer << "\t";
    else
      out << *(fields[i].str) << "\t";
  }
}

ostream &operator<<(ostream &out, const Tuple &t) {
  t.printTuple(out);
  return out;
}

Block::Block() {}

Block Block::getDummyBlock() {
  return Block();
}

bool Block::isFull() const {
  if (tuples.empty()) return false;
  if (tuples.size()==tuples.front().getTuplesPerBlock()) return true;
  return false;
}

bool Block::isEmpty() const {
  return tuples.empty();
}

void Block::clear() {
  tuples.clear();
}

int Block::getNumTuples() const {
  int count=0;
  for (vector<Tuple>::const_iterator it=tuples.begin();
       it!=tuples.end();it++) {
    if (!it->isNull()) count++;
  }
  return count;
}

Tuple Block::getTuple(int tuple_offset) const { // gets the tuple value at tuple_index; returns empty Tuple if tuple_index out of bound
  if (!tuples.empty() && tuple_offset>=tuples.front().getTuplesPerBlock()) {
    cerr << "getTuple ERROR: tuple offet " << tuple_offset << " out of bound of the block" << endl;
    return Tuple::getDummyTuple();
  }
  if (tuple_offset<0 || tuple_offset>=tuples.size()) {
    cerr << "getTuple ERROR: tuple offet " << tuple_offset << " out of bound" << endl;
    return Tuple::getDummyTuple();
  }
  return tuples[tuple_offset];
}

vector<Tuple> Block::getTuples() const {
  return tuples;
}

bool Block::setTuple(int tuple_offset, const Tuple& tuple) { // sets new tuple value at tuple_index; returns false if tuple_index out of bound
  Schema s = tuple.getSchema();
  if (!tuples.empty()) {
    if (tuple_offset>=tuples.front().getTuplesPerBlock()) {
      cerr << "setTuple ERROR: tuple offet " << tuple_offset << " out of bound of the block" << endl;
      return false;
    }
    for (int i=0;i<tuples.size();i++) {
      if (s!=tuples[i].getSchema()) {
        cerr << "setTuple ERROR: tuples' schemas do not match" << endl;
        return false;
      }
    }
  }
  if (tuple_offset<0 || tuple_offset>=s.getTuplesPerBlock()) {
    cerr << "setTuple ERROR: tuple offet " << tuple_offset << " out of bound" << endl;
    return false;
  }
  if (tuple_offset >= tuples.size()) {
    //If there is a gap before the offset, filled it with invalid tuples
    Tuple t(tuple.schema_manager,tuple.schema_index);
    t.null();
    for (int i=tuples.size();i<tuple_offset;i++) {
      tuples.push_back(t);      
    }
    tuples.push_back(tuple);
  } else
    tuples[tuple_offset]=tuple;
  return true;
}

bool Block::setTuples(const vector<Tuple>& tuples) {
  if (tuples.size()>tuples.front().getTuplesPerBlock()) {
    cerr << "setTuples ERROR: number of tuples exceed space limit of the block" << endl;
    return false;
  }
  this->tuples.assign(tuples.begin(),tuples.end());
  return true;
}

bool Block::setTuples(const vector<Tuple>::const_iterator first, const vector<Tuple>::const_iterator last) {
  if (last-first > first->getTuplesPerBlock()) {
    cerr << "setTuples ERROR: number of tuples exceed space limit of the block" << endl;
    return false;
  }
  this->tuples.assign(first,last);
  return true;
}

bool Block::appendTuple(const Tuple& tuple) {
  if (isFull()) {
    cerr << "appendTuple ERROR: the block is full" << endl;
    return false;
  }
  this->tuples.push_back(tuple);
  return true;
}

bool Block::nullTuple(int tuple_offset) { // empty the tuple at the offset
  if (tuple_offset<0 || tuple_offset>=tuples.size()) {
    cerr << "nullTuple ERROR: tuple offet " << tuple_offset << " out of bound" << endl;
    return false;
  }
  tuples[tuple_offset].null();
  return true;
}

bool Block::nullTuples() { // empty all the tuples in the block
  for (int i=0;i<tuples.size();i++) {
    tuples[i].null();
  }
  return true;
}

void Block::printBlock() const {
  printBlock(cout);
  cout << endl;
}

void Block::printBlock(ostream &out) const {
  if (tuples.empty()) return;
  vector<Tuple>::const_iterator lit=tuples.begin();
  if (lit->isNull())
    out << "(hole)";
  else {
    lit->printTuple(out);
  }
  lit++;
  for (;lit!=tuples.end();lit++) {
    out << endl;
    if (lit->isNull())
      out << "(hole)";
    else
      lit->printTuple(out);
  }  
}

ostream &operator<<( ostream &out, const Block &b ) {
  b.printBlock(out);
  return out;
}

Relation::Relation() {
  this->schema_manager=NULL;
  this->schema_index=-1;
  this->relation_name="";
  this->mem=NULL;
  this->disk=NULL;
}

Relation::Relation(SchemaManager* schema_manager, int schema_index, string relation_name,
                   MainMemory* mem, Disk* disk) {
  this->schema_manager=schema_manager;
  this->schema_index=schema_index;
  this->relation_name=relation_name;
  this->mem=mem;
  this->disk=disk;
}


void Relation::null() {
  //data.clear();
  this->schema_manager=NULL;
  this->schema_index=-1;
  this->relation_name="";
  this->mem=NULL;  
}

string Relation::getRelationName() const {
  return relation_name;
}

Schema Relation::getSchema() const {
  return schema_manager->schemas[schema_index];
}

//NOTE: Because the operation should not have disk latency,
//      it is implemented in Relation instead of in Disk
int Relation::getNumOfBlocks() const {
  vector<Block>& data=disk->tracks[schema_index];
  return data.size();
}

// returns actual number of tuples in the relation
//NOTE: Because the operation should not have disk latency,
//      it is implemented in Relation instead of in Disk
int Relation::getNumOfTuples() const {
  vector<Block>& data=disk->tracks[schema_index];
  int total_tuples=0;
  for (vector<Block>::const_iterator vit=data.begin();vit!=data.end();vit++) {
    total_tuples+=vit->getNumTuples();
  }
  return total_tuples;
}

bool Relation::isNull() const {
  return (schema_manager==NULL || schema_index==-1 || mem==NULL);
}

Tuple Relation::createTuple() const {
  return Tuple(schema_manager,schema_index);
}

bool Relation::getBlock(int relation_block_index, int memory_block_index) const {
  //delay();
  //DIOs++;
  if (memory_block_index<0 || memory_block_index>=NUM_OF_BLOCKS_IN_MEMORY) {
    cerr << "getBlock ERROR: block index " << memory_block_index << " out of bound in memory" << endl;
    return false;
  }
  /*
  if (relation_block_index<0 || relation_block_index>=data.size()) {
    cerr << "getBlock ERROR: block index " << relation_block_index << " out of bound in relation" << endl;
    return false;
  }
  */
  //mem->setBlock(memory_block_index,data[relation_block_index]);
  Block b = disk->getBlock(schema_index,relation_block_index);
  if (!b.isEmpty()) {
    mem->setBlock(memory_block_index,b);
    return true;
  }
  return false;
}

bool Relation::getBlocks(int relation_block_index, int memory_block_index, int num_blocks) const {
  //delay();
  //DIOs+=num_blocks;
  if (num_blocks<=0) {
    cerr << "getBlocks ERROR: num of blocks " << num_blocks << " too few" << endl;
    return false;
  }
  if (memory_block_index<0 || memory_block_index>=NUM_OF_BLOCKS_IN_MEMORY) {
    cerr << "getBlocks ERROR: block index " << memory_block_index << " out of bound in memory" << endl;
    return false;
  }
  int i;
  if ((i=memory_block_index+num_blocks-1)>=NUM_OF_BLOCKS_IN_MEMORY) {
    cerr << "getBlocks ERROR: access to block out of memory bound: " << i << endl;
    return false;
  }
  /*
  if (relation_block_index<0 || relation_block_index>=data.size()) {
    cerr << "getBlocks ERROR: block index " << relation_block_index << " out of bound in relation" << endl;
    return false;
  }
  if ((i=relation_block_index+num_blocks-1)>=data.size()) {
    cerr << "getBlocks ERROR: num of blocks out of relation bound: " << i << endl;
    return false;
  }
  mem->setBlock(memory_block_index,data.begin()+relation_block_index,
                data.begin()+relation_block_index+num_blocks);
  */
  vector<Block> v=disk->getBlocks(schema_index,relation_block_index,num_blocks);
  mem->setBlock(memory_block_index,v.begin(),v.end());
  return true;  
}

bool Relation::setBlock(int relation_block_index, int memory_block_index) {
  //delay();
  //DIOs++;
  if (memory_block_index<0 || memory_block_index>=NUM_OF_BLOCKS_IN_MEMORY) {
    cerr << "setBlock ERROR: block index" << memory_block_index << " out of bound in memory" << endl;
    return false;
  }
  if (relation_block_index<0) {
    cerr << "setBlock ERROR: block index " << relation_block_index << " out of bound in relation" << endl;
    return false;
  }
  // check if the schema is correct
  vector<Tuple> v = mem->getBlock(memory_block_index)->getTuples();
  Schema s = getSchema();
  for (int i=0;i<v.size();i++) {
    if (v[i].getSchema() != s) {
      cerr << "setBlock ERROR: The tuple at offest " << i << " of memory block "
           << memory_block_index << " has a different schema." << endl;
      return false;
    }
  }

  //data[relation_block_index]=*(mem->getBlock(memory_block_index));

  Tuple t(schema_manager,schema_index);
  t.null(); //invalidates the tuple
  if (disk->extendTrack(schema_index,relation_block_index+1,t)) {
    //Actual writing on disk
    return disk->setBlock(schema_index,relation_block_index,*(mem->getBlock(memory_block_index)));
  }
  return false;
}

bool Relation::setBlocks(int relation_block_index, int memory_block_index, int num_blocks) {
  //delay();
  //DIOs+=num_blocks;
  if (num_blocks<=0) {
    cerr << "setBlocks ERROR: num of blocks " << num_blocks << " too few" << endl;
    return false;
  }
  if (memory_block_index<0 || memory_block_index>=NUM_OF_BLOCKS_IN_MEMORY) {
    cerr << "setBlocks ERROR: block index " << memory_block_index << " out of bound in memory" << endl;
    return false;
  }
  int i;
  if ((i=memory_block_index+num_blocks-1)>=NUM_OF_BLOCKS_IN_MEMORY) {
    cerr << "setBlocks ERROR: access to block out of memory bound: " << i << endl;
    return false;
  }
  if (relation_block_index<0) {
    cerr << "setBlocks ERROR: block index " << relation_block_index << " out of bound in relation" << endl;
    return false;
  }

  vector<Block> vb;
  Schema s = getSchema();
  int j,k;
  //for (i=relation_block_index,j=memory_block_index;i<relation_block_index+num_blocks;i++,j++) {
  for (j=memory_block_index;j<memory_block_index+num_blocks;j++) {
    // check if the schema is correct
    vector<Tuple> v = mem->getBlock(j)->getTuples();
    for (k=0;k<v.size();k++) {
      if (v[k].getSchema() != s) {
        cerr << "setBlocks ERROR: The tuple at offest " << k << " of memory block "
        << j << " has a different schema." << endl;
        return false;
      }
    }
    //data[i]=*(mem->getBlock(j));
    vb.push_back(*(mem->getBlock(j)));
  }

  Tuple t(schema_manager,schema_index);
  t.null(); //invalidates the tuple
  if (disk->extendTrack(schema_index,relation_block_index+num_blocks,t)) {
    //Actual writing on disk
    return disk->setBlocks(schema_index,relation_block_index,vb);
  }
  return false;
}

//delete the block from [starting_block_index] to the last block
bool Relation::deleteBlocks(int starting_block_index) {
  return disk->shrinkTrack(schema_index,starting_block_index);
}

void Relation::printRelation() const {
  printRelation(cout);
  cout << endl;
}

//NOTE: Because the operation should not have disk latency,
//      it is implemented in Relation instead of in Disk
void Relation::printRelation(ostream &out) const {
  vector<Block>& data=disk->tracks[schema_index];
  int i=0;
  out << "******RELATION DUMP BEGIN******" << endl;
  schema_manager->schemas[schema_index].printFieldNames(out);
  out << endl;
  for (vector<Block>::const_iterator vit=data.begin();vit!=data.end();vit++) {
    out << i << ": ";
    vit->printBlock(out);
    out << endl;
    i++;
  }
  out << "******RELATION DUMP END******";
}

ostream &operator<<(ostream &out, const Relation &r) {
  r.printRelation(out);
  return out;
}

MainMemory::MainMemory() { }

bool MainMemory::setBlock(int memory_block_index, const vector<Block>::const_iterator first,
              const vector<Block>::const_iterator last) {
  if (memory_block_index<0 || memory_block_index>=NUM_OF_BLOCKS_IN_MEMORY) {
    cerr << "setBlock ERROR: block index " << memory_block_index << " out of memory bound" << endl;
    return false;
  }
  int i=memory_block_index;
  for (vector<Block>::const_iterator it=first;it!=last;it++) {
    if (i>=NUM_OF_BLOCKS_IN_MEMORY) {
      cerr << "setBlock ERROR: number of blocks reaches memory boundary" << endl;
      return false;
    }
    blocks[i] = *it;
    i++;
  }
  return true;
}
              
int MainMemory::getMemorySize() const { //returns max number of blocks
  return NUM_OF_BLOCKS_IN_MEMORY;
}

Block* MainMemory::getBlock(int memory_block_index) {
  if (memory_block_index<0 || memory_block_index>=NUM_OF_BLOCKS_IN_MEMORY) {
    cerr << "getBlock ERROR: block index " << memory_block_index << " out of memory bound" << endl;
    return NULL;
  }
  return blocks+memory_block_index;
}

bool MainMemory::setBlock(int memory_block_index, const Block& b) {
  if (memory_block_index<0 || memory_block_index>=NUM_OF_BLOCKS_IN_MEMORY) {
    cerr << "setBlock ERROR: block index " << memory_block_index << " out of memory bound" << endl;
    return false;
  }  
  blocks[memory_block_index] = b;
  return true;
}

vector<Tuple> MainMemory::getTuples(int memory_block_begin,int num_blocks) const { //gets tuples from a range of blocks
  if (memory_block_begin<0 || memory_block_begin>=NUM_OF_BLOCKS_IN_MEMORY) {
    cerr << "getTuples ERROR: block index " << memory_block_begin << " out of memory bound" << endl;
    return vector<Tuple>();
  }
  if (num_blocks<=0) {
    cerr << "getTuples ERROR: num of blocks " << num_blocks << " too few" << endl;
    return vector<Tuple>();     
  }
  int i;
  if ((i=memory_block_begin+num_blocks-1)>=NUM_OF_BLOCKS_IN_MEMORY ) {
    cerr << "getTuples ERROR: access to block out of memory bound: " << i << endl;
    return vector<Tuple>();    
  }
  vector<Tuple> tuples;
  Schema s = blocks[memory_block_begin].getTuples()[0].getSchema();
  for (int i=memory_block_begin;i<memory_block_begin+num_blocks;i++) {
    vector<Tuple> tuples2=blocks[i].getTuples();
    if (tuples2[0].getSchema() != s) {
      cerr << "getTuples ERROR: schema at memory block " << i << " has a different schema" << endl;
      return vector<Tuple>();
    }
    // Only valid tuples are returned
    for (vector<Tuple>::const_iterator it=tuples2.begin();it!=tuples2.end();it++) {
      if (!it->isNull()) tuples.push_back(*it);
    }
  }
  return tuples;
}

//writes tuples consecutively starting from a particular memory block;
//returns false if out of bound in memory
bool MainMemory::setTuples(int memory_block_begin,const vector<Tuple>& tuples) {
  if (memory_block_begin<0 || memory_block_begin>=NUM_OF_BLOCKS_IN_MEMORY) {
    cerr << "setTuples ERROR: block index " << memory_block_begin << " out of memory bound" << endl;
    return false;
  }
  int tuples_per_block=tuples.front().getTuplesPerBlock();
  int num_blocks=tuples.size()/tuples_per_block;
  int num_additional_blocks=(tuples.size()%tuples_per_block>0?1:0);
  if (memory_block_begin + num_blocks + num_additional_blocks >
       NUM_OF_BLOCKS_IN_MEMORY) {
    cerr << "setTuples ERROR: number of tuples exceed the memory space" << endl;
    return false;
  }
  vector<Tuple>::const_iterator lit=tuples.begin(),lit2=tuples.begin();
  int i,j;
  for (i=memory_block_begin;i<memory_block_begin + num_blocks;i++) {
    for (j=0;j<tuples_per_block;j++,lit2++);
    //blocks[i].tuples.assign(lit,lit2);
    blocks[i].setTuples(lit,lit2);
    lit=lit2;
  }
  if (num_additional_blocks==1) {
    //blocks[i].tuples.assign(lit,tuples.end());
    blocks[i].setTuples(lit,tuples.end());
  }
  return true;
}

void MainMemory::dumpMemory() const {
  dumpMemory(cout);
  cout << endl;
}

void MainMemory::dumpMemory(ostream &out) const {
  out << "******MEMORY DUMP BEGIN******" << endl;
  for (int i=0;i<NUM_OF_BLOCKS_IN_MEMORY;i++) {
    out << i << ": ";
    blocks[i].printBlock(out);
    out << endl;
  }
  out << "******MEMORY DUMP END******";
}

ostream &operator<<(ostream &out, const MainMemory &m) {
  m.dumpMemory(out);
  return out;
}

SchemaManager::SchemaManager(MainMemory* mem, Disk* disk) {
  this->mem=mem;
  this->disk=disk;
  offset=0;
}

Schema SchemaManager::getSchema(string relation_name) const {
  map<string,int>::const_iterator it=relation_name_to_index.find(relation_name);
  if (it==relation_name_to_index.end()) {
    cerr << "getSchema ERROR: relation " << relation_name << " does not exist" << endl;
    return Schema();
  } else {
    return schemas[it->second];
  }
}

bool SchemaManager::relationExists(string relation_name) const {
  map<string,int>::const_iterator it=relation_name_to_index.find(relation_name);
  if (it==relation_name_to_index.end())
    return false;
  return true;
}

Relation* SchemaManager::createRelation(string relation_name,const Schema& schema){
  if (relation_name=="") {
    cerr << "createRelation ERROR: empty relation name" << endl;
    return NULL;
  }
  map<string,int>::iterator it=relation_name_to_index.find(relation_name);
  if (it!=relation_name_to_index.end()) {
    cerr << "createRelation ERROR: " << relation_name << " already exists" << endl;
    return NULL;
  }
  if (schema.isEmpty()) {
    cerr << "createRelation ERROR: empty schema" << endl;
    return NULL;
  }
  if (offset==MAX_NUM_CREATING_RELATIONS) {
    cerr << "createRelation ERROR: no more relations can be created." << endl;
    return NULL;
  }
  relation_name_to_index[relation_name]=offset;
  relations[offset]=Relation(this,offset,relation_name,mem,disk);
  schemas[offset]=schema;
  offset++; // increase the boundary
  return &relations[offset-1];
}

Relation* SchemaManager::getRelation(string relation_name) {
  map<string,int>::iterator it=relation_name_to_index.find(relation_name);
  if(it==relation_name_to_index.end()){
    cerr << "getRelation ERROR: relation " << relation_name << " does not exist" << endl;
    return NULL;
  } else {
    return &relations[it->second];
  }
}

bool SchemaManager::deleteRelation(string relation_name) {
  map<string,int>::iterator it;
  if ((it=relation_name_to_index.find(relation_name))==relation_name_to_index.end()) {
    cerr << "deleteRelation ERROR: relation " << relation_name << " does not exist" << endl;
    return false;
  }
  int offset=it->second;
  relations[offset].null();
  schemas[offset].clear();
  relation_name_to_index.erase(it);
  return true;
}

void SchemaManager::printSchemas() const {
  printSchemas(cout);
}

void SchemaManager::printSchemas(ostream &out) const {
  if (offset>0) {
    map<string,int>::const_iterator it;
    int i;
    
    for (i=0;i<offset;i++) {
      if (!relations[i].isNull()) {
        out << relations[i].getRelationName() << endl;
        schemas[i].printSchema(out);
        break;
      }
    }
    for (i++;i<offset;i++) {
      if (!relations[i].isNull()) {
        out << endl;
        out << relations[i].getRelationName() << endl;
        schemas[i].printSchema(out);
      }
    }
  }
}

ostream &operator<<(ostream &out, const SchemaManager &sm) {
  sm.printSchemas(out);
  return out;
}
