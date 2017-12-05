/* Parser.h
* CSCE 608 - 600 Fall 2017
* by Mian Qin
*/

#ifndef   _parser_h   
#define   _parser_h

#include "Common.h"

class Tokens
{
private:
	bool IsDigit(char in)
	{
		if (in <= '9' && in >= '0')
			return true;
		else
			return false;
	}
	bool IsLetter(char in)
	{
		if ((in >= 'a' && in <= 'z') || (in >= 'A' && in <= 'Z'))
			return true;
		else
			return false;
	}
public:
	vector<string> tokens;
	int offset;

	Tokens()
	{
		offset = 0;
	}
	int Size()
	{
		return tokens.size();
	}
	void ResetOffset()
	{
		offset = 0;
	}
	string GetNextToken()
	{
		if (offset >= tokens.size())
			return "";
		else
			return tokens[offset++];
	}
	string GetPrevToken()
	{
		if (--offset < 0)
			return "";
		else
			return tokens[offset];
	}

	bool Tokenize(string& query)
	{
		int quote_r;
		string token;
		int i = 0;
		while (i < query.size())
		{
			if (query[i] == ' ' || query[i] == '\r' || query[i] == '\n')
			{
				i++;
				continue;
			}
			else if (query[i] == '"')
			{
				quote_r = query.find('"', i + 1);
				if (quote_r == -1)
				{
					printf("[Tokenize] \" mismatch, please check query\n");
					return false;
				}
				tokens.push_back(query.substr(i + 1, quote_r - 1 - i));
				i = quote_r + 1;
			}
			else if (IsDigit(query[i]))
			{
				token.clear();
				while (IsDigit(query[i]))
				{
					token.push_back(query[i]);
					i++;
				}
				tokens.push_back(token);
			}
			else if (IsLetter(query[i]))
			{
				token.clear();
				while (IsLetter(query[i]) || IsDigit(query[i]) || query[i] == '.')
				{
					token.push_back(query[i]);
					i++;
				}
				tokens.push_back(token);
			}
			else
			{
				tokens.push_back(string(1, query[i]));
				i++;
			}
		}
		return true;
	}
};

class ParseTreeNode
{
public:
	PTN_TYPE type;
	string value;
	vector<ParseTreeNode*> siblings;
	ParseTreeNode() {}
	ParseTreeNode(PTN_TYPE t)
	{
		type = t;
	}
	ParseTreeNode(PTN_TYPE t, string v)
	{
		type = t;
		value = v;
	}
	void AddSibling(ParseTreeNode* s)
	{
		siblings.push_back(s);
	}
	~ParseTreeNode()
	{
		for (int i = 0; i < siblings.size(); i++)
		{
			delete siblings[i];
		}
	}
};

class ParseTree
{
public:
	ParseTreeNode* root;
	PTN_TYPE GetPTType()
	{
		return root->type;
	}
	enum PTN_TYPE GetInsertTreeType()
	{
		return root->siblings[3]->siblings[0]->type;
	}
	~ParseTree()
	{
		DeleteTreeNode(root);
	}

private:
	void DeleteTreeNode(ParseTreeNode* n)
	{
		if (n->siblings.size() == 0)
		{
			delete n;
			return;
		}
		for (int i = 0; i < n->siblings.size(); i++)
		{
			DeleteTreeNode(n->siblings[i]);
		}
	}
	void Print(ParseTreeNode* n)
	{
		cout << n->type << '(' << n->value << ')' << '|';
		for (int i = 0; i < n->siblings.size(); i++)
		{
			if (i == 0) cout << endl;
			Print(n->siblings[i]);
		}
		if (n->siblings.size() > 0) return;
	}

