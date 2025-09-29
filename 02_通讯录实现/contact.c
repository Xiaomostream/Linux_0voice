/*
一.通讯录功能： 
    1.添加一个人员 
    2.打印所有人员 
    3.删除一个人员 
    4.查找一个人员 
    5.保存文件 
    6.加载文件 
二.产品方案：
    人员存储的数据结构：链表
    人员的信息：姓名，电话
    文件存储的格式：name: xxx,phone: xxx
业务层：业务逻辑
接口层：add,del,search,travel,打包，解包
支持层：链表的增删查，read, write

三.编码实现顺序：
    定义链表、通讯录 => 定义链表插入/删除操作 支持层 => 定义接口层、业务层

四.关于文件的操作：
    说明书： man 3 fopen
    fopen:      FILE *fp = fopen(filename, "w") => 读写文件，"w"/"r"
    fprintf:    fprintf(fp, "name: %s,phone: %s\n", item->name, item->phone) => 使用fprintf写入缓存里
    fflush:     fflush(fp) => 将缓存信息加载入fp文件流指向的filename里
    fgetc:      int fgetc(fp) => 读入一个字符，读到文件末尾，返回EOF
    feof:       feof() tests the end-of-file indicator for the stream pointed to by stream, returning nonzero if it is set.  
    fgets:      char *fgets(char *s, int size, FILE *stream) => 读入一行字符
    fclose:     fclose(fp) 关闭文件流
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_LENGTH     16
#define PHONE_LENGTH    32
#define BUFFER_LENGTH   128
#define MIN_TOKEN_LENGTH 5

#define INFO            printf

//定义插入操作
//这里遇到一个小bug， 由于下面实现的时候， list传入的是一个二级指针， 
//如果list->prev，由于->优先级是低于list这个*的，故应该使用小括号表示我们的优先级含义 (list)->prev
#define LIST_INSERT(item, list) do{ \
    item->next = list;              \
    item->prev = NULL;              \
    if(list != NULL) (list)->prev = item; \
    list = item;                    \
} while(0)

//定义删除操作
#define LIST_REMOVE(item, list) do{                        \
    if(item->prev != NULL) item->prev->next = item->next;  \
    if(item->next != NULL) item->next->prev = item->prev;  \
    if(item == list) list = (list)->next;                    \
    item->next = item->prev = NULL;                        \
} while(0)

//定义个人信息
struct person{ 
    char name[NAME_LENGTH];
    char phone[PHONE_LENGTH];
    struct person *prev;
    struct person *next;
};

//定义通讯录
struct contacts{
    struct person *people;
    int count;
};

//枚举定义操作数
enum {
    OPER_INSERT = 1,
    OPER_PRINT,
    OPER_DEL,
    OPER_SEARCH,
    OPER_SAVE,
    OPER_LOAD
};

//定义架构接口层 

//使用二级指针，进行修改操作
int person_insert(struct person **ppeople, struct person *ps) {
    if(ps == NULL) return -1;
    LIST_INSERT(ps, *ppeople);
    return 0;
}

int person_delete(struct person **ppeople, struct person *ps) {
    if(ps == NULL) return -1;
    if(ppeople == NULL) return -2;
    LIST_REMOVE(ps, *ppeople);
    return 0;
}

struct person* person_search(struct person *people, const char *name) {
    struct person *item = NULL;
    for (item = people; item != NULL; item = item->next) {
        if(strcmp(item->name, name) == 0)
            break;
    }
    return item;
}

int person_travel(struct person *people) {
    int ct = 0;
    for (struct person *item = people; item != NULL; item = item->next) {
        // name: xxx,phone: xxx
        INFO("%d > name: %s,phone: %s\n", ++ct, item->name, item->phone);
    }
    return 0;
}

int save_file(struct person *people, const char *filename) {
    //把当前所有人员写入filename里
    FILE *fp = fopen(filename, "w");
    for (struct person *item = people; item != NULL; item = item->next) {
        // 使用fprintf写入缓存里
        fprintf(fp, "name: %s,phone: %s\n", item->name, item->phone);
        // 将缓存信息加载入filename里
        fflush(fp); 
    }
    fclose(fp);
}

int parser_token(char *buffer, int length, char *name, char *phone) {
    //以' '和','来作为name的首尾标识符 以' '读到最后作为phone的读取策略
    if(length < MIN_TOKEN_LENGTH) return -2;

    int i = 0, j = 0, status = 0;
    for (i = 0; i < length && buffer[i] != ','; i ++ ) {
        if(buffer[i] == ' ') {
            status = 1;
        } else if(status == 1) {
            name[j ++] = buffer[i];
        }
    }
    j = 0;
    status = 0;
    for (; i < length; i ++ ) {
        if(buffer[i] == ' ') {
            status = 1;
        }   else if(status == 1 && buffer[i] != '\n') {
            phone[j ++] = buffer[i];
        }
    }
    INFO("## File Token: %s %s\n", name, phone);
    return 0;
}
int load_file(struct person **ppeople, const char *filename, int *count) {
    //从filename文件里，加载人员信息至people这个链表里，并修改通讯录人数count
    FILE *fp = fopen(filename, "r");
    if(fp == NULL) return -1;

    while(!feof(fp)) {
        char buffer[BUFFER_LENGTH] = {0};
        fgets(buffer, BUFFER_LENGTH, fp); //读入一行
        int len = strlen(buffer);
        INFO("##length : %d\n", len);
        //name: xxx,phone: xxx
        char name[NAME_LENGTH] = {0};
        char phone[PHONE_LENGTH] = {0};
        if(parser_token(buffer, len, name, phone) != 0) {
            //buffer不够MIN_TOKEN_LENGTH
            continue;
        } 
        struct person* p = (struct person*)malloc(sizeof(struct person));
        if(p == NULL) return -2;
        // p->name = name, p->phone = phone;
        memcpy(p->name, name, NAME_LENGTH);
        memcpy(p->phone, phone, PHONE_LENGTH);
        person_insert(ppeople, p);
        (*count) ++;
        //添加成功
    }
    fclose(fp);
    return 0;
}
// 接口层 END

// 开始实现业务层:逻辑实现
int insert_entry(struct contacts *cts) {
    //插入一个人员，需要用户输入name,phone，然后使用接口层的插入操作
    if(cts == NULL) return -1;

    //定义一个待插入的person成员
    struct person *p = (struct person*)malloc(sizeof(struct person));
    if(p == NULL) return -2;
    memset(p, 0, sizeof(struct person)); //初始化
    
    INFO("Please Input Name\n");
    scanf("%s", p->name);
    INFO("Please Input Phone\n");
    scanf("%s", p->phone);
    
    //int person_insert(struct person **ppeople, struct penson *ps) 插入
    if(person_insert(&cts->people, p) != 0) {
        free(p);
        return -3;
    }
    cts->count ++;
    INFO("Insert Successful!\n");
    return 0;
}
int print_entry(struct contacts *cts) {
    if(cts == NULL) return -1;
    INFO("现有的人数: %d\n", cts->count);
    person_travel(cts->people);
    return 0;
}
int delete_entry(struct contacts *cts) {
    //删除一个人员，需要用户输入name，然后先search该人员节点，然后delete该节点
    if(cts == NULL) return -1;

    INFO("Please Input Name\n");
    char name[NAME_LENGTH] = {0};
    scanf("%s", name);

    // struct person* person_search(struct person *people, const char *name); 查找
    struct person* ps = person_search(cts->people, name);
    if(ps == NULL) {
        INFO("Person don't exist\n");
        return -2;
    }
    // int person_delete(struct person **ppeople, struct person *ps); 删除
    INFO("name: %s,phone: %s\n", ps->name, ps->phone);
    if(person_delete(&cts->people, ps) != 0) {
        INFO("Delete Error\n");
        return -2;
    }
    free(ps);
    cts->count --;
    INFO("Delete Successful!\n");
    return 0;
}

int search_entry(struct contacts *cts) {
    //查找一个人员，需要用户输入name，然后先search该人员节点，然后输出
	if (cts == NULL) return -1;

	INFO("Please Input Name : \n");
	char name[NAME_LENGTH] = {0};
	scanf("%s", name);

	// person
	struct person *ps = person_search(cts->people, name);
	if (ps == NULL) {
		INFO("Person don't Exit\n");
		return -2;
	}

	INFO("name: %s,phone: %s\n", ps->name, ps->phone);

	return 0;
}

int save_entry(struct contacts *cts) {
    if(cts == NULL) return -1;
    INFO("Please Input filename\n");
    char filename[NAME_LENGTH] = {0};
    scanf("%s", filename);
    save_file(cts->people, filename);
    return 0;
}

int load_entry(struct contacts *cts) {
    if(cts == NULL) return -1;
    INFO("Please Input filename\n");
    char filename[NAME_LENGTH] = {0};
    scanf("%s", filename);
    load_file(&cts->people, filename, &cts->count);
    return 0;
}
void menu_info() {

	INFO("\n\n********************************************************\n");
	INFO("***** 1. Add Person\t\t2. Print People ********\n");
	INFO("***** 3. Del Person\t\t4. Search Person *******\n");
	INFO("***** 5. Save People\t\t6. Load People *********\n");
	INFO("***** Other Key for Exiting Program ********************\n");
	INFO("********************************************************\n\n");

}

int main() {
    struct contacts *cts = (struct contacts*)malloc(sizeof(struct contacts));
    if(cts == NULL) return -1;
    memset(cts, 0, sizeof(struct contacts));
    while(1) {
        menu_info();
        int op;
        scanf("%d", &op);
        switch (op) {
            case OPER_INSERT:
                insert_entry(cts);
                break;
            case OPER_PRINT:
                print_entry(cts);
                break;
            case OPER_DEL:
                delete_entry(cts);
                break;
            case OPER_SEARCH:
                search_entry(cts);
                break;
            case OPER_SAVE:
                save_entry(cts);
                break;
            case OPER_LOAD:
                load_entry(cts);
                break;
            default:
                goto exit;
        }
    }

exit:
    free(cts);
    return 0;
}