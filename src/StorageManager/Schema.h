#ifndef _SCHEMA_H
#define _SCHEMA_H

#include <vector>
#include <map>

#include "Field.h"
#include "Tuple.h"

using namespace std;

/* A schema specifies what a tuple of a partiular relation contains, 
 * including field names, and field types in a defined order. 
 * The field names and types are given offsets according to the defined order.
 * Every schema specifies at most total MAX_NUM_OF_FIELDS_IN_RELATION fields, 
 * MAX_NUM_OF_FIELDS_IN_RELATION = 8. 
 * The size of a tuple is the total number of fields specified in the schema. 
 * The tuple size will affect the number of tuples which can be held in 
 * one disk block or memory block.
 * Usage: Before creating a relation, you have to create a schema object first.
 *        Create a schema by giving field names and field types. (Refer to "Field.h")
 *        Every field name must be unique and non-empty.
 *        Then create a relation through the SchemaManager using the created schema.
 */
class Schema {
  private:
  vector<string> field_names;
  vector<enum FIELD_TYPE> field_types;
  map<string,int> field_offsets; // Maps a field name to a field offset.

  void clear();

  public:
  friend class SchemaManager; // accesses clear()

  Schema();
  Schema(const vector<string>& field_names, const vector<enum FIELD_TYPE>& field_types);

  bool operator==(const Schema& s) const; // test equality
  bool operator!=(const Schema& s) const; // test equality

  bool isEmpty() const; //returns true if it is empty
  bool fieldNameExists(string field_name) const; // returns true if the field exists
  vector<string> getFieldNames() const; //returns the field names in defined order
  vector<enum FIELD_TYPE> getFieldTypes() const; //returns field types in defined order
  string getFieldName(int offset) const; //return empty string if the offset is out of bound
  enum FIELD_TYPE getFieldType(int offset) const; //return empty string if the offset is out of bound
  enum FIELD_TYPE getFieldType(string field_name) const; //return empty string if the field is not found
  int getFieldOffset(string field_name) const; //return -1 if field not found
  int getNumOfFields() const;
  int getTuplesPerBlock() const; //A block storing data of a relation of this schema holds no more
                                 //than this number of tuples/records
  void printSchema() const;
  void printSchema(ostream& out) const;
  void printFieldNames() const; //prints field names in defined order
  void printFieldNames(ostream& out) const;
  friend ostream &operator<<(ostream &out, const Schema &s);
				 
};
#endif
