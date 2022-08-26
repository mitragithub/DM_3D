#include<stdio.h>
#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<algorithm>
#include<cmath>

//#include"hash.h"
//#include"readini.h"

using namespace std;


#define DEBUG 0

int main(int argc, char* argv[])
{
string search_path;
	
	string search_path;
	parameter para;
	vector<fileinfo> blocks;
	
	if (argc == 2){
		/*
		prefix = string(argv[1]);
		trans = string(argv[2]);
		setting_name = string(argv[3]);
		nb = 12;
		*/
		string configname(argv[1]);
		cout << "Reading parameters from: " << configname << endl;
		init_file(configname, para, blocks);
		cout << para << endl;
		cout << "block count: " << blocks.size() << endl;

		search_path = getpath(argv[1]);
		cout << "work folder: " << search_path << endl;
	}
	else {
		cout << "usage: merge_graph <config_file>";
		return 0;
	}
	return 0;
}