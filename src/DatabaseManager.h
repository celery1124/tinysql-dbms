/* DatabaseManager.h
* CSCE 608 - 600 Fall 2017
* by Mian Qin
*/

#ifndef   _database_manager_h   
#define   _database_manager_h

#include "Common.h"
#include "Parser.h"
#include "ConditionParser.h"

class ListBlock
{
private:
	int list_id;
	int block_id;
	int num_blocks;
	std::queue<Tuple> queue;
	Relation* relation;
	MainMemory* memory;
	
public:
	ListBlock()
	{
		block_id = 0;
	}
	void init(MainMemory* mem, Relation* rel, int lid, int nb)
	{
		relation = rel;
		memory = mem;
		list_id = lid;
		block_id = 0;
		num_blocks = nb;
	}

	Tuple GetMinTuple(int mem_blk_id)
	{
		if (queue.empty())
		{
			relation->getBlock(list_id*TOTAL_MEM_BLKS + block_id, mem_blk_id);
			block_id++;
			Block* blk_ptr = memory->getBlock(mem_blk_id);
			vector<Tuple> tuples = blk_ptr->getTuples();
			for (int i = 0; i < tuples.size(); i++)
			{
				queue.push(tuples[i]);
			}
		}
		return queue.front();
	}
	bool Empty()
	{
		return queue.empty() && block_id == num_blocks;
	}
	void Pop()
	{
		queue.pop();
	}
};

class DatabaseManager {
private:
	MainMemory* memory;
	Disk* disk;
	SchemaManager* schemaMng;

	/******* Utility *******/
	void PrintStats()
	{

		cout << "********** Statistics **********" << endl;
		cout << "*" << setiosflags(ios::left) << setw(20) << "  Disk I/O: ";
		cout << resetiosflags(ios::adjustfield);
		cout << setiosflags(ios::left) << setw(10) << disk->getDiskIOs();
		cout << resetiosflags(ios::adjustfield);
		cout << setiosflags(ios::right) << setw(1) << "*" << endl;
		cout << resetiosflags(ios::adjustfield);
		cout << "*" << setiosflags(ios::left) << setw(20) << "  Execution Time: ";
		cout << resetiosflags(ios::adjustfield);
		cout << setiosflags(ios::left) << setw(10) << disk->getDiskTimer();
		cout << resetiosflags(ios::adjustfield);
		cout << setiosflags(ios::right) << setw(1) << "*" << endl;
		cout << resetiosflags(ios::adjustfield);
		cout << "********************************" << endl;
	}
	int GetColWidth(vector<string> attr_list)
	{
		int width = 0;
		for (int i = 0; i < attr_list.size(); i++)
		{
			width = attr_list[i].size() > width ? attr_list[i].size() : width;
		}
		return width + 1;
	}
	void PrintBar(int NumCol, int ColWidth)
	{
		for (int i = 0; i < NumCol; i++)
		{
			for (int j=0; j<ColWidth; j++)
				cout << "-";
		}
		cout << endl;
	}
	void PrintAttr(vector<string>& attr_list)
	{
		int colwidth = GetColWidth(attr_list);
		PrintBar(attr_list.size(), colwidth);
		for (int i = 0; i < attr_list.size(); i++)
		{
			cout << setiosflags(ios::left) << setw(colwidth) << attr_list[i];
		}
		cout << endl;
		PrintBar(attr_list.size(), colwidth);
	}
	void PrintAttr(string& table_name, vector<string>& attr_list)
	{
		Relation* relation = schemaMng->getRelation(table_name);
		Schema schema = relation->getSchema();
		vector<string> AttrNames = schema.getFieldNames();
		
		int colwidth = GetColWidth(attr_list);
		PrintBar(attr_list.size(), colwidth);
		vector<string>::iterator it;
		for (int i = 0; i < AttrNames.size();i++)
		{
			it = find(attr_list.begin(), attr_list.end(), AttrNames[i]);
			if (it != attr_list.end())
				cout << setiosflags(ios::left) << setw(colwidth) << AttrNames[i] ;
		}
		cout << endl;
		PrintBar(attr_list.size(), colwidth);
	}

	void PrintTuple(Tuple& t)
	{
		Schema schema = t.getSchema();
		vector<string> AttrNames = schema.getFieldNames();
		int colwidth = GetColWidth(AttrNames);

		for (int i = 0; i < t.getNumOfFields(); i++)
		{
			if (schema.getFieldType(i) == INT)
			{
				if(t.getField(i).integer == INT_MIN)
					cout << setiosflags(ios::left) << setw(colwidth) << "NULL";
				else
					cout << setiosflags(ios::left) << setw(colwidth) << t.getField(i).integer;
			}
			else
				cout << setiosflags(ios::left) << setw(colwidth) << *(t.getField(i).str);
		}
		cout << endl;
	}

	void PrintTuple(Tuple& t, vector<string>& attr_list)
	{
		Schema schema = t.getSchema();
		vector<string> AttrNames = schema.getFieldNames();
		int colwidth = GetColWidth(AttrNames);

		vector<string>::iterator it;
		for (int i = 0; i < t.getNumOfFields(); i++)
		{
			it = find(attr_list.begin(), attr_list.end(), AttrNames[i]);
			if (it != attr_list.end())
			{
				if (schema.getFieldType(i) == INT)
				{
					if (t.getField(i).integer == INT_MIN)
						cout << setiosflags(ios::left) << setw(colwidth) << "NULL";
					else
						cout << setiosflags(ios::left) << setw(colwidth) << t.getField(i).integer;
				}
				else
					cout << setiosflags(ios::left) << setw(colwidth) << *(t.getField(i).str) ;
			}
		}
		cout << endl;
	}

	bool TupleCompareSmaller(Tuple& t1, Tuple& t2, vector<string>& attr_list1, vector<string>& attr_list2)
	{
		Schema schema1 = t1.getSchema();
		Schema schema2 = t2.getSchema();
		// t1 t2 same tuple field
		if (attr_list1.size() == 1)
		{
			if (schema1.getFieldType(attr_list1[0]) == INT)
			{
				return t1.getField(attr_list1[0]).integer <= t2.getField(attr_list2[0]).integer;
			}
			else
			{
				return *(t1.getField(attr_list1[0]).str) <= *(t2.getField(attr_list2[0]).str);
			}
		}
		// t1 t2 different field
		else
		{
			string cmp1;
			string cmp2;
			for (int i = 0; i < attr_list1.size(); i++)
			{
				if (schema1.getFieldType(attr_list1[0]) == INT)
				{
					cmp1 += to_string(t1.getField(attr_list1[0]).integer);
					cmp2 += to_string(t2.getField(attr_list2[0]).integer);
				}
				else
				{
					cmp1 += *(t1.getField(attr_list1[0]).str);
					cmp2 += *(t2.getField(attr_list2[0]).str);
				}
			}
			return cmp1<=cmp2;
		}
	}

	bool TupleCompareEqual(Tuple& t1, Tuple& t2, vector<string>& attr_list1, vector<string>& attr_list2)
	{
		Schema schema1 = t1.getSchema();
		Schema schema2 = t2.getSchema();
		if (attr_list1.size() != attr_list2.size())
			return false;
		// t1 t2 same tuple field
		if (attr_list1.size() == 0)
		{
			for (int i = 0; i < t1.getNumOfFields(); i++)
			{
				if (schema1.getFieldType(i) != schema2.getFieldType(i))
					return false;
				if (schema1.getFieldType(i) == INT)
				{
					if (t1.getField(i).integer != t2.getField(i).integer)
						return false;
				}
				else
				{
					if (*(t1.getField(i).str) != *(t2.getField(i).str))
						return false;
				}
			}
			return true;
		}
		// t1 t2 different field
		else
		{
			for (int i = 0; i < attr_list1.size(); i++)
			{
				if (schema1.getFieldType(attr_list1[i]) != schema2.getFieldType(attr_list2[i]))
					return false;
				if (schema1.getFieldType(attr_list1[i]) == INT)
				{
					if (t1.getField(attr_list1[i]).integer != t2.getField(attr_list2[i]).integer)
						return false;
				}
				else
				{
					if (*(t1.getField(attr_list1[i]).str) != *(t2.getField(attr_list2[i]).str))
						return false;
				}
			}
			return true;
		}
	}

