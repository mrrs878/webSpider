#include <iostream>
#include <string>
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
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

void errExit(const string &_errinfo) {
	cout << _errinfo << endl;
	exit(1);
}

class webSpider {
private:
	string beginURL;
	char host[512];
	char repath[512];
	string all_html;
	unordered_map<string, int> is_exit;
	vector<string> img_URL;
	vector<string> com_URL;
	int conn_fd;
	sockaddr_in serv_addr;
	
public:
	webSpider(const string &_beginURL) : beginURL(_beginURL), all_html(""), conn_fd(-1) {
		bzero(host, sizeof(host));
		bzero(repath, sizeof(repath));
	}
	~webSpider() {}
	void connServer(void);
	void getImgByBFS(void);
private:
	void analyHost(const string &_url);
	void analyURL(void);
	void regexGetImg(void);
	void regexGetCom(void);
	void saveImg(const string &_img_url);
};

void webSpider::getImgByBFS(void) {
	queue<string> que_url;
	string cur_url;
	vector<string>::iterator iter;
	
	que_url.push(beginURL);
	while(!que_url.empty()) {
		cur_url = que_url.front();
		que_url.pop();
		is_exit[cur_url]++;
		analyHost(cur_url);
		connServer();
		analyURL();
		for(iter = img_URL.begin(); iter != img_URL.end(); ++iter)
			saveImg(*iter);
		img_URL.clear();
		
		for(iter = com_URL.begin(); iter != com_URL.end(); ++iter) {
			if(is_exit[*iter] == 0)
				que_url.push(*iter);
		}
		com_URL.clear();
	}
}

void webSpider::analyHost(const string &_url) {
	string url = _url;
	
	char *pos = strstr(const_cast<char*>(url.c_str()), "http://");
	if(pos == NULL)
		return;
	else 
		pos += 7;
	sscanf(pos, "%[^/]%s", host, repath);
	cout << "host: " << host << "  repath: " << repath << endl;
}

void webSpider::analyURL(void) {
	int n = 0;
	char buf[1024];
	
	bzero(buf, sizeof(buf));
	while((n = recv(conn_fd, buf, sizeof(buf) - 1, 0)) > 0) {
		buf[n] = '\0';
		all_html += string(buf);
	}
	regexGetImg();
	regexGetCom();
}

void webSpider::saveImg(const string &_img_url) {
	int n = 0;
	string img_name;

	analyHost(_img_url);
	connServer();
	
	img_name.resize(_img_url.size());
	for(int k=0, i=0; i < _img_url.length(); ++i) {
		char ch = _img_url[i];
		if(ch != '\\' && ch != '/' && ch != ':' && ch != '*' && ch != '?' 
		   && ch != '"' && ch != '<' && ch != '>' && ch != '|')
			img_name[k++] = ch;
	}
	img_name = "./img/" + img_name + ".jpg";
	
	fstream file;
	file.open(img_name, ios::out | ios::binary);
	char buf[1024];
	bzero(buf, sizeof(buf));
	n = recv(conn_fd, buf, sizeof(buf) - 1, 0);
	char *pos = strstr(buf, "\r\n\r\n");
	
	file.write(pos + strlen("\r\n\r\n"), n - (pos - buf) - strlen("\r\n\r\n"));
	while((n = recv(conn_fd, buf, sizeof(buf) - 1, 0)) > 0)
		file.write(buf, n);
	file.close();
}

void webSpider::regexGetImg(void) {
	smatch mat;
	regex  expr("src=\"(.*?\\.jpg)\"");
	string::const_iterator start = all_html.begin();
	string::const_iterator end   = all_html.end();
	
	while(regex_search(start, end, mat, expr)) {
		string msg(mat[1].first, mat[1].second);
		img_URL.push_back(msg);
		start = mat[0].second;
	}
}

void webSpider::regexGetCom(void) {
	smatch mat;
	regex expr("href=\"(http://[^\\s'\"]+)\"");
	string::const_iterator start = all_html.begin();
	string::const_iterator end = all_html.end();
	
	while (regex_search(start, end, mat, expr)) {
		string msg(mat[1].first, mat[1].second);
		com_URL.push_back(msg);
		start = mat[0].second;
	}
}

void webSpider::connServer(void) {
	if((conn_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		errExit("socket error");
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(80);

	struct hostent *p = gethostbyname(host);
	if(p == NULL)
		errExit("gethostbyname error!");
	//inet_pton(AF_INET, p->h_addr, &serv_addr.sin_addr);
	memcpy(&serv_addr.sin_addr, p->h_addr, 4);
	
	if(connect(conn_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0)
		errExit("connnect error!");
	
	string reqGET = "GET " + string(repath) + " HTTP/1.1\r\nHost: " + string(host) + "\r\n" + "Connection:Close\r\n\r\n";
	if(send(conn_fd, reqGET.c_str(), reqGET.size(), 0) < 0) {
		errExit("send error!");
		close(conn_fd);
	}
}

using namespace std;

int main(void) {
	string beginURL = "http://www.178linux.com/2739";
	
	if(mkdir("./img", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
		errExit("mkdir error!");
	webSpider myspider(beginURL);
	
	myspider.getImgByBFS();
	
	return 0;
}