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

void errExit(const string &_errinfo) {
	cout << _errinfo << endl;
	exit(1);
}

class webSpider {
private:
	bool protocol;
	string beginURL;
	unordered_map<string, int> is_exit;  //hash表，用来存放已经爬过的URL
	//unordered_map<string, int> is_get;   //            存放已经下载过的图片的URL
	vector<string> img_URL;
	vector<string> com_URL;

public:
	webSpider(const string &_beginURL) : beginURL(_beginURL) {}
	~webSpider() {}
	void getImgByBFS(void);
private:
	void getHTML(const string &_url);
	void regexImgAndURL(void);
	void saveImg(const string &_img_url, const string &_host);
};

void webSpider::getImgByBFS(void) {
	queue<string> que_url;
	string cur_url;
	vector<string>::iterator iter;

	que_url.push(beginURL);
	while (!que_url.empty()) {
		cur_url = que_url.front();
		que_url.pop();
		is_exit[cur_url]++;

		getHTML(cur_url);
		
		regexImgAndURL();
		
		for (iter = img_URL.begin(); iter != img_URL.end(); ++iter)
			saveImg(*iter, cur_url);
		img_URL.clear();

		for (iter = com_URL.begin(); iter != com_URL.end(); ++iter) {
			if (is_exit[*iter] == 0)
				que_url.push(*iter);
		}
		com_URL.clear();
	}
}

void webSpider::getHTML(const string &_url) {
	int i = 0;
	string url_name;
	string cmd = "wget -O index.html ";  //获取种子URL页面所有资源
	cmd += _url;
	//cmd += "http://www.php.cn/course/37.html";
	system(cmd.c_str());

	cmd = "mv";    //rename
	cmd += " index.html";
	cmd += " ./tmp.txt";
	system(cmd.c_str());
	//exit(0);
}

void webSpider::saveImg(const string &_img_url, const string &_host) {
	static int x = 0;
	int n = 0;
	int i = 0;
	string format;
	string img_name;

	for(i = _img_url.length() - 4; i<_img_url.length(); ++i)
		format.push_back(_img_url[i]);
	for(i = _img_url.length() - 5; ; --i)
		if(_img_url[i] == '/')
			break;
	++i;
	for(; i<_img_url.length() - 4; ++i)
		img_name.push_back(_img_url[i]);
	img_name = img_name + format;
	
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

void webSpider::regexImgAndURL(void) {
	smatch mat;       
	regex  expr_img("src=\"(.*(png|svg|jpg))\"");
	//regex  expr_url("[a-zA-Z0-9][-a-zA-Z0-9]{0,62}(/.[a-zA-Z0-9][-a-zA-Z0-9]{0,62})+/.?");
	regex  expr_url("href=\"(http://[^\\s'\"]+)\"");
	string line_buf;
	bool too_long = 0;
	
	ifstream file("./tmp.txt");
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

using namespace std;

int main(void) {
	string beginURL = "https://www.zhipin.com/";
	//string beginURL = "http://www.php.cn/";
	
	if (mkdir("./img", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
		errExit("mkdir error!");
	webSpider myspider(beginURL);

	myspider.getImgByBFS();

	return 0;
}