	bool CreateTable(string& table_name, vector<string>& field_names, vector<enum FIELD_TYPE>& field_types)
	{
		Schema schema(field_names, field_types);
		Relation* Relation = schemaMng->createRelation(table_name, schema);
		if (Relation == NULL) {
			return false;
		}
		return true;
	}
	bool CreateTable(ParseTree& PT)
	{
		string table_name = PT.GetTableName();
		vector<string> field_names;
		vector<enum FIELD_TYPE> field_types;
		PT.GetAttrTypeList(field_names, field_types);

		// Storage Manager API
		Schema schema(field_names, field_types);
		Relation* newRelation = schemaMng->createRelation(table_name, schema);
		if (newRelation == NULL) {
			return false;
		}
		return true;
	}
	bool DropTable(string table_name)
	{
		return schemaMng->deleteRelation(table_name);
		return true;
	}
	bool DropTable(ParseTree& PT)
	{
		string table_name = PT.GetTableName();
		return schemaMng->deleteRelation(table_name);
		return true;
	}
	/* acknowlegement from TestStorageManager  */
	void appendTupleToRelation(Relation* relation_ptr, int memory_block_index, Tuple& tuple)
	{
		Block* block_ptr;
		if (relation_ptr->getNumOfBlocks() == 0) {
			block_ptr = memory->getBlock(memory_block_index);
			block_ptr->clear(); //clear the block
			block_ptr->appendTuple(tuple); // append the tuple
			relation_ptr->setBlock(relation_ptr->getNumOfBlocks(), memory_block_index);
		}
		else {
			relation_ptr->getBlock(relation_ptr->getNumOfBlocks() - 1, memory_block_index);
			block_ptr = memory->getBlock(memory_block_index);

			if (block_ptr->isFull()) {
				block_ptr->clear(); //clear the block
				block_ptr->appendTuple(tuple); // append the tuple
				relation_ptr->setBlock(relation_ptr->getNumOfBlocks(), memory_block_index); //write back to the relation
			}
			else {
				block_ptr->appendTuple(tuple); // append the tuple
				relation_ptr->setBlock(relation_ptr->getNumOfBlocks() - 1, memory_block_index); //write back to the relation
			}
		}
	}

	void appendTuplesToRelationFromMemory(int mem_blk_end, Relation* relation_dst, vector<string>& select_list, ConditionParser& CP)
	{
		Block* block_out;
		Schema schema = relation_dst->getSchema();
		// prepare initial output block
		int write_block_index;
		if (relation_dst->getNumOfBlocks() == 0) {
			block_out = memory->getBlock(mem_blk_end+1);
			block_out->clear(); //clear the block
			write_block_index = 0;
		}
		else {
			relation_dst->getBlock(relation_dst->getNumOfBlocks() - 1, mem_blk_end + 1);
			block_out = memory->getBlock(mem_blk_end+1);

			if (block_out->isFull()) {
				block_out->clear(); //clear the block
				write_block_index = relation_dst->getNumOfBlocks();
			}
			else
				write_block_index = relation_dst->getNumOfBlocks() - 1;
		}

		Block* block_in;
		for (int i = 0; i <= mem_blk_end; i++)
		{
			block_in = memory->getBlock(i);
			vector<Tuple> tuples = block_in->getTuples();
			for (int j = 0; j < block_in->getNumTuples(); j++)
			{
				if (!CP.HasExpression() || (CP.HasExpression() && CP.Evaluation(tuples[j])))
				{
					if (block_out->isFull()) {
						relation_dst->setBlock(write_block_index++, mem_blk_end + 1);
						block_out->clear(); //clear the block
					}
					block_out->appendTuple(tuples[j]);
				}
			}
		}
		// write back last block
		if (block_out->isFull()) {
			relation_dst->setBlock(write_block_index++, mem_blk_end + 1);
		}
	}

	void appendTuplesToRelationFromRelation(Relation* relation_src, Relation* relation_dst, vector<string>& select_list, ConditionParser& CP)
	{
		Block* block_in = memory->getBlock(0);
		Block* block_out;
		Schema schema = relation_src->getSchema();
		
		// prepare initial output block
		int write_block_index;
		if (relation_dst->getNumOfBlocks() == 0) {
			block_out = memory->getBlock(1);
			block_out->clear(); //clear the block
			write_block_index = 0;
		}
		else {
			relation_dst->getBlock(relation_dst->getNumOfBlocks() - 1, 1);
			block_out = memory->getBlock(1);
			
			if (block_out->isFull()) {
				block_out->clear(); //clear the block
				write_block_index = relation_dst->getNumOfBlocks();
			}
			else
				write_block_index = relation_dst->getNumOfBlocks() - 1;
		}

		for (int i = 0; i < relation_src->getNumOfBlocks(); i++)
		{
			relation_src->getBlock(i, 0);
			vector<Tuple> tuples = block_in->getTuples();
			for (int j = 0; j < block_in->getNumTuples(); j++)
			{
				if (!CP.HasExpression() || (CP.HasExpression() && CP.Evaluation(tuples[j])))
				{
					if (block_out->isFull()) {
						relation_dst->setBlock(write_block_index++, 1);
						block_out->clear(); //clear the block
					}
					block_out->appendTuple(tuples[j]);
				}
			}
		}
		// write back last block
		if (block_out->isFull()) {
			relation_dst->setBlock(write_block_index++, 1);
		}
	}
	bool Insert(ParseTree& PT)
	{
		bool ret = true;
		string table_name = PT.GetTableName();
		Relation* relation = schemaMng->getRelation(table_name);
		if (relation == NULL) {
			return false;
		}
		Schema schema = relation->getSchema();

		vector<Tuple> tuples;
		vector<string> attr_list;
		vector<string> value_list;
		// get attribute_list
		PT.GetAttrList(attr_list);

		if (PT.GetInsertTreeType() == VALUES) {
			// get value_list
			PT.GetValueList(value_list);

			// Storage Manager API
			Tuple t = relation->createTuple();
			for (int i = 0; i < attr_list.size(); ++i) 
			{
				FIELD_TYPE f = schema.getFieldType(attr_list[i]);
				// handle NULL
				if (f== INT && value_list[i] == "NULL")
					value_list[i] = to_string(INT_MIN);
				
				if (f == INT)
					ret = t.setField(attr_list[i], atoi(value_list[i].c_str()));
				else
					ret = t.setField(attr_list[i], value_list[i]);

				if (ret == false) {
					return false;
				}
			}
			tuples.push_back(t);
			appendTupleToRelation(relation, 0, tuples[0]);
		}
		// select statement
		else
		{
			vector<string> select_list;
			vector<string> table_list;
			vector<string> condition_list;
			ConditionParser CP;

			// get select_list
			PT.GetSelectListFromSubtree(select_list, PT.GetInsertSelectTree());
			// get table_list
			PT.GetTableListFromSubtree(table_list, PT.GetInsertSelectTree());
			if (PT.IsInsertSelectWhere(PT.GetInsertSelectTree()))
			{
				PT.GetConditionList(condition_list);
				CP.GetReversePolish(condition_list);
			}
			// not single table doesn't handle
			if (table_list.size() > 1)
				return false;
			Relation* relation_src = schemaMng->getRelation(table_list[0]);
			if (relation_src == NULL) {
				return false;
			}
			if (select_list.size() == 1 && select_list[0] == "*")
			{
				Schema schema_src = relation_src->getSchema();
				select_list = schema_src.getFieldNames();
			}
			// select list doesn't match insert attr list
			if (select_list != attr_list)
				return false;
			// SELECT table can fit in memory
			if (relation_src->getNumOfBlocks() < TOTAL_MEM_BLKS)
			{
				for (int i = 0; i < relation_src->getNumOfBlocks(); i++)
				{
					relation_src->getBlock(i, i);
				}
				appendTuplesToRelationFromMemory(relation_src->getNumOfBlocks() - 1, relation, select_list, CP);
			}
			// same table, cannot fit in memory, need tempory table to hold data
			else if (table_name == table_list[0])
			{
				string tmp_table_name = "insert_tmp";
                vector<enum FIELD_TYPE> attr_types = schema.getFieldTypes();
				if (!CreateTable(tmp_table_name, attr_list, attr_types))
					return false;
				Relation* tmp_relation = schemaMng->getRelation(tmp_table_name);
				if (tmp_relation == NULL)
					return false;
				appendTuplesToRelationFromRelation(relation_src, tmp_relation, select_list, CP);
				appendTuplesToRelationFromRelation(tmp_relation, relation, select_list, CP);
				DropTable(tmp_table_name);
			}
			else
			{
				appendTuplesToRelationFromRelation(relation_src, relation, select_list, CP);
			}
		}
		
		return true;
	}

