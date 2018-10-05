# webSpider

webspider-wget与另外两个的区别：webspider-wget较简单，程序中主要使用wget命令来进行网页数据以及图片的下载，没有涉及socket API

webspider-base：最开始的版本，有很多bug，留下来只是为了纪念一下

webspider-updatex：后续更新版本

compile:
  0：webspider/webspider-bkp
  /usr/local/bin/g++ -std=c++11 -g -o webspider webspider-bkp.cpp -lssl -lcrypto
  1：webspider-wget
  /usr/local/bin/g++ -std=c++11 -g -o webspider webspider-wget.cpp