	// 5 types of statement create, drop, insert, delete, select
	/**     create statement     **/
	bool IsCreateTree(Tokens& t)
	{
		if (t.Size() < 2) return false;
		if (t.GetNextToken() == "CREATE" && t.GetNextToken() == "TABLE")
			return true;
		else
			return false;
	}
	ParseTreeNode* GetAttrTypeListTree(Tokens& t)
	{
		ParseTreeNode* att_type_list = new ParseTreeNode(ATTRIBUTE_TYPE_LIST);
		att_type_list->AddSibling(new ParseTreeNode(ATTRIBUTE_NAME, t.GetNextToken()));
		att_type_list->AddSibling(new ParseTreeNode(DATA_TYPE, t.GetNextToken()));
		if (t.GetNextToken() == ",")
		{
			att_type_list->AddSibling(GetAttrTypeListTree(t));
		}
		return att_type_list;
	}
	void TranverseAttrTypeListTree(ParseTreeNode* n, vector<string>& field_names, vector<enum FIELD_TYPE>& field_types)
	{
		if (n->siblings.size() > 0)
		{
			field_names.push_back(n->siblings[0]->value);
			enum FIELD_TYPE ft;
			if (n->siblings[1]->value == "INT")
				ft = INT;
			else if (n->siblings[1]->value == "STR20")
				ft = STR20;
			field_types.push_back(ft);
			if(n->siblings.size() > 2)
				TranverseAttrTypeListTree(n->siblings[2], field_names, field_types);
		}
	}
	ParseTreeNode* GetCreateTree(Tokens& t)
	{
		ParseTreeNode* root = new ParseTreeNode(CREATE_TABLE_STATEMENT);
		root->AddSibling(new ParseTreeNode(CREATE_TABLE));
		root->AddSibling(new ParseTreeNode(TABLE_NAME, t.GetNextToken()));
		// atrribute type list
		t.GetNextToken();
		root->AddSibling(GetAttrTypeListTree(t));

		return root;
	}

	/**     drop statement     **/
	bool IsDropTree(Tokens& t)
	{
		if (t.Size() < 2) return false;
		if (t.GetNextToken() == "DROP" && t.GetNextToken() == "TABLE")
			return true;
		else
			return false;
	}
	ParseTreeNode* GetDropTree(Tokens& t)
	{
		ParseTreeNode* root = new ParseTreeNode(DROP_TABLE_STATEMENT);
		root->AddSibling(new ParseTreeNode(DROP_TABLE));
		root->AddSibling(new ParseTreeNode(TABLE_NAME, t.GetNextToken()));

		return root;
	}

	/**     insert statement     **/
	bool IsInsertTree(Tokens& t)
	{
		if (t.Size() < 2) return false;
		if (t.GetNextToken() == "INSERT" && t.GetNextToken() == "INTO")
			return true;
		else
			return false;
	}
	ParseTreeNode* GetAttrListTree(Tokens& t)
	{
		ParseTreeNode* att_type_list = new ParseTreeNode(ATTRIBUTE_LIST);
		att_type_list->AddSibling(new ParseTreeNode(ATTRIBUTE_NAME, t.GetNextToken()));
		if (t.GetNextToken() == ",")
		{
			att_type_list->AddSibling(GetAttrListTree(t));
		}
		return att_type_list;
	}
	void TranverseAttrListTree(ParseTreeNode* n, vector<string>& field_names)
	{
		if (n->siblings.size() > 0)
		{
			field_names.push_back(n->siblings[0]->value);
			if (n->siblings.size() > 1)
				TranverseAttrListTree(n->siblings[1], field_names);
		}
	}
	ParseTreeNode* GetValueListTree(Tokens& t)
	{
		ParseTreeNode* value_list = new ParseTreeNode(VALUE_LIST);
		value_list->AddSibling(new ParseTreeNode(VALUE, t.GetNextToken()));
		if (t.GetNextToken() == ",")
		{
			value_list->AddSibling(GetValueListTree(t));
		}
		return value_list;
	}
	void TranverseValueListTree(ParseTreeNode* n, vector<string>& value_lists)
	{
		if (n->siblings.size() > 0)
		{
			value_lists.push_back(n->siblings[0]->value);
			if (n->siblings.size() > 1)
				TranverseValueListTree(n->siblings[1], value_lists);
		}
	}
	ParseTreeNode* GetInsertTuplesTree(Tokens& t)
	{
		ParseTreeNode* insert_tuples = new ParseTreeNode(INSERT_TUPLES);
		
		if (t.GetNextToken() == "VALUES")
		{
			insert_tuples->AddSibling(new ParseTreeNode(VALUES, t.GetNextToken()));
			insert_tuples->AddSibling(GetValueListTree(t));
		}
		// select statement
		else
		{
			insert_tuples->AddSibling(new ParseTreeNode(SELECT));
			insert_tuples->AddSibling(GetSelectTree(t));
		}
		return insert_tuples;
	}
	ParseTreeNode* GetInsertTree(Tokens& t)
	{
		ParseTreeNode* root = new ParseTreeNode(INSERT_STATEMENT);
		root->AddSibling(new ParseTreeNode(INSERT_INTO));
		root->AddSibling(new ParseTreeNode(TABLE_NAME, t.GetNextToken()));
		// atrribute list
		t.GetNextToken();
		root->AddSibling(GetAttrListTree(t));
		// insert tuple
		root->AddSibling(GetInsertTuplesTree(t));

		return root;
	}