    void TableScanWithConditionForOnePass(string& table_name, vector<string>& select_list, ConditionParser& CP, bool IsSort,  vector<string>& sort_attrs, bool IsDistinct)
	{
		Block* block_ptr;
		Relation* relation = schemaMng->getRelation(table_name);
		Schema schema = relation->getSchema();
		vector<Tuple> tuples;

		for (int i = 0; i < relation->getNumOfBlocks(); i++)
		{
			relation->getBlock(i, i);
			block_ptr = memory->getBlock(i);
			for (int j = 0; j < block_ptr->getNumTuples(); j++)
			{
				tuples.push_back(block_ptr->getTuple(j));
			}
		}

		PrintAttr(table_name, select_list);
		if (IsSort && !IsDistinct)
		{
			SortInMemory(tuples, sort_attrs, schema);
			for (int i = 0; i < tuples.size(); i++)
			{
				if (!CP.HasExpression() || (CP.HasExpression() && CP.Evaluation(tuples[i])))
					PrintTuple(tuples[i], select_list);
			}
		}
		else if(!IsSort && IsDistinct)
		{
			SortInMemory(tuples, select_list, schema);
			int i = 0;
			Tuple temp_tuple = relation->createTuple();
			while (i < tuples.size())
			{
				if (i + 1 < tuples.size())
				{
					while (TupleCompareEqual(tuples[i], tuples[i + 1], select_list, select_list))
						i++;
				}
				if (!CP.HasExpression() || (CP.HasExpression() && CP.Evaluation(tuples[i])))
				{
					PrintTuple(tuples[i], select_list);
					i++;
				}
			}
		}
		else // IsSort && IsDistinct
		{
			SortInMemory(tuples, select_list, schema);
			int i = 0;
			Tuple temp_tuple = relation->createTuple();
			vector<Tuple> tuples_dr;
			while (i < tuples.size())
			{
				if (i + 1 < tuples.size())
				{
					while (TupleCompareEqual(tuples[i], tuples[i + 1], select_list, select_list))
						i++;
				}
				if (!CP.HasExpression() || (CP.HasExpression() && CP.Evaluation(tuples[i])))
				{
					tuples_dr.push_back(tuples[i]);
					i++;
				}
			}
			SortInMemory(tuples_dr, sort_attrs, schema);
			for (int i = 0; i < tuples_dr.size(); i++)
			{
				PrintTuple(tuples_dr[i], select_list);
			}
		}
		PrintBar(select_list.size(), GetColWidth(select_list));
	}

	void TableScanWithConditionForDistinct(SCAN_TYPE t, string& table_name, vector<string>& select_list, ConditionParser& CP, string& temp_table_name)
	{
		Block* block_ptr = memory->getBlock(0);
		Relation* relation = schemaMng->getRelation(table_name);
		Schema schema = relation->getSchema();

		Relation* relation_o;
		Block* block_ptr_o = memory->getBlock(9);
		block_ptr_o->clear();
		int write_block_index = 0;
		if (t == TABLE)
		{
			relation_o = schemaMng->getRelation(temp_table_name);
		}
		else
			relation_o = NULL;

		if(t == PRINT)
			PrintAttr(table_name, select_list);

		int blks = (int)ceil((double)relation->getNumOfBlocks() / TOTAL_MEM_BLKS);
		vector<ListBlock> LB(blks);
		for (int i = 0; i < blks; i++)
		{
			if (i == blks - 1)
			{
				if(relation->getNumOfBlocks() % TOTAL_MEM_BLKS == 0)
					LB[i].init(memory, relation, i, TOTAL_MEM_BLKS);
				else
					LB[i].init(memory, relation, i, relation->getNumOfBlocks() % TOTAL_MEM_BLKS);
			}
			else
				LB[i].init(memory, relation, i, TOTAL_MEM_BLKS);
		}
		vector<Tuple> tuples;
		vector<Tuple> temp_tuple; // always 1 size
		int id;
		vector<int> ind_map;
		while (true)
		{
			tuples.clear();
			ind_map.clear();
			for (int blk = 0; blk < blks; blk++)
			{
				if (!LB[blk].Empty())
				{
					ind_map.push_back(blk);
					tuples.push_back(LB[blk].GetMinTuple(blk));
				}
			}
			if (tuples.size() == 0) break;
			id = FindMinIndex(tuples, select_list, schema);

			do
			{
				temp_tuple.clear();
				temp_tuple.push_back(tuples[id]);
				LB[ind_map[id]].Pop();
				// read another
				tuples.clear();
				ind_map.clear();
				for (int blk = 0; blk < blks; blk++)
				{
					if (!LB[blk].Empty())
					{
						ind_map.push_back(blk);
						tuples.push_back(LB[blk].GetMinTuple(blk));
					}
				}
				if (tuples.size() == 0) 
					break;
				id = FindMinIndex(tuples, select_list, schema);
			} while (TupleCompareEqual(tuples[id], temp_tuple[0], select_list, select_list));

			if (!CP.HasExpression() || (CP.HasExpression() && CP.Evaluation(temp_tuple[0])))
			{
				if (block_ptr_o->isFull() && t == TABLE) {
					relation_o->setBlock(write_block_index++, 9);
					block_ptr_o->clear(); //clear the block
				}
				if (t == TABLE)
					block_ptr_o->appendTuple(temp_tuple[0]);
				else
					PrintTuple(temp_tuple[0], select_list);
			}
				
			temp_tuple.clear();
		}
		if (!block_ptr_o->isEmpty() && t == TABLE) {
			relation_o->setBlock(write_block_index, 9);
			block_ptr_o->clear(); //clear the block
		}
		if (t == PRINT)
			PrintBar(select_list.size(), GetColWidth(select_list));
	}

	void TableScanWithConditionForSort(string& table_name, vector<string>& select_list, ConditionParser& CP, vector<string>& sort_attr)
	{
		Block* block_ptr = memory->getBlock(0);
		Relation* relation = schemaMng->getRelation(table_name);
		Schema schema = relation->getSchema();

		PrintAttr(table_name, select_list);

		int blks = (int)ceil((double)relation->getNumOfBlocks() / TOTAL_MEM_BLKS);
		vector<ListBlock> LB(blks);
		for (int i = 0; i < blks; i++)
		{
			if (i == blks - 1)
			{
				if(relation->getNumOfBlocks() % TOTAL_MEM_BLKS == 0)
					LB[i].init(memory, relation, i, TOTAL_MEM_BLKS);
				else
					LB[i].init(memory, relation, i, relation->getNumOfBlocks() % TOTAL_MEM_BLKS);
			}
				
			else
				LB[i].init(memory, relation, i, TOTAL_MEM_BLKS);
		}
		vector<Tuple> tuples;
		int id;
		vector<int> ind_map;
		while (true)
		{
			tuples.clear();
			ind_map.clear();
			for (int blk = 0; blk < blks; blk++)
			{
				if (!LB[blk].Empty())
				{
					ind_map.push_back(blk);
					tuples.push_back(LB[blk].GetMinTuple(blk));
				}
			}
			if (tuples.size() == 0) break;
			id = FindMinIndex(tuples, sort_attr, schema);
			if (!CP.HasExpression() || (CP.HasExpression() && CP.Evaluation(tuples[id])))
			{
				PrintTuple(tuples[id], select_list);
			}
				
			LB[ind_map[id]].Pop();
		}
		
		PrintBar(select_list.size(), GetColWidth(select_list));
	}

	void TableScanWithCondition(string& table_name, vector<string>& select_list, ConditionParser& CP)
	{
		Block* block_ptr = memory->getBlock(0);
		Relation* relation = schemaMng->getRelation(table_name);
		Schema schema = relation->getSchema();

		PrintAttr(table_name, select_list);
		
		for (int i = 0; i < relation->getNumOfBlocks(); i++)
		{
			relation->getBlock(i, 0);
			vector<Tuple> tuples = block_ptr->getTuples();
			for (int j = 0; j < block_ptr->getNumTuples(); j++)
			{
				if (!CP.HasExpression() || (CP.HasExpression() && CP.Evaluation(tuples[j])))
					PrintTuple(tuples[j], select_list);
			}
		}
		PrintBar(select_list.size(), GetColWidth(select_list));
	}

