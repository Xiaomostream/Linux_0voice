/*
目标是记录文件b.txt中的单词数，将该任务当作状态机
1.字符无非就两种状态，要么是分隔符，要么是单词中的字母
    如果当前是分割符，记录状态为 OUT
    否则就是单词，记录状态为 IN，只需要记录从 OUT->IN 的次数，因为单词的个数其实就是每个单词的首字符的出现个数
2.fopen:
    FILE *fopen(const char *pathname, const char *mode);
    Upon successful completion fopen(), fdopen(), and freopen() return a FILE pointer.  
    Otherwise, NULL is returned and errno is set to indicate the error.
3.fgetc:
    int fgetc(FILE *stream);
    return the character read as an unsigned char cast to an int or EOF on end of file or error.
*/
#include <stdio.h>
#define IN      1
#define OUT     0

int bool_split(char c) {
    if(c == ' ' || c == ',' || c == '.' || c == '!' || c == ';' ||
            c == '\'' || c == '\"' || c == '-' ||
            c == '\n' || c == '\t' || c == '+' ||
            c == '{' || c == '}' || c == '(' || 
            c == ')' || c == '[' || c == ']')
       return 1;
    else 
        return 0;
}
int count_word(char *filename) {
    int res = 0;
    int status = OUT; //初始为OUT态
    FILE *fp = fopen(filename, "r");
    if(fp == NULL) return -1; //读入失败
    char c;
    while((c = getc(fp)) != EOF) {
        if(bool_split(c)) {
            status = OUT;
        }
        else {
            if(status == OUT) res ++;
            status = IN;
        }   
    }
    return res;
}
int main(int argc, char *argv[]) {
    //文件名作为main函数的传参
    if(argc < 2) return -1; //未传入文件名
    int res = count_word(argv[1]);
    printf("word: %d\n", res);
    return 0;
}