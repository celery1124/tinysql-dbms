/* main.cpp
* CSCE 608 - 600 Fall 2017
* by Mian Qin
*/

#include "Common.h"
#include "DatabaseManager.h"

void print_usage()
{
	printf("usage: [-h] \tprint help\n [-f] \t execute query from file\n");
}

int main(int argc, char **argv)
{
	MainMemory mem;
	Disk disk;
	DatabaseManager DBMS(&mem, &disk);
	string query;
	if (argc > 1)
	{
		char *filename = argv[1];
		fstream fp;
		fp.open(filename, fstream::in);
		if (!fp)
		{
			cout << "query script: " << filename << " open failed!" << endl;
			print_usage();
			return 0;
		}
		
		while (!fp.eof())
		{
			getline(fp, query);
			cout << "> " << query << endl;
			if (DBMS.ProcessQuery(query))
				cout << "SUCCESS" << endl;
			else
				cout << "FAILED" << endl;
		}
		fp.close();

	}
	else
	{
		while (true)
		{
			cout << "> ";
			getline(cin, query);
			if (query == "q" || query == "quit") {
				break;
			}
			if (DBMS.ProcessQuery(query))
				cout << "SUCCESS" << endl;
			else
				cout << "FAILED" << endl;
		}
	}

	cout << "BYE!" << endl;

	return 0;
}