	/**********  Cross Join  **********/
	void ConnectTwoTuples(Tuple& tuple1, Tuple& tuple2, Tuple& tuple_o)
	{
		Schema schema_o = tuple_o.getSchema();
		for (int i = 0; i < tuple_o.getNumOfFields(); i++)
		{
			if (i < tuple1.getNumOfFields())
			{
				if (schema_o.getFieldType(i) == INT)
					tuple_o.setField(i, tuple1.getField(i).integer);
				else
					tuple_o.setField(i, *(tuple1.getField(i).str));
			}
			else
			{
				if (schema_o.getFieldType(i) == INT)
					tuple_o.setField(i, tuple2.getField(i - tuple1.getNumOfFields()).integer);
				else
					tuple_o.setField(i, *(tuple2.getField(i - tuple1.getNumOfFields()).str));
			}
		}
	}
	
	void OrderTalbeName(string& t1, string& t2, string& table1, string& table2)
	{
		Relation* relation1 = schemaMng->getRelation(t1);
		Relation* relation2 = schemaMng->getRelation(t2);

		if (relation1->getNumOfBlocks() < relation2->getNumOfBlocks())
		{
			table1 = t1;
			table2 = t2;
		}
		else
		{
			table2 = t1;
			table1 = t2;
		}
	}

	bool CrossJoinTwoTables(JOIN_TYPE t, string& t1, string& t2, string& ret_table)
	{
		string table1, table2;
		// small table in front (table1)
		OrderTalbeName(t1,t2,table1,table2);

		vector<string> joined_table_field_names;
		vector<enum FIELD_TYPE> joined_table_field_types;
		Relation* relation = schemaMng->getRelation(table1);
		Schema schema = relation->getSchema();
		
		for (int j = 0; j < schema.getNumOfFields(); j++)
		{
			if (schema.getFieldName(j).find(".") != string::npos) // already full attr name
				joined_table_field_names.push_back(schema.getFieldName(j));
			else
				joined_table_field_names.push_back(table1 + "." + schema.getFieldName(j));
			joined_table_field_types.push_back(schema.getFieldType(j));
		}

		relation = schemaMng->getRelation(table2);
		schema = relation->getSchema();
		for (int j = 0; j < schema.getNumOfFields(); j++)
		{
			if (schema.getFieldName(j).find(".") != string::npos) // already full attr name
				joined_table_field_names.push_back(schema.getFieldName(j));
			else
				joined_table_field_names.push_back(table2 + "." + schema.getFieldName(j));
			joined_table_field_types.push_back(schema.getFieldType(j));
		}
		
		if (!CreateTable(ret_table, joined_table_field_names, joined_table_field_types))
			return false;
		
		if (t == TO_PRINT)
			PrintAttr(joined_table_field_names);

		Relation* relation1 = schemaMng->getRelation(table1);
		Relation* relation2 = schemaMng->getRelation(table2);
		Relation* relation_o = schemaMng->getRelation(ret_table);

		Block* block_ptr1;
		Block* block_ptr2 = memory->getBlock(8);
		Block* block_ptr_o = memory->getBlock(9);
		block_ptr_o->clear();
		vector<Tuple> tuples1;
		vector<Tuple> tuples2;
		Tuple tuple_o = relation_o->createTuple();
		int write_block_index = 0;
		int table1_read_blocks = 0;
		int blocks_in_mem = 0;
		while (table1_read_blocks < relation1->getNumOfBlocks())
		{
			// read small table into memory (table1)
			blocks_in_mem = 0;
			// IO
			for (int i1 = 0; i1 < TOTAL_MEM_BLKS - 2; i1++)
			{
				relation1->getBlock(table1_read_blocks++, i1);
				blocks_in_mem++;
				if (table1_read_blocks == relation1->getNumOfBlocks())
					break;
			}
			// IO
			for (int i2 = 0; i2 < relation2->getNumOfBlocks(); i2++)
			{
				relation2->getBlock(i2, 8);
				tuples2 = block_ptr2->getTuples();
				// memory
				for (int k = 0; k < blocks_in_mem; k++)
				{
					block_ptr1 = memory->getBlock(k);
					tuples1 = block_ptr1->getTuples();
					for (int j1 = 0; j1 < block_ptr1->getNumTuples(); j1++)
					{
						// memory
						for (int j2 = 0; j2 < block_ptr2->getNumTuples(); j2++)
						{
							if (block_ptr_o->isFull() && t == TO_TABLE) {
								relation_o->setBlock(write_block_index++, 9);
								block_ptr_o->clear(); //clear the block
							}
							ConnectTwoTuples(tuples1[j1], tuples2[j2], tuple_o);
							if (t == TO_TABLE)
								block_ptr_o->appendTuple(tuple_o);
							else
								PrintTuple(tuple_o);
						}
					}
				}
			}
		}
		if (!block_ptr_o->isEmpty() && t == TO_TABLE) {
			relation_o->setBlock(write_block_index, 9);
			block_ptr_o->clear(); //clear the block
		}
		if(t==TO_PRINT)
			PrintBar(joined_table_field_names.size(), GetColWidth(joined_table_field_names));
		return true;
	}

	bool CrossJoin(JOIN_TYPE t, vector<string>& table_list, string& ret_table_name)
	{
		// more than two tables need extra tempory table
		if (table_list.size() > 2)
		{
			string table1, table2;
			string temp_joined_table_name;
			
			for (int i = 1; i < table_list.size()-1; i++)
			{
				table2 = table_list[i];
				if (temp_joined_table_name.size() > 0)
					table1 = temp_joined_table_name;
				else
					table1 = table_list[0];

				temp_joined_table_name = "joined_table_tmp" + to_string(i);
				if (!CrossJoinTwoTables(TO_TABLE, table1, table2, temp_joined_table_name))
					return false;
				if (table1 != table_list[0])
					DropTable(table1);
			}
			if (!CrossJoinTwoTables(t, temp_joined_table_name, table_list[table_list.size() - 1], ret_table_name));
				return false;
			DropTable(temp_joined_table_name);
			if (t == TO_PRINT)
				DropTable(ret_table_name);
		}
		// only two tables, directly call CrossJoinTwoTables
		else
		{
			CrossJoinTwoTables(t, table_list[0], table_list[1], ret_table_name);
			if(t==TO_PRINT)
				DropTable(ret_table_name);
		}
		return true;
	}

	/**********  Two Pass Sort  **********/
	bool FindAttrForSort(string& attr, vector<string>& attr_list)
	{
		return find(attr_list.begin(), attr_list.end(), attr) != attr_list.end();
	}
	vector<int> IndexSortInt(vector<int>& v)
	{
		// initialize original index locations
		vector<int> idx(v.size());
		iota(idx.begin(), idx.end(), 0);
		// sort indexes based on comparing values in v
		sort(idx.begin(), idx.end(),
			[&v](int i1, int i2) {return v[i1] < v[i2]; });
		return idx;
	}
	vector<int> IndexSortStr(vector<string>& v)
	{
		// initialize original index locations
		vector<int> idx(v.size());
		iota(idx.begin(), idx.end(), 0);
		// sort indexes based on comparing values in v
		sort(idx.begin(), idx.end(),
			[&v](int i1, int i2) {return v[i1] < v[i2]; });
		return idx;
	}
	bool SortInMemory(vector<Tuple>& tuples, vector<string>& sort_attrs, Schema& schema)
	{
		int offset;
		for (int i = 0; i < sort_attrs.size(); i++)
		{
			offset = schema.getFieldOffset(sort_attrs[i]);
			if (offset == -1)
				return false;
		}

		vector<int> index;
		if (sort_attrs.size() == 1 && schema.getFieldType(sort_attrs[0]) == INT)
		{
			vector<int> v;
			for (int i = 0; i < tuples.size(); i++)
			{
				v.push_back(tuples[i].getField(sort_attrs[0]).integer);
			}
			index = IndexSortInt(v);
		}
		else
		{
			vector<string> v;
			string tmp_v;
			for (int i = 0; i < tuples.size(); i++)
			{
				for (int j = 0; j < tuples[i].getNumOfFields(); j++)
				{
                    string fn = schema.getFieldName(j);
					if (schema.getFieldType(j) == INT && (FindAttrForSort(fn, sort_attrs) || sort_attrs.size() == 0))
						tmp_v += to_string(tuples[i].getField(j).integer);
					else if (schema.getFieldType(j) == STR20 && (FindAttrForSort(fn, sort_attrs) || sort_attrs.size() == 0))
						tmp_v += *(tuples[i].getField(j).str);
				}
				v.push_back(tmp_v);
				tmp_v.clear();
			}
			index = IndexSortStr(v);
		}
		
		vector<Tuple> t;
		for (int i = 0; i < index.size(); i++)
		{
			t.push_back(tuples[index[i]]);
		}
		tuples = t;
		return true;
	}
	void PreSortTwoPass(Relation* relation, vector<string>& sort_attrs)
	{
		Block* block_ptr;
		vector<Tuple> tuples;
		Schema schema = relation->getSchema();
		int total_blks = 0;
		int read_blks;
		int write_blks = 0;
		while (total_blks < relation->getNumOfBlocks())
		{
			// IO read
			tuples.clear();
			read_blks = 0;
			for (int i = 0; i < TOTAL_MEM_BLKS; i++)
			{
				if (total_blks == relation->getNumOfBlocks())
					break;
				relation->getBlock(total_blks++, i);
				block_ptr = memory->getBlock(i);
				for(int j=0;j<block_ptr->getNumTuples();j++)
					tuples.push_back(block_ptr->getTuple(j));
				read_blks++;
			}
			SortInMemory(tuples, sort_attrs, schema);
			int cnt = 0;
			for (int i = 0; i < read_blks; i++)
			{
				block_ptr = memory->getBlock(i);
				for (int j = 0; j < block_ptr->getNumTuples(); j++)
				{
					block_ptr->setTuple(j, tuples[cnt++]);
				}
			}
			// IO WRITE
			relation->setBlocks(write_blks, 0, read_blks);
			write_blks += read_blks;
		}
	}

