//                                                      _ooOoo_
//                                                     o8888888o
//                                                     88" . "88
//                                                     (| -_- |)
//                                                      O\ = /O
//                                                  ____/`---'\____
//                                                .   ' \\| |// `.
//                                                 / \\||| : |||// \
//                                               / _||||| -:- |||||- \
//                                                 | | \\\ - /// | |
//                                               | \_| ''\---/'' | |
//                                                \ .-\__ `-` ___/-. /
//                                             ___`. .' /--.--\ `. . __
//                                          ."" '< `.___\_<|>_/___.' >'"".
//                                         | | : `- \`.;`\ _ /`;.`/ - ` : | |
//                                           \ \ `-. \_ __\ /__ _/ .-` / /
//                                   ======`-.____`-.___\_____/___.-`____.-'======
//                                                       =---='
//
//                                   .............................................
//                                              佛祖保佑             永无BUG
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

#include "thread_pool.h"
#include "locker.h"
#include "task.h"

using namespace std;

class webSpider {
private:
	string beginURL;
	unordered_map<string, int> is_exit;  //hash表，用来存放已经爬过的URL
	//unordered_map<string, int> is_get;   //            存放已经下载过的图片的URL
	queue<string> que_url;  //存放待爬URL
	vector<string> com_URL; 
	vector<string>::iterator iter;
	
public:
	webSpider(const string &_beginURL) : beginURL(_beginURL) {}
	~webSpider() {}
	void getImgByBFS(void);
private:
	int getHTML(const string &_url);  //获取指定_url整个html界面并下载保存，返回文件的名字
	void regexURL(int _flie_index);
};

void webSpider::getImgByBFS(void) {
	string cur_url;
	int index = 0;
	
	//多线程主要是<同时>进行"多个html"中img的下载，不是一个html中<同时>"多个img"下载
	//所以要将每个种子URL对应的html分别下载保存，以1.txt, 2.txt, 3.txt, ... x.txt命名
	threadpool<task> pool(20);  
	pool.start();
	
	que_url.push(beginURL);
	while (!que_url.empty()) {
		cur_url = que_url.front();
		que_url.pop();
		is_exit[cur_url]++;

		index = getHTML(cur_url);
		if(index == -1)  //wget failed
			continue;
		task *ta = new task(index, cur_url);  //regexImg and download img
		pool.append_task(ta);
		
		regexURL(index);
		
		for (iter = com_URL.begin(); iter != com_URL.end(); ++iter) {
			if (is_exit[*iter] == 0)
				que_url.push(*iter);
		}
		com_URL.clear();
	}
	
	pool.stop();
}

int webSpider::getHTML(const string &_url) {
	static int index = 0;
	string url_name;
	pid_t stat;
	
	string cmd;
	cmd = "wget -O index.html ";  
	cmd += _url;
	stat = system(cmd.c_str());
	
	if(WEXITSTATUS(stat) == 0) {  //wget success
		cmd = "mv";    //rename
		cmd += " index.html";
		cmd += " ./html/";
		cmd += to_string(index);
		cmd += ".txt";
		stat = system(cmd.c_str());
		
		if(WEXITSTATUS(stat) == 0)  //mv success
			return index++;
	}
	return -1;
}

void webSpider::regexURL(int _file_index) {
	smatch mat;       
	//regex  expr_url("[a-zA-Z0-9][-a-zA-Z0-9]{0,62}(/.[a-zA-Z0-9][-a-zA-Z0-9]{0,62})+/.?");
	regex  expr_url("href=\"(http://[^\\s'\"]+)\"");
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
			if (regex_search(start, end, mat, expr_url)) {  //整合页面中的所有URL
				string msg = mat[1].str();
				com_URL.push_back(msg);
				start = mat[0].second;
			}
			line_buf.clear();
		}
		file.close();	
	}
	else
		errExit("ifstream error!");
}

int main(void) {
	string beginURL = "https://www.zhipin.com/";
	//string beginURL = "http://www.php.cn/";
	
	if (mkdir("./img", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
		errExit("mkdir error!");
	if (mkdir("./html", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
		errExit("mkdir error!");
	
	webSpider myspider(beginURL);

	myspider.getImgByBFS();

	return 0;
}