#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <regex>
#include <algorithm>
#include <fstream>
#include <sstream>
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
#include <openssl/ssl.h>
#include <openssl/err.h>

#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")
#pragma comment(lib, "ws2_32.lib")

using namespace std;

void errExit(const string &_errinfo) {
	cout << _errinfo << endl;
	exit(1);
}

class webSpider {
private:
	bool protocol;  //0: http	 1:https
	string beginURL;
	char host[512];
	char repath[512];
	string all_html;
	unordered_map<string, int> is_exit;
	vector<string> img_URL;
	vector<string> com_URL;
	SSL_CTX *ctx;
	SSL *ssl;	
	int conn_fd;
	sockaddr_in serv_addr;

public:
	webSpider(const string &_beginURL) : beginURL(_beginURL), protocol(1), conn_fd(-1) {
		bzero(host, sizeof(host));
		bzero(repath, sizeof(repath));
	}
	~webSpider() {}
	void getImgByBFS(void);
private:
	void connServerByHttp(void);
	void connServerByHttps(void);
	void analyHost(const string &_url);
	void getAllURL(void);
	void regexGetImg(void);
	void regexGetCom(void);
	void downloadSaveImg(const string &_img_url);
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

		getAllURL();

		regexGetImg();
		regexGetCom();
	
		for (iter = img_URL.begin(); iter != img_URL.end(); ++iter)
			downloadSaveImg(*iter);
		img_URL.clear();

		for (iter = com_URL.begin(); iter != com_URL.end(); ++iter) {
			if (is_exit[*iter] == 0)
				que_url.push(*iter);
		}
		com_URL.clear();
	}
}

void webSpider::analyHost(const string &_url) {
	string url = _url;

	char *pos = strstr(const_cast<char*>(url.c_str()), "https://");
	if (pos == NULL) {  
		if((pos = strstr(const_cast<char*>(url.c_str()), "http://")) == NULL)  
			return;
		else {  
			protocol = 0;  //http
			pos += 7;
		}
	}
	else {
		protocol = 1;  //https
		pos += 8;
	}
	sscanf(pos, "%[^/]%s", host, repath);
	cout << "host: " << host << "  repath: " << repath << endl;
}

void webSpider::getAllURL(void) {
	analyHost(cur_url);
	if(protocol)
		connServerByHttps();
	else
		connServerByHttp();
	
	if(protocol) {
		int res = 0;
		char recv_buf[1024];
		bzero(recv_buf, sizeof(recv_buf));
		while((res = SSL_read(ssl, recv_buf, sizeof(recv_buf - 1))) > 0) {
			recv_buf[res] = '\0';
			all_html += string(recv_buf);
			bzero(recv_buf, sizeof(recv_buf));
		}
		SSL_shutdown(ssl);
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		close(conn_fd);
	}
	else {
		int n = 0;
		char recv_buf[1024];
		bzero(recv_buf, sizeof(recv_buf));
		while ((n = recv(conn_fd, recv_buf, sizeof(recv_buf) - 1, 0)) > 0) {
			recv_buf[n] = '\0';
			all_html += string(recv_buf);
			bzero(recv_buf, sizeof(recv_buf));
		}
		close(conn_fd);
	}
}