	int FindMinIndex(vector<Tuple>& tuples, vector<string>& sort_attrs, Schema& schema)
	{
		int offset;
		for (int i = 0; i < sort_attrs.size(); i++)
		{
			offset = schema.getFieldOffset(sort_attrs[i]);
			if (offset == -1)
				return false;
		}

		vector<int> index;
		if (sort_attrs.size() == 1 && schema.getFieldType(sort_attrs[0]) == INT)
		{
			vector<int> v;
			for (int i = 0; i < tuples.size(); i++)
			{
				v.push_back(tuples[i].getField(sort_attrs[0]).integer);
			}
			index = IndexSortInt(v);
		}
		else
		{
			vector<string> v;
			string tmp_v;
			for (int i = 0; i < tuples.size(); i++)
			{
				for (int j = 0; j < tuples[i].getNumOfFields(); j++)
				{
                    string fn = schema.getFieldName(j);
					if (schema.getFieldType(j) == INT && (FindAttrForSort(fn, sort_attrs) || sort_attrs.size() == 0))
						tmp_v += to_string(tuples[i].getField(j).integer);
					else if (schema.getFieldType(j) == STR20 && (FindAttrForSort(fn, sort_attrs) || sort_attrs.size() == 0))
						tmp_v += *(tuples[i].getField(j).str);
				}
				v.push_back(tmp_v);
				tmp_v.clear();
			}
			index = IndexSortStr(v);
		}

		return index[0];
	}

	/**********  Natural Join  **********/
	bool NaturalJoinTwoTables(JOIN_TYPE t, ConditionParser& CP, string& t1, string& t2, vector<string>& select_list, vector<string>& attrs1, vector<string>& attrs2, string& ret_table)
	{
		string table1, table2;
		vector<string> attr_list1, attr_list2;
		// small table in front (table1)
		OrderTalbeName(t1, t2, table1, table2);
		if (t1 != table1)
		{
			attr_list1 = attrs2;
			attr_list2 = attrs1;
		}
		else
		{
			attr_list1 = attrs1;
			attr_list2 = attrs2;
		}

		vector<string> joined_table_field_names;
		vector<enum FIELD_TYPE> joined_table_field_types;
		Relation* relation = schemaMng->getRelation(table1);
		Schema schema = relation->getSchema();

		for (int j = 0; j < schema.getNumOfFields(); j++)
		{
			if (schema.getFieldName(j).find(".") != string::npos) // already full attr name
				joined_table_field_names.push_back(schema.getFieldName(j));
			else
				joined_table_field_names.push_back(table1 + "." + schema.getFieldName(j));
			joined_table_field_types.push_back(schema.getFieldType(j));
		}

		relation = schemaMng->getRelation(table2);
		schema = relation->getSchema();
		for (int j = 0; j < schema.getNumOfFields(); j++)
		{
			if (schema.getFieldName(j).find(".") != string::npos) // already full attr name
				joined_table_field_names.push_back(schema.getFieldName(j));
			else
				joined_table_field_names.push_back(table2 + "." + schema.getFieldName(j));
			joined_table_field_types.push_back(schema.getFieldType(j));
		}

		if (select_list[0] == "*" && t == TO_PRINT)
			select_list = joined_table_field_names;

		if (!CreateTable(ret_table, joined_table_field_names, joined_table_field_types))
			return false;

		if (t == TO_PRINT)
			PrintAttr(select_list);

		Relation* relation1 = schemaMng->getRelation(table1);
		Relation* relation2 = schemaMng->getRelation(table2);
		Relation* relation_o = schemaMng->getRelation(ret_table);
		Tuple tuple_o = relation_o->createTuple();
		Block* block_ptr_o = memory->getBlock(9);
		block_ptr_o->clear();
		int write_block_index = 0;
		Schema schema1 = relation1->getSchema();
		Schema schema2 = relation2->getSchema();
		// one pass 
		if (relation1->getNumOfBlocks() <= TOTAL_MEM_BLKS - 2)
		{
			// read table1 into memory frotm block0
			Block* block_in1;
			Block* block_in2;
			vector<Tuple> tuples1;
			for (int i = 0; i < relation1->getNumOfBlocks(); i++)
			{
				relation1->getBlock(i, i);
				block_in1 = memory->getBlock(i);
				for (int j = 0; j < block_in1->getNumTuples(); j++)
				{
					tuples1.push_back(block_in1->getTuple(j));
				}
			}
			//SortInMemory(tuples1, attr_list1, schema);

			// read table2 by block and join
			block_in2 = memory->getBlock(8);
			Tuple tuple2 = relation2->createTuple();
			for (int i = 0; i < relation2->getNumOfBlocks(); i++)
			{
				relation2->getBlock(i,8);
				for (int j = 0; j < block_in2->getNumTuples(); j++)
				{
					tuple2 = block_in2->getTuple(j);
					for (int k = 0; k < tuples1.size(); k++)
					{
						if (TupleCompareEqual(tuples1[k], tuple2, attr_list1, attr_list2))
						{
							ConnectTwoTuples(tuples1[k], tuple2, tuple_o);
							if (block_ptr_o->isFull() && t == TO_TABLE) {
								relation_o->setBlock(write_block_index++, 9);
								block_ptr_o->clear(); //clear the block
							}
							if (t == TO_TABLE)
								block_ptr_o->appendTuple(tuple_o);
							else
							{
								if (!CP.HasExpression() || (CP.HasExpression() && CP.Evaluation(tuple_o)))
									PrintTuple(tuple_o, select_list);
							}
						}
					}
					
				}
			}
		}
		// two pass
		else
		{
			PreSortTwoPass(relation1, attr_list1);
			PreSortTwoPass(relation2, attr_list2);
			int blks1 = (int)ceil((double)relation1->getNumOfBlocks() / TOTAL_MEM_BLKS);
			int blks2 = (int)ceil((double)relation2->getNumOfBlocks() / TOTAL_MEM_BLKS);
			vector<ListBlock> LB1(blks1);
			vector<ListBlock> LB2(blks2);
			for (int i = 0; i < blks1; i++)
			{
				if (i == blks1 - 1)
				{
					if (relation1->getNumOfBlocks() % TOTAL_MEM_BLKS == 0)
						LB1[i].init(memory, relation1, i, TOTAL_MEM_BLKS);
					else
						LB1[i].init(memory, relation1, i, relation1->getNumOfBlocks() % TOTAL_MEM_BLKS);
				}
				else
					LB1[i].init(memory, relation1, i, TOTAL_MEM_BLKS);
			}
			for (int i = 0; i < blks2; i++)
			{
				if (i == blks2 - 1)
				{
					if(relation2->getNumOfBlocks() % TOTAL_MEM_BLKS == 0)
						LB2[i].init(memory, relation2, i, TOTAL_MEM_BLKS);
					else
						LB2[i].init(memory, relation2, i, relation2->getNumOfBlocks() % TOTAL_MEM_BLKS);
				}
				else
					LB2[i].init(memory, relation2, i, TOTAL_MEM_BLKS);
			}
			vector<Tuple> tuples1;
			vector<Tuple> tuples2;
			int id1, id2;
			vector<int> ind1_map, ind2_map;
			block_ptr_o->clear();
			while (true)
			{
				int blk = 0;
				tuples1.clear();
				tuples2.clear();
				ind1_map.clear();
				ind2_map.clear();
				for (; blk < blks1; blk++)
				{
					if (!LB1[blk].Empty())
					{
						ind1_map.push_back(blk);
						tuples1.push_back(LB1[blk].GetMinTuple(blk));
					}
				}
				if (tuples1.size() == 0) return true;
				for (; blk < blks1+blks2; blk++)
				{
					if (!LB2[blk - blks1].Empty())
					{
						ind2_map.push_back(blk - blks1);
						tuples2.push_back(LB2[blk - blks1].GetMinTuple(blk));
					}
				}
				if (tuples2.size() == 0) return true;
				id1 = FindMinIndex(tuples1, attr_list1, schema1);
				id2 = FindMinIndex(tuples2, attr_list2, schema2);
				vector<Tuple> temp_tuples;
				temp_tuples.clear();
				while (TupleCompareEqual(tuples1[id1], tuples2[id2], attr_list1, attr_list2) == true)
				{
					if (tuples1[id1].getField(0).integer == 16)
					{
						int test = 1;
					}
					temp_tuples.push_back(tuples1[id1]);
					LB1[ind1_map[id1]].Pop();
					tuples1.clear();
					ind1_map.clear();
					// read all Rmin which are same
					for (int i = 0; i < blks1; i++)
					{
						if (!LB1[i].Empty())
						{
							tuples1.push_back(LB1[i].GetMinTuple(i));
							ind1_map.push_back(i);
						}
					}
					if (tuples1.size() == 0) return true;
					id1 = FindMinIndex(tuples1, attr_list1, schema1);
				}

				while (temp_tuples.size() > 0 && TupleCompareEqual(temp_tuples[0], tuples2[id2], attr_list1, attr_list2) == true)
				{
					if (temp_tuples[0].getField(0).integer == 3)
					{
						int test = 1;
					}
					for (int i = 0; i < temp_tuples.size(); i++)
					{
						ConnectTwoTuples(temp_tuples[i], tuples2[id2], tuple_o);
						if (block_ptr_o->isFull() && t == TO_TABLE) {
							relation_o->setBlock(write_block_index++, 9);
							block_ptr_o->clear(); //clear the block
						}
						if (t == TO_TABLE)
							block_ptr_o->appendTuple(tuple_o);
						else
						{
							if (!CP.HasExpression() || (CP.HasExpression() && CP.Evaluation(tuple_o)))
								PrintTuple(tuple_o, select_list);
						}
					}
					LB2[ind2_map[id2]].Pop();
					tuples2.clear();
					ind2_map.clear();
					for (int i = blks1; i < blks1 + blks2; i++)
					{
						if (!LB2[i - blks1].Empty())
						{
							ind2_map.push_back(i - blks1);
							tuples2.push_back(LB2[i - blks1].GetMinTuple(i));
						}
					}
					if (tuples2.size() == 0) return true;
					id2 = FindMinIndex(tuples2, attr_list2, schema2);
				}
				// delete smaller one
				if (temp_tuples.size() == 0)
				{
					if(TupleCompareSmaller(tuples1[id1],tuples2[id2],attr_list1,attr_list2))
						LB1[ind1_map[id1]].Pop();
					else
						LB2[ind2_map[id2]].Pop();
				}
			}
		}
		if (!block_ptr_o->isEmpty() && t == TO_TABLE) {
			relation_o->setBlock(write_block_index, 9);
			block_ptr_o->clear(); //clear the block
		}
		if (t == TO_PRINT)
			PrintBar(select_list.size(), GetColWidth(select_list));
		return true;
	}

