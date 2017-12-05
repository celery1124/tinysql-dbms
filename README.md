# TAMU 608 project2


1 Files included?
1) tinysql-dbms/ project files (Visual studio 2015), includes all the source code for the database management system (also including the storage manager library)
	src/Common.h all the imclude headers and some global enum
	src/ConditionParser.h  interface and implementations of the condition parser and evaluator (revese polish algorithm)
	src/DatabaseManager.h  interface and implementations of the database management system (including create/drop table, insert/delete tuples select statement: table scan, sorting, duplication removal, multi-table cross/natural/theta join)
	src/main.h main function of the whole program, including a command interface and a file interpreter interface
	src/Parser.h interface and implementations of tokenizer and parser tree.
	src/StorageManager/ Storage manager source code

	test.txt test script provide by class
2) report.pdf is the final report for the project.

2 How to build and run the program?
1) make
2) Run
	i) run directly as ./tinysql-dbms in powershell and input query manually (q or quit to exit).
	ii) use a file script using ./tinysql-dbms script.txt

Any questions, please contact celery1124@tamu.edu, tang16@tamu.edu

Best regards,
Mian Qin, Songyuan Tang