	/**     select statement     **/
	bool IsSelectTree(Tokens& t)
	{
		if (t.Size() < 2) return false;
		if (t.GetNextToken() == "SELECT")
			return true;
		else
			return false;
	}
	ParseTreeNode* GetSelectSublistTree(Tokens& t)
	{
		ParseTreeNode* select_sublist = new ParseTreeNode(SELECT_SUBLIST);
		select_sublist->AddSibling(new ParseTreeNode(COLUMN_NAME, t.GetNextToken()));
		if (t.GetNextToken() == ",")
		{
			select_sublist->AddSibling(GetSelectSublistTree(t));
		}
		else
			t.GetPrevToken();
		return select_sublist;
	}
	ParseTreeNode* GetSelectListTree(Tokens& t)
	{
		ParseTreeNode* select_list = new ParseTreeNode(SELECT_LIST);
		if(t.GetNextToken() == "*")
			select_list->AddSibling(new ParseTreeNode(STAR, "*"));
		else
		{
			t.GetPrevToken();
			select_list->AddSibling(GetSelectSublistTree(t));
		}
		return select_list;
	}
	void TranverseSelectListTree(ParseTreeNode* n, vector<string>& select_list)
	{
		if (n->siblings[0]->type == STAR)
		{
			select_list.push_back(n->siblings[0]->value);
			return;
		}
		else
		{
			if (n->siblings.size() > 0)
			{
				if (n->siblings.size() > 1)
				{
					select_list.push_back(n->siblings[0]->value);
					TranverseValueListTree(n->siblings[1], select_list);
				}
				else
					TranverseValueListTree(n->siblings[0], select_list);
			}
		}
		
	}
	ParseTreeNode* GetTableListTree(Tokens& t)
	{
		ParseTreeNode* table_list = new ParseTreeNode(TABLE_LIST);
		table_list->AddSibling(new ParseTreeNode(TABLE_NAME, t.GetNextToken()));
		if (t.GetNextToken() == ",")
		{
			table_list->AddSibling(GetTableListTree(t));
		}
		else
			t.GetPrevToken();
		return table_list;
	}
	void TranverseTableListTree(ParseTreeNode* n, vector<string>& table_list)
	{
		if (n->siblings.size() > 0)
		{
			table_list.push_back(n->siblings[0]->value);
			if (n->siblings.size() > 1)
				TranverseValueListTree(n->siblings[1], table_list);
		}
	}
	// fixme
	ParseTreeNode* GetSearchConditionTree(Tokens& t)
	{
		ParseTreeNode* condition_list = new ParseTreeNode(CONDITION_LIST);
		string tmp = t.GetNextToken();
		while (tmp.size() > 0 && tmp != "ORDER")
		{
			t.GetPrevToken();
			condition_list->AddSibling(new ParseTreeNode(CONDITION_LIST_ELEMENT, t.GetNextToken()));
			tmp = t.GetNextToken();
		}
		t.GetPrevToken();
		return condition_list;
	}
	ParseTreeNode* GetSelectTree(Tokens& t)
	{
		ParseTreeNode* root = new ParseTreeNode(SELECT_STATEMENT);
		root->AddSibling(new ParseTreeNode(SELECT));
		// distinct
		if(t.GetNextToken() == "DISTINCT")
			root->AddSibling(new ParseTreeNode(DISTINCT));
		// select-list
		else
		{
			t.GetPrevToken();
		}
		root->AddSibling(GetSelectListTree(t));
		// from
		root->AddSibling(new ParseTreeNode(FROM, t.GetNextToken()));
		// table list
		root->AddSibling(GetTableListTree(t));
		// where
		if (t.GetNextToken() == "WHERE")
		{
			root->AddSibling(new ParseTreeNode(WHERE));
			root->AddSibling(GetSearchConditionTree(t));
		}
		else
			t.GetPrevToken();
		// order by
		if (t.GetNextToken() == "ORDER" && t.GetNextToken() == "BY")
		{
			root->AddSibling(new ParseTreeNode(ORDER_BY));
			root->AddSibling(new ParseTreeNode(COLUMN_NAME, t.GetNextToken()));
		}
		return root;
	}
	/**     delete statement     **/
	bool IsDeleteTree(Tokens& t)
	{
		if (t.Size() < 2) return false;
		if (t.GetNextToken() == "DELETE" && t.GetNextToken() == "FROM")
			return true;
		else
			return false;
	}
 	ParseTreeNode* GetDeleteTree(Tokens& t)
	{
		ParseTreeNode* root = new ParseTreeNode(DELETE_STATEMENT);
		root->AddSibling(new ParseTreeNode(DELETE_FROM));
		root->AddSibling(new ParseTreeNode(TABLE_NAME, t.GetNextToken()));
		// where
		if (t.GetNextToken() == "WHERE")
		{
			root->AddSibling(new ParseTreeNode(WHERE));
			root->AddSibling(GetSearchConditionTree(t));
		}
		return root;
	}

public:

	bool GetParseTree(Tokens& tokens)
	{
		if (IsCreateTree(tokens))
		{
			root = GetCreateTree(tokens);
			//Print(root);
			return true;
		}
		tokens.ResetOffset();
		if (IsDropTree(tokens))
		{
			root = GetDropTree(tokens);
			//Print(root);
			return true;
		}
		tokens.ResetOffset();
		if (IsInsertTree(tokens))
		{
			root = GetInsertTree(tokens);
			//Print(root);
			return true;
		}
		tokens.ResetOffset();
		if (IsSelectTree(tokens))
		{
			root = GetSelectTree(tokens);
			//Print(root);
			return true;
		}
		tokens.ResetOffset();
		if (IsDeleteTree(tokens))
		{
			root = GetDeleteTree(tokens);
			//Print(root);
			return true;
		}
		return false;
	}

	string GetTableName()
	{
		return root->siblings[1]->value;
	}
	void GetAttrTypeList(vector<string>& field_names, vector<enum FIELD_TYPE>& field_types)
	{
		TranverseAttrTypeListTree(root->siblings[2], field_names, field_types);
	}
	void GetAttrList(vector<string>& attr_list)
	{
		TranverseAttrListTree(root->siblings[2], attr_list);
	}
	void GetValueList(vector<string>& value_list)
	{
		TranverseValueListTree(root->siblings[3]->siblings[1], value_list);
	}
	ParseTreeNode* GetInsertSelectTree()
	{
		return root->siblings[3]->siblings[1];
	}
	bool IsInsertSelectWhere(ParseTreeNode* sub_root)
	{
		if (IsSelectDistinct(sub_root))
		{
			if (sub_root->siblings.size() >= 6 && sub_root->siblings[5]->type == WHERE)
				return true;
			else
				return false;
		}
		else
		{
			if (sub_root->siblings.size() >= 5 && sub_root->siblings[4]->type == WHERE)
				return true;
			else
				return false;
		}
	}
	bool IsSelectDistinct(ParseTreeNode* sub_root)
	{
		if (sub_root->siblings.size() >= 2 && sub_root->siblings[1]->type == DISTINCT)
			return true;
		else
			return false;
	}
	bool IsSelectDistinct()
	{
		if (root->siblings.size()>=2 && root->siblings[1]->type == DISTINCT)
			return true;
		else
			return false;
	}
	bool IsSelectWhere()
	{
		if (IsSelectDistinct())
		{
			if (root->siblings.size() >= 6 && root->siblings[5]->type == WHERE)
				return true;
			else
				return false;
		}
		else
		{
			if (root->siblings.size() >= 5 && root->siblings[4]->type == WHERE)
				return true;
			else
				return false;
		}
	}
	bool IsSelectOrderBy()
	{
		if (IsSelectDistinct())
		{
			if (IsSelectWhere())
				return (root->siblings.size() >= 8 && root->siblings[7]->type == ORDER_BY);
			else
				return (root->siblings.size() >= 6 && root->siblings[5]->type == ORDER_BY);
		}
		else
		{
			if (IsSelectWhere())
				return (root->siblings.size() >= 7 && root->siblings[6]->type == ORDER_BY);
			else
				return (root->siblings.size() >= 5 && root->siblings[4]->type == ORDER_BY);
		}
	}
	void GetSelectOrderBy(vector<string>& sort_attr)
	{
		if (!IsSelectOrderBy())
		{
			return;
		}
		if (IsSelectDistinct())
		{
			if (IsSelectWhere())
				sort_attr.push_back(root->siblings[8]->value);
			else
				sort_attr.push_back(root->siblings[6]->value);
		}
		else
		{
			if (IsSelectWhere())
				sort_attr.push_back(root->siblings[7]->value);
			else
				sort_attr.push_back(root->siblings[5]->value);
		}
	}