	bool MatchNaturalJoinAttr(JoinPair& jp, string& table1, string& table2, vector<string>& attr_list1, vector<string>& attr_list2)
	{
		// match table name
		if ((jp.GetTable1() == table1 && jp.GetTable2() == table2) || (jp.GetTable1() == table2 && jp.GetTable2() == table1))
		{
			attr_list1.push_back(jp.join_attr);
			attr_list2.push_back(jp.join_attr);
			return true;
		}
		// match full attr name
		else if (jp.GetTable1() == table1)
		{
			Relation* relation = schemaMng->getRelation(table2);
			Schema schema = relation->getSchema();
			vector<string> attr_names = schema.getFieldNames();
			if (find(attr_names.begin(), attr_names.end(), jp.GetTable2FullAttr()) != attr_names.end())
			{
				attr_list1.push_back(jp.join_attr);
				attr_list2.push_back(jp.GetTable2FullAttr());
				return true;
			}
			else
				return false;
		}
		else if (jp.GetTable1() == table2)
		{
			Relation* relation = schemaMng->getRelation(table1);
			Schema schema = relation->getSchema();
			vector<string> attr_names = schema.getFieldNames();
			if (find(attr_names.begin(), attr_names.end(), jp.GetTable2FullAttr()) != attr_names.end())
			{
				attr_list2.push_back(jp.join_attr);
				attr_list1.push_back(jp.GetTable2FullAttr());
				return true;
			}
			else
				return false;
		}
		else if (jp.GetTable2() == table1)
		{
			Relation* relation = schemaMng->getRelation(table2);
			Schema schema = relation->getSchema();
			vector<string> attr_names = schema.getFieldNames();
			if (find(attr_names.begin(), attr_names.end(), jp.GetTable1FullAttr()) != attr_names.end())
			{
				attr_list1.push_back(jp.join_attr);
				attr_list2.push_back(jp.GetTable1FullAttr());
				return true;
			}
			else
				return false;
		}
		else if (jp.GetTable2() == table2)
		{
			Relation* relation = schemaMng->getRelation(table1);
			Schema schema = relation->getSchema();
			vector<string> attr_names = schema.getFieldNames();
			if (find(attr_names.begin(), attr_names.end(), jp.GetTable1FullAttr()) != attr_names.end())
			{
				attr_list2.push_back(jp.join_attr);
				attr_list1.push_back(jp.GetTable1FullAttr());
				return true;
			}
			else
				return false;
		}
		else
			return false;
	}
	
	bool NaturalJoin(JOIN_TYPE t, ConditionParser& CP, vector<string>& table_list, vector<string>& select_list, vector<JoinPair>& natural_join_list, string& ret_table_name)
	{
		vector<string> join_attr_list1;
		vector<string> join_attr_list2;

		// more than two tables need extra tempory table
		if (table_list.size() > 2)
		{
			string table1, table2;
			string temp_joined_table_name;

			for (int i = 1; i < table_list.size() - 1; i++)
			{
				table2 = table_list[i];
				if (temp_joined_table_name.size() > 0)
					table1 = temp_joined_table_name;
				else
					table1 = table_list[0];

				temp_joined_table_name = "joined_table_tmp" + to_string(i);

				join_attr_list1.clear();
				join_attr_list2.clear();
				for (int i = 0; i < natural_join_list.size(); i++)
				{
					if (MatchNaturalJoinAttr(natural_join_list[i], table1, table2, join_attr_list1, join_attr_list2))
						natural_join_list[i].Clear();
				}

				NaturalJoinTwoTables(TO_TABLE, CP, table1, table2, select_list, join_attr_list1, join_attr_list2, temp_joined_table_name);
				if (table1 != table_list[0])
					DropTable(table1);
			}

			join_attr_list1.clear();
			join_attr_list2.clear();
			for (int i = 0; i < natural_join_list.size(); i++)
			{
				if (MatchNaturalJoinAttr(natural_join_list[i], temp_joined_table_name, table_list[table_list.size() - 1], join_attr_list1, join_attr_list2))
					natural_join_list[i].Clear();
			}

			NaturalJoinTwoTables(t, CP, temp_joined_table_name, table_list[table_list.size()-1], select_list, join_attr_list1, join_attr_list2, ret_table_name);
			DropTable(temp_joined_table_name);
			if (t == TO_PRINT)
				DropTable(ret_table_name);
		}
		// only two tables, directly call CrossJoinTwoTables
		else
		{
			string table1, table2;
			table1 = table_list[0];
			table2 = table_list[1];
			join_attr_list1.clear();
			join_attr_list2.clear();
			for (int i = 0; i < natural_join_list.size(); i++)
			{
				if(MatchNaturalJoinAttr(natural_join_list[i], table1, table2, join_attr_list1, join_attr_list2))
					natural_join_list[i].Clear();
			}
			NaturalJoinTwoTables(t, CP, table1, table2, select_list, join_attr_list1, join_attr_list2, ret_table_name);
			if (t == TO_PRINT)
				DropTable(ret_table_name);
		}
		return true;
	}

