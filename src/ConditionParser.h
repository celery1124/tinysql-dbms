/* ConditionParser.h
* CSCE 608 - 600 Fall 2017
* by Mian Qin
*/

#ifndef   _condition_parser_h   
#define   _condition_parser_h

#include "Common.h"

class JoinPair
{
public:
	string table1;
	string table2;
	string join_attr;

	JoinPair(string& t1, string& t2, string& attr)
	{
		table1 = t1;
		table2 = t2;
		join_attr = attr;
	}
	string GetTable1FullAttr()
	{
		return table1 + "." + join_attr;
	}
	string GetTable2FullAttr()
	{
		return table2 + "." + join_attr;
	}
	string GetTable1()
	{
		return table1;
	}
	string GetTable2()
	{
		return table2;
	}
	void Clear()
	{
		table1.clear();
		table2.clear();
		join_attr.clear();
	}
};

class FullAttrName
{
public:
	string table_name;
	string attr_name;
	FullAttrName(){}
	void Set(string full_name)
	{
		int found = full_name.find('.');
		if (found != string::npos)
		{
			table_name = full_name.substr(0, found);
			attr_name = full_name.substr(found + 1, full_name.size()-found-1);
		}
		else
			attr_name = full_name;
	}
	bool IsJoinable(FullAttrName name)
	{
		return (table_name.size()>0 && name.table_name.size() > 0 && attr_name == name.attr_name);
	}
};

class ConditionToken
{
public:
	CT_TYPE type;
	string value;
	ConditionToken() {}
	ConditionToken(CT_TYPE t, string& val)
	{
		type = t;
		value = val;
	}
private:

public:
	bool IsDigit(char in)
	{
		if (in <= '9' && in >= '0')
			return true;
		else
			return false;
	}
	void Set(string& val)
	{
		value = val;
		if (val == "*")
			type = MULT;
		else if (val == "/")
			type = DIV;
		else if (val == "+")
			type = ADD;
		else if (val == "-")
			type = SUB;
		else if (val == "=")
			type = EQUAL;
		else if (val == ">")
			type = GREATER;
		else if (val == "<")
			type = SMALLER;
		else if (val == "NOT")
			type = NOT;
		else if (val == "AND")
			type = AND;
		else if (val == "OR")
			type = OR;
		else if (val == "[" || val == "(")
			type = LEFT_BRACKET;
		else if (val == "]" || val == ")")
			type = RIGHT_BRACKET;
		else
		{
			if (IsDigit(val[0]))
				type = OPRAND_INT;
			else
				type = OPRAND_STR;
		}
	}

	bool IsOperand()
	{
		if (type == OPRAND_INT || type == OPRAND_STR)
			return true;
		else
			return false;
	}
	bool IsLeftBracket()
	{
		return type == LEFT_BRACKET;
	}
	bool IsRightBracket()
	{
		return (type == RIGHT_BRACKET);
	}
	bool IsOperator()
	{
		if (!IsOperand() && !IsLeftBracket() && !IsRightBracket())
			return true;
		else
			return false;
	}
};

