#ifndef _TASK_H
#define _TASK_H

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <regex>
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace std;

const string path = "./img";
const int BUFFER_SIZE = 1024;

void errExit(const string &_errinfo) {
	cout << _errinfo << endl;
	exit(1);
}

class task {
private:
	string cur_url;
	int file;
	vector<string> img_URL;
	vector<string>::iterator iter;
	
public:
	task(int _file, const string &_cur_url) : file(_file), cur_url(_cur_url) {}
	~task(void) {}
	void doit(void);
private:
	void regexImg(int _flie_index);
	void saveImg(const string &_img_url, const string &_host);
};

void task::doit(void) {		
	regexImg(file);
	
	for (iter = img_URL.begin(); iter != img_URL.end(); ++iter)
		saveImg(*iter, cur_url);
	img_URL.clear();
}

void task::regexImg(int _file_index) {
	smatch mat;       
	regex  expr_img("src=\"(.*(png|svg|jpg))\"");
	string line_buf;
	string file_name;
	bool too_long = 0;
	
	file_name = "./html/" + to_string(_file_index) + ".txt";
	ifstream file(file_name);
	if(file) {
		while(getline(file, line_buf)) {
			if(line_buf.length() > 1024) 
				continue;
			string::const_iterator start = line_buf.begin();
			string::const_iterator end   = line_buf.end();
			if (regex_search(start, end, mat, expr_img)) {  //整合页面中的所有IMG
				string msg = mat[1].str();
				img_URL.push_back(msg);
				start = mat[0].second;
			}
			line_buf.clear();
		}
		file.close();	
	}
	else
		errExit("ifstream error!");
}

void task::saveImg(const string &_img_url, const string &_host) {
	static int x = 0;
	int n = 0;
	int i = 0;
	string format;
	string img_name;

	/*set img name and path*/
	for(i = _img_url.length() - 4; i<_img_url.length(); ++i)
		format.push_back(_img_url[i]);
	for(i = _img_url.length() - 5; ; --i)
		if(_img_url[i] == '/')
			break;
	++i;
	for(; i<_img_url.length() - 4; ++i)
		img_name.push_back(_img_url[i]);
	img_name = img_name + format;
	
	/*download img*/
	string sys_cmd = "wget ";  //下面这个if还有待调试
	if((!_host.find("https")) && (_img_url.find("http") == -1) && (_img_url.find("ftp") == -1)) {
		sys_cmd += (_host + "/" + _img_url);
	}
	else
		sys_cmd += _img_url;
	system(sys_cmd.c_str());
	sys_cmd = "mv ";
	sys_cmd += img_name;
	sys_cmd += (" ./img/" + img_name);
	system(sys_cmd.c_str());
}

#endif