	bool ThetaJoinTwoTables(JOIN_TYPE t, string& t1, string& t2, string& ret_table, ConditionParser& CP)
	{
		string table1, table2;
		// small table in front (table1)
		OrderTalbeName(t1, t2, table1, table2);

		vector<string> joined_table_field_names;
		vector<enum FIELD_TYPE> joined_table_field_types;
		Relation* relation = schemaMng->getRelation(table1);
		Schema schema = relation->getSchema();

		for (int j = 0; j < schema.getNumOfFields(); j++)
		{
			if (schema.getFieldName(j).find(".") != string::npos) // already full attr name
				joined_table_field_names.push_back(schema.getFieldName(j));
			else
				joined_table_field_names.push_back(table1 + "." + schema.getFieldName(j));
			joined_table_field_types.push_back(schema.getFieldType(j));
		}

		relation = schemaMng->getRelation(table2);
		schema = relation->getSchema();
		for (int j = 0; j < schema.getNumOfFields(); j++)
		{
			if (schema.getFieldName(j).find(".") != string::npos) // already full attr name
				joined_table_field_names.push_back(schema.getFieldName(j));
			else
				joined_table_field_names.push_back(table2 + "." + schema.getFieldName(j));
			joined_table_field_types.push_back(schema.getFieldType(j));
		}

		if (!CreateTable(ret_table, joined_table_field_names, joined_table_field_types))
			return false;

		if (t == TO_PRINT)
			PrintAttr(joined_table_field_names);

		Relation* relation1 = schemaMng->getRelation(table1);
		Relation* relation2 = schemaMng->getRelation(table2);
		Relation* relation_o = schemaMng->getRelation(ret_table);

		Block* block_ptr1;
		Block* block_ptr2 = memory->getBlock(8);
		Block* block_ptr_o = memory->getBlock(9);
		block_ptr_o->clear();
		vector<Tuple> tuples1;
		vector<Tuple> tuples2;
		Tuple tuple_o = relation_o->createTuple();
		int write_block_index = 0;
		int table1_read_blocks = 0;
		int blocks_in_mem = 0;
		while (table1_read_blocks < relation1->getNumOfBlocks())
		{
			// read small table into memory (table1)
			blocks_in_mem = 0;
			// IO
			for (int i1 = 0; i1 < TOTAL_MEM_BLKS - 2; i1++)
			{
				relation1->getBlock(table1_read_blocks++, i1);
				blocks_in_mem++;
				if (table1_read_blocks == relation1->getNumOfBlocks())
					break;
			}
			// IO
			for (int i2 = 0; i2 < relation2->getNumOfBlocks(); i2++)
			{
				relation2->getBlock(i2, 8);
				tuples2 = block_ptr2->getTuples();
				// memory
				for (int k = 0; k < blocks_in_mem; k++)
				{
					block_ptr1 = memory->getBlock(k);
					tuples1 = block_ptr1->getTuples();
					for (int j1 = 0; j1 < block_ptr1->getNumTuples(); j1++)
					{
						// memory
						for (int j2 = 0; j2 < block_ptr2->getNumTuples(); j2++)
						{
							ConnectTwoTuples(tuples1[j1], tuples2[j2], tuple_o);
							if (!CP.HasExpression() || (CP.HasExpression() && CP.Evaluation(tuple_o)))
							{
								if (block_ptr_o->isFull() && t == TO_TABLE) {
									relation_o->setBlock(write_block_index++, 9);
									block_ptr_o->clear(); //clear the block
								}
								if (t == TO_TABLE)
									block_ptr_o->appendTuple(tuple_o);
								else
									PrintTuple(tuple_o);
							}
						}
					}
				}
			}
		}
		if (!block_ptr_o->isEmpty() && t == TO_TABLE) {
			relation_o->setBlock(write_block_index, 9);
			block_ptr_o->clear(); //clear the block
		}
		if (t == TO_PRINT)
			PrintBar(joined_table_field_names.size(), GetColWidth(joined_table_field_names));
		return true;
	}

	bool ThetaJoin(JOIN_TYPE t, vector<string>& table_list, string& ret_table_name, ConditionParser& CP)
	{
		// more than two tables need extra tempory table
		if (table_list.size() > 2)
		{
			string table1, table2;
			string temp_joined_table_name;

			for (int i = 1; i < table_list.size() - 1; i++)
			{
				table2 = table_list[i];
				if (temp_joined_table_name.size() > 0)
					table1 = temp_joined_table_name;
				else
					table1 = table_list[0];

				temp_joined_table_name = "joined_table_tmp" + to_string(i);
				if (!ThetaJoinTwoTables(TO_TABLE, table1, table2, temp_joined_table_name, CP))
					return false;
				if (table1 != table_list[0])
					DropTable(table1);
			}
			if (!ThetaJoinTwoTables(t, temp_joined_table_name, table_list[table_list.size() - 1], ret_table_name, CP));
			return false;
			DropTable(temp_joined_table_name);
			if (t == TO_PRINT)
				DropTable(ret_table_name);
		}
		// only two tables, directly call ThetaJoinTwoTables
		else
		{
			ThetaJoinTwoTables(t, table_list[0], table_list[1], ret_table_name, CP);
			if (t == TO_PRINT)
				DropTable(ret_table_name);
		}
		return true;
	}