class ConditionParser
{
private:
	bool has_expression;
public:
	vector<ConditionToken> tokens_queue;
	ConditionParser()
	{
		has_expression =  false;
	}
private:
	bool GreaterPrecedence(CT_TYPE l, CT_TYPE r)
	{
		return l < r;
	}
	bool IsDigit(char in)
	{
		if (in <= '9' && in >= '0')
			return true;
		else
			return false;
	}
public:
	bool HasExpression()
	{
		return has_expression;
	}
	void GetReversePolish(vector<string>& vs)
	{
		if (vs.size() == 0)
		{
			has_expression = false;
			return;
		}
		else
			has_expression = true;

		GetReversePolish(vs, tokens_queue);
	}
	void GetReversePolish(vector<string>& vs, vector<ConditionToken>& ret)
	{
		stack<ConditionToken> stack;
		ConditionToken token;

		for (int i = 0; i < vs.size(); i++)
		{
			token.Set(vs[i]);
			if (token.IsOperand())
				ret.push_back(token);
			if (token.IsOperator())
			{
				while (!stack.empty() && stack.top().IsOperator() && GreaterPrecedence(stack.top().type, token.type))
				{
					ret.push_back(stack.top());
					stack.pop();
				}
				stack.push(token);
			}
			if (token.IsLeftBracket())
				stack.push(token);
			if (token.IsRightBracket())
			{
				while (!stack.empty() && !stack.top().IsLeftBracket())
				{
					ret.push_back(stack.top());
					stack.pop();
				}
				stack.pop();
			}
		}
		while (!stack.empty())
		{
			ret.push_back(stack.top());
			stack.pop();
		}
	}
	bool Evaluation(Tuple& tuple)
	{
		stack<string> stack;
		ConditionToken token;
		Schema schema = tuple.getSchema();

		string l, r;
		for (int i = 0; i < tokens_queue.size(); i++)
		{
			token = tokens_queue[i];
			if (!token.IsOperator())
			{
				if (token.type == OPRAND_INT)
					stack.push(token.value);
				else
				{
					if (schema.fieldNameExists(token.value))
					{
						if (schema.getFieldType(token.value) == INT)
							stack.push(to_string(tuple.getField(token.value).integer));
						else
						{
							stack.push(*(tuple.getField(token.value).str));
						}
					}
					else
						stack.push(token.value);
				}
			}
			else
			{
				if (token.type == MULT)
				{
					r = stack.top();
					stack.pop();
					l = stack.top();
					stack.pop();
					stack.push(to_string(stoi(l) * stoi(r)));
				}
				else if (token.type == DIV)
				{
					r = stack.top();
					stack.pop();
					l = stack.top();
					stack.pop();
					stack.push(to_string(stoi(l) / stoi(r)));
				}
				else if (token.type == ADD)
				{
					r = stack.top();
					stack.pop();
					l = stack.top();
					stack.pop();
					stack.push(to_string(stoi(l) + stoi(r)));
				}
				else if (token.type == SUB)
				{
					r = stack.top();
					stack.pop();
					l = stack.top();
					stack.pop();
					stack.push(to_string(stoi(l) - stoi(r)));
				}
				else if (token.type == EQUAL)
				{
					r = stack.top();
					stack.pop();
					l = stack.top();
					stack.pop();
					if (IsDigit(r[0]))
						stack.push(to_string((int)(stoi(l) == stoi(r))));
					else
						stack.push(to_string((int)(l == r)));
				}
				else if (token.type == GREATER)
				{
					r = stack.top();
					stack.pop();
					l = stack.top();
					stack.pop();
					stack.push(to_string((int)(stoi(l) > stoi(r))));
				}
				else if (token.type == SMALLER)
				{
					r = stack.top();
					stack.pop();
					l = stack.top();
					stack.pop();
					stack.push(to_string((int)(stoi(l) < stoi(r))));
				}
				else if (token.type == NOT)
				{
					r = stack.top();
					stack.pop();
					stack.push(to_string((int)(!stoi(r))));
				}
				else if (token.type == AND)
				{
					r = stack.top();
					stack.pop();
					l = stack.top();
					stack.pop();
					stack.push(to_string((int)(stoi(l) && stoi(r))));
				}
				else if (token.type == OR)
				{
					r = stack.top();
					stack.pop();
					l = stack.top();
					stack.pop();
					stack.push(to_string((int)(stoi(l) || stoi(r))));
				}
			}
		}
		return stack.top() == "1";
	}

	void GetNaturalJoinList(vector<string>& vs, vector<JoinPair>& natural_join_list)
	{
		vector<string>::iterator it;
		FullAttrName r, l;
		int offset = 0;
		if (find(vs.begin(), vs.end(), "find") == vs.end())
		{
			vector<ConditionToken> temp_rpq;
			GetReversePolish(vs, temp_rpq);

			for (int i = 0; i < temp_rpq.size(); i++)
			{
				if (temp_rpq[i].type == EQUAL)
				{
					r.Set(temp_rpq[i - 1].value);
					l.Set(temp_rpq[i - 2].value);
					if (r.IsJoinable(l))
					{
						natural_join_list.push_back(JoinPair(r.table_name, l.table_name, r.attr_name));
					}
				}
			}
		}
		else
		{
			while ((it = find(vs.begin() + offset, vs.end(), "AND")) != vs.end())
			{
				vector<string> sub_vs(vs.begin() + offset, it - 1);
				vector<ConditionToken> temp_rpq;
				offset = it - vs.begin() + 1;

				GetReversePolish(sub_vs, temp_rpq);

				for (int i = 0; i < temp_rpq.size(); i++)
				{
					if (temp_rpq[i].type == EQUAL)
					{
						r.Set(temp_rpq[i - 1].value);
						l.Set(temp_rpq[i - 2].value);
						if (r.IsJoinable(l))
						{
							natural_join_list.push_back(JoinPair(r.table_name, l.table_name, r.attr_name));
						}
					}
				}
			}
		}
	}
};

#endif