void webSpider::downloadSaveImg(const string &_img_url) {
	int res = 0;
	int i = 0;
	string format;
	string img_name;
	char recv_buf[1024];
	
	for(i = _img_url.length() - 4; i<_img_url.length(); ++i)
		format.push_back(_img_url[i]);
	for(i = _img_url.length() - 5; ; --i)
		if(_img_url[i] == '/')
			break;
	++i;
	for(; i<_img_url.length() - 4; ++i)
		img_name.push_back(_img_url[i]);
	img_name = "./img/" + img_name + format;
	
	fstream file;
	file.open(img_name, ios::out | ios::binary);
	char buf[1024];
	bzero(buf, sizeof(buf));
	
	analyHost(_img_url);
	
	if(protocol) {
		string tmp;
		char x;
		
		connServerByHttps();
		bzero(recv_buf, sizeof(recv_buf));
		while(SSL_read(ssl, &x, 1) > 0) {  //response info ignore
			tmp.push_back(x);
			if(tmp.find("\r\n\r\n") != -1)
				break;
		}
		if(tmp.find("200 OK")) {  //成功连接
			while((res = SSL_read(ssl, recv_buf, sizeof(recv_buf - 1))) > 0) {  //file 
				file.write(recv_buf, res);
				bzero(recv_buf, sizeof(recv_buf));
			}
		}
		file.close();
		SSL_shutdown(ssl);
		SSL_free(ssl);
		SSL_CTX_free(ctx);
		close(conn_fd);
	}
	else {
		connServerByHttp();
		res = recv(conn_fd, recv_buf, sizeof(recv_buf) - 1, 0);
		char *pos = strstr(recv_buf, "\r\n\r\n");
		file.write(pos + strlen("\r\n\r\n"), res - (pos - recv_buf) - strlen("\r\n\r\n"));
		while ((res = recv(conn_fd, recv_buf, sizeof(recv_buf) - 1, 0)) > 0) {
			file.write(recv_buf, res);
			bzero(recv_buf, sizeof(recv_buf));
		}
		file.close();
		close(conn_fd);
		
	}
}

void webSpider::regexGetImg(void) {
	smatch mat;       
	regex  expr("src=\"(.*(png|svg|jpg))\"");
	string::const_iterator start = all_html.begin();
	string::const_iterator end = all_html.end();

	while (regex_search(start, end, mat, expr)) {
		string msg = mat[1].str();
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

void webSpider::connServerByHttp(void) {
	if ((conn_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		errExit("socket error");
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(80);

	struct hostent *p = gethostbyname(host);
	if (p == NULL)
		errExit("gethostbyname error!");
	//inet_pton(AF_INET, p->h_addr_list[0], &serv_addr.sin_addr);
	memcpy(&serv_addr.sin_addr.s_addr, p->h_addr, 4);

	if (connect(conn_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0)
		errExit("connnect error!");

	string reqGET = "GET " + string(repath) + " HTTP/1.1\r\nHost: " + string(host) + "\r\n" + "Connection:Close\r\n\r\n";
	if (send(conn_fd, reqGET.c_str(), reqGET.size(), 0) < 0) {
		errExit("send error!");
		close(conn_fd);
	}
}

void webSpider::connServerByHttps(void) {
	const SSL_METHOD *meth = SSLv23_client_method();  //客户端，服务端选择SSLv23_server_method
	SSL_load_error_strings();  //加载SSL错误信息
	SSL_library_init();  //初始化ssl算法库
	SSLeay_add_all_algorithms(); //加载SSL的加密/Hash算法

	ctx = SSL_CTX_new(meth);  //建立新的SSL上下文
	if (ctx == NULL) {
		ERR_print_errors_fp(stderr);
		errExit("SSL_CTX_new error!");
	}

	/*建立tcp连接*/
	if ((conn_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		errExit("socket error!");
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(443);

	struct hostent *p = gethostbyname(host);
	if (p == NULL)
		errExit("gethostbyname error!");
	memcpy(&serv_addr.sin_addr.s_addr, p->h_addr, 4);
	if(connect(conn_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))
		errExit("connect error!");
	
	/*建立SSL*/
	int res;
	ssl = SSL_new(ctx);
	if (ssl == NULL)
		errExit("SSL_new error!");
	SSL_set_fd(ssl, conn_fd);  //将SSL与tcp连接
	if ((res = SSL_connect(ssl)) == -1)
		errExit("SSL_connect error!");

	string tmp = "GET " + string(repath) + " HTTP/1.1\r\n" 
			   + "Host: " + string(host) + "\r\n"
			   + "Content-Type: text/html; charset=UTF-8\r\n" 
			   + "Connection:Close\r\n\r\n";
	const char *send_data = tmp.c_str();
	if ((res = SSL_write(ssl, send_data, strlen(send_data))) == -1)
		errExit("SSL_write error!");
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
