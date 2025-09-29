/*
作业：实现统计单词个数
b.txt含有中文引号

*/
#include <stdio.h>
#include <iostream>
#include <string>
#include <map>
#define IN      1
#define OUT     0

using namespace std;

int bool_split(char c) {
    if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        return 0;
    else 
        return 1;
}
int count_word(char *filename) {
    int res = 0;
    int status = OUT; //初始为OUT态
    FILE *fp = fopen(filename, "r");
    if(fp == NULL) return -1; //读入失败
    char c;
    map<string, int> mp;
    string str = "";
    while((c = getc(fp)) != EOF) {
        if(bool_split(c)) {
            if(status == IN) {
                if(str.size() > 0) {
                    mp[str] ++;
                    str = "";
                }
            }
            status = OUT;
        }
        else {
            if(status == OUT) res ++;
            str.push_back(c);
            status = IN;
        }   
    }
    if(str.size() > 0) mp[str] ++;

    printf("各单词出现的次数\n");
    for (map<string,int>::iterator it=mp.begin(); it!=mp.end(); it ++) {
        std::cout<<it->first<<' '<<it->second<<'\n';
    }
    return res;
}
int main(int argc, char *argv[]) {
    //文件名作为main函数的传参
    if(argc < 2) return -1; //未传入文件名
    int res = count_word(argv[1]);
    printf("单词总个数: %d\n", res);
    return 0;
}