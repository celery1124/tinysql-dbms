#ifndef _SCHEMA_MANAGER_H
#define _SCHEMA_MANAGER_H

#include <map>
#include <set>
#include <vector>

using namespace std;

/* A schema manager maps a relation name to a relation and a corresponding schema. 
 * You will always create a relation through schema manager by specifying 
 * a relation name and a schema.
 * You will also get access to relations and schemas starting from here.
 * Usage: At the beginning of your program, you need to initialize a schema manager.
 *        Initialize the schema manager by supplying the pointer to memory and to disk
 *        Create a relation through here (and not elsewhere) by giving relation name and schema
 *        Every relation name must be unique.
 *        Once a relation is created, the schema cannot be changed
 */
class MainMemory;
class Disk;
class Relation;
class Schema;

class SchemaManager {
  private:
    MainMemory* mem;
    Disk* disk;
    map<string,int> relation_name_to_index;
    Relation relations[MAX_NUM_CREATING_RELATIONS];
    Schema schemas[MAX_NUM_CREATING_RELATIONS];
    int offset;

  public:
    friend class Tuple; // accesses schema
    friend class Relation; // accesses schema
    
    SchemaManager(MainMemory* mem, Disk* disk);
    Schema getSchema(string relation_name) const; //returns empty schema if the relation is not found
    bool relationExists(string relation_name) const; //returns true if the relation exists
    
    // returns a pointer to the newly allocated relation; the relation name must not exist already
    Relation* createRelation(string relation_name,const Schema& schema);
    Relation* getRelation(string relation_name); //returns NULL if the relation is not found
    bool deleteRelation(string relation_name); //returns false if the relation is not found
    
    void printSchemas() const; //print all relations and their schema
    void printSchemas(ostream &out) const;
    friend ostream &operator<<(ostream &out, const SchemaManager &sm);
};
#endif