	bool IsDeleteWhere()
	{
		if (root->siblings.size() >= 3 && root->siblings[2]->type == WHERE)
			return true;
		else
			return false;
	}
	void GetSelectListFromSubtree(vector<string>& select_list, ParseTreeNode* sub_root)
	{
		ParseTreeNode* select_list_tree;
		if (IsSelectDistinct(sub_root))
			select_list_tree = sub_root->siblings[2];
		else
			select_list_tree = sub_root->siblings[1];

		TranverseSelectListTree(select_list_tree, select_list);
	}
	void GetSelectList(vector<string>& select_list)
	{
		GetSelectListFromSubtree(select_list, root);
	}
	
	void GetTableListFromSubtree(vector<string>& table_list, ParseTreeNode* sub_root)
	{
		ParseTreeNode* table_list_tree;
		if (IsSelectDistinct(sub_root))
			table_list_tree = sub_root->siblings[4];
		else
			table_list_tree = sub_root->siblings[3];
		TranverseTableListTree(table_list_tree, table_list);
	}
	void GetTableList(vector<string>& table_list)
	{
		GetTableListFromSubtree(table_list, root);
	}
	void GetConditionListFromSubtree(vector<string>& condition_list, ParseTreeNode* sub_root)
	{
		ParseTreeNode* condition_list_tree;
		if (GetPTType() == SELECT_STATEMENT || GetPTType() == INSERT_STATEMENT)
		{
			if (IsSelectDistinct(sub_root))
				condition_list_tree = sub_root->siblings[6];
			else
				condition_list_tree = sub_root->siblings[5];
		}
		else if (GetPTType() == DELETE_STATEMENT)
		{
			condition_list_tree = sub_root->siblings[3];
		}
		else
			return;

		for (int i = 0; i < condition_list_tree->siblings.size(); i++)
		{
			condition_list.push_back(condition_list_tree->siblings[i]->value);
		}
	}
	void GetConditionList(vector<string>& condition_list)
	{
		GetConditionListFromSubtree(condition_list, root);
	}

};

#endif