	bool Select(ParseTree& PT)
	{
		vector<string> select_list;
		vector<string> table_list;
		vector<string> condition_list;
		vector<JoinPair> natural_join_list;
		vector<string> sort_attr;
		// get select_list
		PT.GetSelectList(select_list);
		// get table_list
		PT.GetTableList(table_list);

		bool IsDistinct = PT.IsSelectDistinct();
		bool IsSort = PT.IsSelectOrderBy();
		if (IsSort)
			PT.GetSelectOrderBy(sort_attr);

		ConditionParser CP;
		if (PT.IsSelectWhere())
		{
			PT.GetConditionList(condition_list);
			CP.GetReversePolish(condition_list);
		}

		if (table_list.size() == 0)
		{
			return false;
		}
		// single table
		else if (table_list.size() == 1)
		{
			string table_name = table_list[0];
			Relation* relation = schemaMng->getRelation(table_name);
			Schema schema = relation->getSchema();
			vector<string> AttrNames = schema.getFieldNames();

			if (select_list.size() == 1 && select_list[0] == "*")
			{
				select_list = AttrNames;
			}
			else
			{
				// filter table name for single table
				for (int i = 0; i < select_list.size(); i++)
				{
					if (select_list[i].find('.') != string::npos)
						select_list[i] = select_list[i].substr(select_list[i].find('.')+1, select_list[i].size() - select_list[i].find('.'));
				}
			}

			if (IsSort && !IsDistinct)
			{
				if (relation->getNumOfBlocks() <= TOTAL_MEM_BLKS)
				{
					TableScanWithConditionForOnePass(table_list[0], select_list, CP, IsSort, sort_attr, IsDistinct);
				}
				else
				{
					PreSortTwoPass(relation, sort_attr);
					TableScanWithConditionForSort(table_list[0], select_list, CP, sort_attr);
				}
			}
			else if (!IsSort && IsDistinct)
			{
				if (relation->getNumOfBlocks() <= TOTAL_MEM_BLKS)
				{
					TableScanWithConditionForOnePass(table_list[0], select_list, CP, IsSort, sort_attr, IsDistinct);
				}
				else
				{
					string tmp_table_name;
					PreSortTwoPass(relation, select_list);
					TableScanWithConditionForDistinct(PRINT, table_list[0], select_list, CP, tmp_table_name);
				}
			}
			else if (IsSort && IsDistinct)
			{
				if (relation->getNumOfBlocks() <= TOTAL_MEM_BLKS)
				{
					TableScanWithConditionForOnePass(table_list[0], select_list, CP, IsSort, sort_attr, IsDistinct);
				}
				else
				{
					string tmp_table_name = "temp_table";
                    vector<string> tmp_attr_names = schema.getFieldNames();
                    vector<enum FIELD_TYPE> tmp_attr_types = schema.getFieldTypes();
					if (!CreateTable(tmp_table_name, tmp_attr_names, tmp_attr_types))
						return false;
					Relation* relation_tmp = schemaMng->getRelation(tmp_table_name);
					PreSortTwoPass(relation, select_list);
					TableScanWithConditionForDistinct(TABLE, table_list[0], select_list, CP, tmp_table_name);
					PreSortTwoPass(relation_tmp, sort_attr);
					TableScanWithConditionForSort(tmp_table_name, select_list, CP, sort_attr);
					DropTable(tmp_table_name);
				}
			}
			else
			{
				TableScanWithCondition(table_list[0], select_list, CP);
			}
		}
		// multiple table
		else
		{
			// joined table relation schema
			string joined_table_name = "joined_table_tmp";
			
			// first find natural join
			if (CP.HasExpression())
			{
				CP.GetNaturalJoinList(condition_list, natural_join_list);

				// natural join
				if (natural_join_list.size() > 0)
				{
					Relation* relation_nj;
					if (IsSort && !IsDistinct)
					{
						NaturalJoin(TO_TABLE, CP, table_list, select_list, natural_join_list, joined_table_name);
						relation_nj = schemaMng->getRelation(joined_table_name);
						if (select_list[0] == "*")
							select_list = relation_nj->getSchema().getFieldNames();
						if (relation_nj->getNumOfBlocks() <= TOTAL_MEM_BLKS)
						{
							TableScanWithConditionForOnePass(joined_table_name, select_list, CP, IsSort, sort_attr, IsDistinct);
						}
						else
						{
							PreSortTwoPass(relation_nj, sort_attr);
							TableScanWithConditionForSort(joined_table_name, select_list, CP, sort_attr);
							DropTable(joined_table_name);
						}
					}
					else if (!IsSort && IsDistinct)
					{
						string tmp_table_name;
						NaturalJoin(TO_TABLE, CP, table_list, select_list, natural_join_list, joined_table_name);
						relation_nj = schemaMng->getRelation(joined_table_name);
						if (select_list[0] == "*")
							select_list = relation_nj->getSchema().getFieldNames();
						if (relation_nj->getNumOfBlocks() <= TOTAL_MEM_BLKS)
						{
							TableScanWithConditionForOnePass(joined_table_name, select_list, CP, IsSort, sort_attr, IsDistinct);
						}
						else
						{
							PreSortTwoPass(relation_nj, select_list);
							TableScanWithConditionForDistinct(PRINT, joined_table_name, select_list, CP, tmp_table_name);
							DropTable(joined_table_name);
						}
					}
					else if (IsSort && IsDistinct)
					{
						string tmp_table_name = "temp_table";
						NaturalJoin(TO_TABLE, CP, table_list, select_list, natural_join_list, joined_table_name);
						relation_nj = schemaMng->getRelation(joined_table_name);
						Schema schema_nj = relation_nj->getSchema();
						if (relation_nj->getNumOfBlocks() <= TOTAL_MEM_BLKS)
						{
							TableScanWithConditionForOnePass(joined_table_name, select_list, CP, IsSort, sort_attr, IsDistinct);
						}
						else
						{
                            vector<string> nj_attr_names = schema_nj.getFieldNames();
                            vector<enum FIELD_TYPE> nj_attr_types = schema_nj.getFieldTypes();
							if (!CreateTable(tmp_table_name, nj_attr_names,nj_attr_types))
								return false;
							Relation* relation_tmp = schemaMng->getRelation(tmp_table_name);
							PreSortTwoPass(relation_nj, select_list);
							TableScanWithConditionForDistinct(TABLE, joined_table_name, select_list, CP, tmp_table_name);
							PreSortTwoPass(relation_tmp, sort_attr);
							TableScanWithConditionForSort(tmp_table_name, select_list, CP, sort_attr);
							DropTable(joined_table_name);
							DropTable(tmp_table_name);
						}
					}
					else
					{
						NaturalJoin(TO_PRINT, CP, table_list, select_list, natural_join_list, joined_table_name);
					}
				}
				// theta join
				else
				{
					ThetaJoin(TO_PRINT, table_list, joined_table_name, CP);
				}

			}
			// we can only cross join
			else
			{
				CrossJoin(TO_PRINT, table_list, joined_table_name);
			}
		}

		return true;
	}

	bool BlockIsAllNull(Block* block_ptr)
	{
		vector<Tuple> tuples = block_ptr->getTuples();
		for (int i = 0; i < tuples.size(); i++)
		{
			if (!tuples[i].isNull()) return false;
		}
		return true;
	}

	bool Delete(ParseTree& PT)
	{
		string table_name = PT.GetTableName();
		Block* block_ptr = memory->getBlock(0);
		Block* block_ptr_last = memory->getBlock(1);
		Relation* relation = schemaMng->getRelation(table_name);
		Schema schema = relation->getSchema();
		ConditionParser CP;

		if (PT.IsDeleteWhere())
		{
			vector<string> condition_list;
			PT.GetConditionList(condition_list);
			CP.GetReversePolish(condition_list);
		}

		for (int i = 0; i < relation->getNumOfBlocks(); i++)
		{
			relation->getBlock(i, 0);
			Tuple last_tuple = relation->createTuple();
			vector<Tuple> tuples = block_ptr->getTuples();
			for (int j = 0; j < block_ptr->getNumTuples(); j++)
			{
				if (!CP.HasExpression() || (CP.HasExpression() && CP.Evaluation(tuples[j])))
				{
					if (i < relation->getNumOfBlocks() - 1)
					{
						relation->getBlock(relation->getNumOfBlocks() - 1, 1);
						while (true)
						{
							last_tuple = block_ptr_last->getTuple(block_ptr_last->getNumTuples() - 1);
							if (CP.HasExpression() && !CP.Evaluation(last_tuple))
							{
								block_ptr->setTuple(j, last_tuple);
								block_ptr_last->nullTuple(block_ptr_last->getNumTuples() - 1);
								if (BlockIsAllNull(block_ptr_last))
								{
									relation->deleteBlocks(relation->getNumOfBlocks() - 1);
								}
								relation->setBlock(i, 0);
								break;
							}
							else
							{
								block_ptr_last->nullTuple(block_ptr_last->getNumTuples() - 1);
							}
							if (BlockIsAllNull(block_ptr_last))
							{
								relation->deleteBlocks(relation->getNumOfBlocks() - 1);
								if (relation->getNumOfBlocks() == 0)
									break;
								relation->getBlock(relation->getNumOfBlocks() - 1, 1);
							}
						}
					}
					else // must be last blcok
					{
						block_ptr->nullTuple(j);
						if (BlockIsAllNull(block_ptr_last))
							relation->deleteBlocks(i); // must be last blcok
						else
							relation->setBlock(i, 0);
					}
				}
			}
		}
		return true;
	}

public:
	bool ProcessQuery(std::string& query) {
		Tokens t;
		ParseTree PT;
		bool ret;

		disk->resetDiskIOs();
		disk->resetDiskTimer();

		t.Tokenize(query);
		PT.GetParseTree(t);

		if (PT.GetPTType() == CREATE_TABLE_STATEMENT) 
		{
			ret = CreateTable(PT);
		}
		else if (PT.GetPTType() == DROP_TABLE_STATEMENT) 
		{
			ret = DropTable(PT);
		}
		else if (PT.GetPTType() == INSERT_STATEMENT) 
		{
			ret = Insert(PT);
		}
		else if (PT.GetPTType() == SELECT_STATEMENT) 
		{
			ret = Select(PT);
		}
		else if (PT.GetPTType() == DELETE_STATEMENT)
		{
			ret = Delete(PT);
		}

		PrintStats();
		return ret;
	}
public:
	DatabaseManager(MainMemory* m, Disk* d){
		schemaMng = new SchemaManager(m, d);
		memory = m;
		disk = d;
	}
	~DatabaseManager()
	{
		delete schemaMng;
	}

};

#endif