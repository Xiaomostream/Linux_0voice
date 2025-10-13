# Mysql5.7的安装、配置、卸载
## 安装Mysql5.7
由于我的虚拟机版本为Ubuntu22.04, 不能通过apt-get安装，只能通过wget下载5.7资源包，编译安装。
参考链接：https://www.cnblogs.com/DingyLand/p/17466734.html
```
# 更新apt源
sudo apt-get update 
sudo apt-get upgrade
# 下载mysql5.7的tar包
wget https://downloads.mysql.com/archives/get/p/23/file/mysql-server_5.7.36-1ubuntu18.04_amd64.deb-bundle.tar
# 解压
tar xvf ./mysql-server_5.7.36-1ubuntu18.04_amd64.deb-bundle.tar
# 安装依赖(cd 到解压的路径)
sudo apt-get install ./libmysql*
sudo apt-get install libtinfo5
# 安装客户端和服务端 （安装过程会提示设置MySQL的密码，用户名默认root）
sudo apt-get install ./mysql-community-client_5.7.36-1ubuntu18.04_amd64.deb
sudo apt-get install ./mysql-client_5.7.36-1ubuntu18.04_amd64.deb
sudo apt-get install ./mysql-community-server_5.7.36-1ubuntu18.04_amd64.deb
sudo apt-get install ./mysql-server_5.7.36-1ubuntu18.04_amd64.deb 
# 启动Mysql
systemctl status mysql.service
```
## 配置远程连接
由于默认情况下，不能直接对root进行远程连接，可以通过查看mysql下的user表进行`select Host, User from user`来查看权限，我们可以创建一个admin用户，并可以让任意用户访问`'admin'@'%'`。
```
# 登录mysql的root用户
mysql -u root -p
# 创建一个admin用户，密码为root, 且允许任意用户来进行访问
CREATE USER 'admin'@'%' IDENTIFIED BY 'root';
# 赋予admin用户所有权限(相当于root)
GRANT ALL PRIVILEGES ON *.* TO 'admin'@'%';
FLUSH PRIVILEGES;
```
接着需要配置一下mysqld.cnf
```
sudo vim /etc/mysql/mysql.conf.d/mysqld.cnf

# 追加或修改bind，0.0.0.0表示所有ip都可以进行访问
[mysqld]
bind-address = 0.0.0.0

# 重启MySQL
sudo systemctl restart mysql
```
## Mysql的卸载

```
# 步骤一：停止MySQL服务
sudo systemctl stop mysql
# 步骤二：卸载MySQL服务器及其相关包。
sudo apt-get remove --purge mysql-server mysql-client mysql-common
# 这条命令会移除MySQL服务器、客户端和公共包，并且使用--purge选项确保删除配置文件。

# 步骤三：清理残留的配置和数据文件
sudo rm -rf /etc/mysql
sudo rm -rf /var/lib/mysql

# 步骤四：清理未使用的依赖包
sudo apt-get autoremove
sudo apt-get autoclean
# 步骤五：检查系统中是否还有遗留的MySQL包
dpkg -l | grep -i mysql

# 如果发现有遗留的包，可以使用以下命令手动删除, 将<package-name>替换为实际发现的包名。
sudo dpkg -P <package-name>

# 步骤六：删除MySQL的日志文件
sudo rm -rf /var/log/mysql*

# 步骤七：重启系统
sudo reboot

# 重启系统后，再次检查MySQL服务是否还在运行：
sudo systemctl status mysql
# 如果显示服务未找到或未运行，说明MySQL已被成功卸载。
```

# SQL语句
```
show databases;

use mysql;

show tables;

select * from user;

create user 'test'@'%' identified by 'root';

select * from user;

DROP DATABASE XIAOMO_DB; # 删除数据库

CREATE DATABASE XIAOMO_DB; # 创建数据库

SHOW DATABASES;  # 显示所有数据库

USE XIAOMO_DB; # 使用数据库

CREATE TABLE TBL_USER (# 创建USER表
	U_ID INT PRIMARY KEY AUTO_INCREMENT,
	U_NAME VARCHAR(32),
	U_GENDER VARCHAR(8)
);

SHOW TABLES; # 显示所有表

DESC TBL_USER; # 显示表结构

INSERT TBL_USER(U_NAME, U_GENDER) VALUES('Xiaomo', 'MAN'); # 插入

SELECT * FROM TBL_USER; # 查询表中的所有数据

# 定义存储过程, 以 标识符 $$ 为结束符
DELIMITER $$
CREATE PROCEDURE PROC_DELETE_USER(IN UNAME VARCHAR(32))
BEGIN
SET SQL_SAFE_UPDATES=0;
DELETE FROM TBL_USER where U_NAME=UNAME;
SET SQL_SAFE_UPDATES=1;
END$$

CALL PROC_DELETE_USER('KING');

# BLOB（Binary Large OBject）是用于存储二进制数据的一种数据类型，它是一个可以存储大量数据的容器，能够容纳不同大小的数据。
# BLOB类型主要用于存储那些可能超出常规大小的数据，如图片、音频、视频等二进制数据。
ALTER TABLE TBL_USER ADD U_IMG BLOB;

# 修改数据类型为更长的MEDIUMBLOB(可存储16Mb)
DESC TBL_USER;
ALTER TABLE TBL_USER MODIFY COLUMN U_IMG MEDIUMBLOB;
DESC TBL_USER;
```
# Mysql实现图片的存储
总体思路：实现图片存储(mysql_write/mysql_read)
1) 准备好图片，并且将图片read
2) mysql_write_image
3) mysql_read_image
4) 写入磁盘
## 预定义的宏
```
#define XIAOMO_DB_SERVER_IP "192.168.66.130"
#define XIAOMO_DB_SERVER_PORT 3306
#define XIAOMO_DB_USERNAME "admin"
#define XIAOMO_DB_PASSWD "root"
#define XIAOMO_DB_DEFAULTDB "XIAOMO_DB"

#define SQL_INSERT_TBL_USER "INSERT TBL_USER (U_NAME, U_GENDER) VALUES ('Lee', 'WOMAN');"
#define SQL_SELECT_TBL_USER "SELECT * FROM TBL_USER;"
#define SQL_DELETE_TBL_USER "CALL PROC_DELETE_USER('Lee');"

#define SQL_INSERT_IMG_USER "INSERT TBL_USER (U_NAME, U_GENDER, U_IMG) VALUES ('KING', 'WOMAN', ?);"
#define SQL_SELECT_IMG_USER "SELECT U_IMG FROM TBL_USER WHERE U_NAME='KING';"

#define FILE_IMAGE_LENGTH (888888)        
```
## mysql的连接与查询操作
首先定义MYSQL类型的数据对象，然后通过mysql_real_connect函数进行连接，连接成功后方可使用 mysql_real_query(SQL语句)执行代码
```
    MYSQL mysql;

    // 对mysql初始化
    if(NULL == mysql_init(&mysql)) {
        printf("mysql_init : %s\n", mysql_error(&mysql));
        return -1;
    }

    // mysql连接时，返回0，不成功
    if(!mysql_real_connect(&mysql, XIAOMO_DB_SERVER_IP, XIAOMO_DB_USERNAME, 
        XIAOMO_DB_PASSWD, XIAOMO_DB_DEFAULTDB, XIAOMO_DB_SERVER_PORT, NULL, 0)) {
        printf("mysql_real_connect : %s\n", mysql_error(&mysql));
    }

    // mysql查询时，返回0，成功
    
    // INSERT 操作
    if(mysql_real_query(&mysql, SQL_INSERT_TBL_USER, strlen(SQL_INSERT_TBL_USER))) {
        printf("mysql_real_query : %s\n", mysql_error(&mysql));
        goto Exit;
    }

    // DELETE 操作
    if(mysql_real_query(&mysql, SQL_DELETE_TBL_USER, strlen(SQL_DELETE_TBL_USER))) {
        printf("mysql_real_query : %s\n", mysql_error(&mysql));
        goto Exit;
    }
```
## 查询SQL并使用API读取数据
**思路：**
1）Node Server --<select SQL语句>-> DB server
2）DB server 通过管道返回给 Node Server查询数据
3）保存查询结果 MYSQL_RES *res = mysql_store_result(handle)
4）按照行打印结果 row = mysql_fetch_row(handle), row[i]是第i列
```
int xiaomo_mysql_select(MYSQL *handle) {
    // mysql_real_query --> 执行SQL语句，结果存储在管道里 
    if(mysql_real_query(handle, SQL_SELECT_TBL_USER, strlen(SQL_SELECT_TBL_USER))) {
        printf("mysql_real_query : %s\n", mysql_error(handle));
        return -1;
    }
    
    // mysql_store_result--> 拿到存储结果集合 res
    MYSQL_RES *res = mysql_store_result(handle);
    if(res == NULL) {
        printf("mysql_store_result : %s\n", mysql_error(handle));
        return -2;
    }

    // rows / fileds --> res有多少行多少列
    int rows = mysql_num_rows(res);
    int fields = mysql_num_fields(res);
    printf("(rows,fields) = (%d,%d)\n", rows, fields);

    // fetch 
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res))) { //如果读取为空，while循环退出
        for (int i = 0; i < fields; i ++) {
            printf("%s\t", row[i]);
        }
        printf("\n");
    }
    // 释放
    mysql_free_result(res);

    return 0;
}
```
## 读取图片与写入图片
通过文件操作来进行图片文件的读写
```
// 读取图片，并把结果存储到buffer中
int read_image(char *filename, char *buffer) {
    if(filename == NULL || buffer == NULL) return -1;

    FILE *fp = fopen(filename, "rb");
    if(fp == NULL) {
        printf("fopen failed\n");
        return -2;
    }

    // 通过fseek使得，文件指针指到文件末尾，以获得文件中有多少字节数据
    fseek(fp, 0, SEEK_END);
    // 通过fp偏移量获得filesize
    int length = ftell(fp);
    // 再次把fp置为开始
    fseek(fp, 0, SEEK_SET);

    // fread: 从文件指针fp开始，每次读一个字节，共读length次，结果放入buffer里
    int size = fread(buffer, 1, length, fp);
    if(size != length) {
        printf("fread failed: %d\n", size);
        return -3;
    }
    fclose(fp);

    return size;
}

int write_image(char *filename, char *buffer, int length) {
    if(filename == NULL || buffer == NULL || length <= 0) return -1;
    FILE *fp = fopen(filename, "wb+"); //wb : 只写模式，+ : 如果没有该文件 创建
    if(fp == NULL) {
        printf("fopen failed\n");
        return -2;
    }

    int size = fwrite(buffer, 1, length, fp);  // 写的数据来自buffer，写到fp里，每次写1个自己，共写length次
    if(size != length) {
        printf("fwrite failed: %d\n", size);
        return -3;
    }

    fclose(fp);
    return size;
}

```
## mysql读写图片
前置工作：由于图片本质就是二进制文件，故可以把文件内容看作一个`BLOB`二进制来进行Mysql实际存储的对象
- BLOB（Binary Large OBject）是用于存储二进制数据的一种数据类型，它是一个可以存储大量数据的容器，能够容纳不同大小的数据。
```
# 在TBL_USER表插入类型为BLOB的列`U_IMG`
ALTER TABLE TBL_USER ADD U_IMG BLOB;
```
BLOB类型主要用于存储那些可能超出常规大小的数据，如图片、音频、视频等二进制数据。
如果存储的二进制对象过大可以使用 MEDIUMBLOB(可存储16Mb)

接下来进行关键代码mysql_write/mysql_read
### mysql_write: 可以看作陆地基站(NodeServer) 向 卫星(DBServer) 发送数据
statement相当于基站的存储空间，handle(传入的MYSQL对象)相当于基站的发射器
由于我们预定义的SQL语句有占位符`?`
```
#define SQL_INSERT_IMG_USER "INSERT TBL_USER (U_NAME, U_GENDER, U_IMG) VALUES ('KING', 'WOMAN', ?);"
```
我们需要通过MYSQL_STMT stmt来完成'?'的补全解析，将图片数据(LONG_BLOB类型)给它。
**完整代码如下:**
```
// NodeServer 基站 --> DBServer 卫星
int mysql_write(MYSQL *handle, char *buffer, int length) {
    if(handle == NULL || buffer == NULL || length <= 0) return -1;
    
    // statement相当于基站的存储空间，handle相当于基站的发射器

    // a.初始化stmt
    MYSQL_STMT *stmt = mysql_stmt_init(handle);

    // b.准备SQL语句(带占位符 ? )
    int ret = mysql_stmt_prepare(stmt, SQL_INSERT_IMG_USER, strlen(SQL_INSERT_IMG_USER));
    if(ret) {
        printf("mysql_stmt_prepare : %s\n", mysql_error(handle));
        return -2;
    }
    // c.绑定参数，告诉MySQL ? 的类型和数据
    MYSQL_BIND param = {0};
    param.buffer_type = MYSQL_TYPE_LONG_BLOB; //指定绑定参数的数据类型。
    param.buffer = NULL; //指定存储参数数据的内存缓冲区地址。
    param.is_null = 0; //该参数是有效的非 NULL 数据
    param.length = NULL;
    
    ret = mysql_stmt_bind_param(stmt, &param); //将参数绑定到预处理语句
    if(ret) {
        printf("mysql_stmt_bind_param : %s\n", mysql_error(handle));
        return -3;
    }
    // d.发生二进制数据
    ret = mysql_stmt_send_long_data(stmt, 0, buffer, length);
    if(ret) {
        printf("mysql_stmt_send_long_data : %s\n", mysql_error(handle));
        return -4;
    }
    // printf("## length = %d\n", length);
    // e.执行预处理语句
    ret = mysql_stmt_execute(stmt);
    if(ret) {
        printf("mysql_stmt_execute : %s\n", mysql_error(handle));
        return -5;
    }
    
    ret = mysql_stmt_close(stmt);
    if(ret) {
        printf("mysql_stmt_close : %s\n", mysql_error(handle));
        return -6;
    }

    return 0;
}

```
## mysql_read
这个可以看作卫星给陆地基站返回图片数据，我们要解析返回结果中的图片数据
statement相当于基站的存储空间，handle相当于基站的接收器
**完整代码如下：**
```
//DBServer 卫星 --> NodeServer 基站
int mysql_read(MYSQL *handle, char *buffer, int length) {
    if(handle == NULL || buffer == NULL || length <= 0) return -1;
    
    // statement相当于基站的存储空间，handle相当于基站的接收器
    // 1. 初始化stmt
    MYSQL_STMT *stmt = mysql_stmt_init(handle);
    int ret = mysql_stmt_prepare(stmt, SQL_SELECT_IMG_USER, strlen(SQL_SELECT_IMG_USER));
    if(ret) {
        printf("mysql_stmt_prepare : %s\n", mysql_error(handle));
        return -2;
    }
    // 2. 绑定结果，告诉mysql查询结果放哪里
    MYSQL_BIND result = {0};
    result.buffer_type = MYSQL_TYPE_LONG_BLOB;
    unsigned long total_length = 0; // 用于接收BLOB数据的总长度
    // 由于result.length是一个unsigned long *length的指针变量
    result.length = &total_length; //  // 绑定长度变量（MySQL会把实际长度写入这里），

    ret = mysql_stmt_bind_result(stmt, &result); // 将结果绑定到变量
    if(ret) {
        printf("mysql_stmt_bind_result : %s\n", mysql_error(handle));
        return -3;
    }
    // 3. 执行查询语句
    ret = mysql_stmt_execute(stmt); // 存储结果在管道里
    if(ret) {
        printf("mysql_stmt_execute : %s\n", mysql_error(handle));
        return -4;
    }
    // 4. 存储结果放到stmt里
    ret = mysql_stmt_store_result(stmt);

    if(ret) {
        printf("mysql_stmt_store_result : %s\n", mysql_error(handle));
        return -5;
    }
    printf("## total_length = %d\n", length);
    // printf("## result_buffer_length = %d\n", result.buffer_length);
    while(1) {
        ret= mysql_stmt_fetch(stmt);
        // 0: 成功，数据被提取到应用程序数据缓冲区。
        // 1: 出现错误。通过调用mysql_stmt_errno()和mysql_stmt_error()，可获取错误代码和错误消息。
        // MYSQL_NO_DATA: 不存在行／数据。
        // MYSQL_DATA_TRUNCATED: 出现数据截短。
        if(ret != 0 && ret != MYSQL_DATA_TRUNCATED) break;

        int start = 0;
        while(start < (int)total_length) {
            result.buffer = buffer + start; //
            result.buffer_length = 1;
            mysql_stmt_fetch_column(stmt, &result, 0, start);
            start += result.buffer_length;
        }
    }
    mysql_stmt_close(stmt);
    return total_length;
}
```

# 完整Code
可以通过`gcc -o Mysql Mysql.c -I /usr/include/mysql -lmysqlclient`编译代码。
```
/* 实现图片存储
1) 准备好图片，并且将图片read
2) mysql_write_image
3) mysql_read_image
4) 写入磁盘
*/
#include <stdio.h>
#include <string.h>
#include <mysql.h>

#define XIAOMO_DB_SERVER_IP "192.168.66.130"
#define XIAOMO_DB_SERVER_PORT 3306
#define XIAOMO_DB_USERNAME "admin"
#define XIAOMO_DB_PASSWD "root"
#define XIAOMO_DB_DEFAULTDB "XIAOMO_DB"

#define SQL_INSERT_TBL_USER "INSERT TBL_USER (U_NAME, U_GENDER) VALUES ('Lee', 'WOMAN');"
#define SQL_SELECT_TBL_USER "SELECT * FROM TBL_USER;"
#define SQL_DELETE_TBL_USER "CALL PROC_DELETE_USER('Lee');"

#define SQL_INSERT_IMG_USER "INSERT TBL_USER (U_NAME, U_GENDER, U_IMG) VALUES ('KING', 'WOMAN', ?);"
#define SQL_SELECT_IMG_USER "SELECT U_IMG FROM TBL_USER WHERE U_NAME='KING';"

#define FILE_IMAGE_LENGTH (888888)
// C U R D 增删改查

/* 
查询并读取数据
1）Node Server --<select SQL语句>-> DB server
2）DB server 通过管道返回给 Node Server查询数据
3）保存查询结果 MYSQL_RES *res = mysql_store_result(handle)
4）按照行打印结果 row = mysql_fetch_row(handle), row[i]是第i列
*/
int xiaomo_mysql_select(MYSQL *handle) {
    // mysql_real_query --> 执行SQL语句，结果存储在管道里 
    if(mysql_real_query(handle, SQL_SELECT_TBL_USER, strlen(SQL_SELECT_TBL_USER))) {
        printf("mysql_real_query : %s\n", mysql_error(handle));
        return -1;
    }
    
    // mysql_store_result--> 拿到存储结果集合 res
    MYSQL_RES *res = mysql_store_result(handle);
    if(res == NULL) {
        printf("mysql_store_result : %s\n", mysql_error(handle));
        return -2;
    }

    // rows / fileds --> res有多少行多少列
    int rows = mysql_num_rows(res);
    int fields = mysql_num_fields(res);
    printf("(rows,fields) = (%d,%d)\n", rows, fields);

    // fetch 
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res))) { //如果读取为空，while循环退出
        for (int i = 0; i < fields; i ++) {
            printf("%s\t", row[i]);
        }
        printf("\n");
    }
    // 释放
    mysql_free_result(res);

    return 0;
}

// 读取图片，并把结果存储到buffer中
int read_image(char *filename, char *buffer) {
    if(filename == NULL || buffer == NULL) return -1;

    FILE *fp = fopen(filename, "rb");
    if(fp == NULL) {
        printf("fopen failed\n");
        return -2;
    }

    // 通过fseek使得，文件指针指到文件末尾，以获得文件中有多少字节数据
    fseek(fp, 0, SEEK_END);
    // 通过fp偏移量获得filesize
    int length = ftell(fp);
    // 再次把fp置为开始
    fseek(fp, 0, SEEK_SET);

    // fread: 从文件指针fp开始，每次读一个字节，共读length次，结果放入buffer里
    int size = fread(buffer, 1, length, fp);
    if(size != length) {
        printf("fread failed: %d\n", size);
        return -3;
    }
    fclose(fp);

    return size;
}

int write_image(char *filename, char *buffer, int length) {
    if(filename == NULL || buffer == NULL || length <= 0) return -1;
    FILE *fp = fopen(filename, "wb+"); //wb : 只写模式，+ : 如果没有该文件 创建
    if(fp == NULL) {
        printf("fopen failed\n");
        return -2;
    }

    int size = fwrite(buffer, 1, length, fp);  // 写的数据来自buffer，写到fp里，每次写1个自己，共写length次
    if(size != length) {
        printf("fwrite failed: %d\n", size);
        return -3;
    }

    fclose(fp);
    return size;
}

// NodeServer 基站 --> DBServer 卫星
int mysql_write(MYSQL *handle, char *buffer, int length) {
    if(handle == NULL || buffer == NULL || length <= 0) return -1;
    
    // statement相当于基站的存储空间，handle相当于基站的发射器

    // a.初始化stmt
    MYSQL_STMT *stmt = mysql_stmt_init(handle);

    // b.准备SQL语句(带占位符 ? )
    int ret = mysql_stmt_prepare(stmt, SQL_INSERT_IMG_USER, strlen(SQL_INSERT_IMG_USER));
    if(ret) {
        printf("mysql_stmt_prepare : %s\n", mysql_error(handle));
        return -2;
    }
    // c.绑定参数，告诉MySQL ? 的类型和数据
    MYSQL_BIND param = {0};
    param.buffer_type = MYSQL_TYPE_LONG_BLOB; //指定绑定参数的数据类型。
    param.buffer = NULL; //指定存储参数数据的内存缓冲区地址。
    param.is_null = 0; //该参数是有效的非 NULL 数据
    param.length = NULL;
    
    ret = mysql_stmt_bind_param(stmt, &param); //将参数绑定到预处理语句
    if(ret) {
        printf("mysql_stmt_bind_param : %s\n", mysql_error(handle));
        return -3;
    }
    // d.发生二进制数据
    ret = mysql_stmt_send_long_data(stmt, 0, buffer, length);
    if(ret) {
        printf("mysql_stmt_send_long_data : %s\n", mysql_error(handle));
        return -4;
    }
    // printf("## length = %d\n", length);
    // e.执行预处理语句
    ret = mysql_stmt_execute(stmt);
    if(ret) {
        printf("mysql_stmt_execute : %s\n", mysql_error(handle));
        return -5;
    }
    
    ret = mysql_stmt_close(stmt);
    if(ret) {
        printf("mysql_stmt_close : %s\n", mysql_error(handle));
        return -6;
    }

    return 0;
}

//DBServer 卫星 --> NodeServer 基站
int mysql_read(MYSQL *handle, char *buffer, int length) {
    if(handle == NULL || buffer == NULL || length <= 0) return -1;
    
    // statement相当于基站的存储空间，handle相当于基站的接收器
    // 1. 初始化stmt
    MYSQL_STMT *stmt = mysql_stmt_init(handle);
    int ret = mysql_stmt_prepare(stmt, SQL_SELECT_IMG_USER, strlen(SQL_SELECT_IMG_USER));
    if(ret) {
        printf("mysql_stmt_prepare : %s\n", mysql_error(handle));
        return -2;
    }
    // 2. 绑定结果，告诉mysql查询结果放哪里
    MYSQL_BIND result = {0};
    result.buffer_type = MYSQL_TYPE_LONG_BLOB;
    unsigned long total_length = 0; // 用于接收BLOB数据的总长度
    // 由于result.length是一个unsigned long *length的指针变量
    result.length = &total_length; //  // 绑定长度变量（MySQL会把实际长度写入这里），

    ret = mysql_stmt_bind_result(stmt, &result); // 将结果绑定到变量
    if(ret) {
        printf("mysql_stmt_bind_result : %s\n", mysql_error(handle));
        return -3;
    }
    // 3. 执行查询语句
    ret = mysql_stmt_execute(stmt); // 存储结果在管道里
    if(ret) {
        printf("mysql_stmt_execute : %s\n", mysql_error(handle));
        return -4;
    }
    // 4. 存储结果放到stmt里
    ret = mysql_stmt_store_result(stmt);

    if(ret) {
        printf("mysql_stmt_store_result : %s\n", mysql_error(handle));
        return -5;
    }
    printf("## total_length = %d\n", length);
    // printf("## result_buffer_length = %d\n", result.buffer_length);
    while(1) {
        ret= mysql_stmt_fetch(stmt);
        // 0: 成功，数据被提取到应用程序数据缓冲区。
        // 1: 出现错误。通过调用mysql_stmt_errno()和mysql_stmt_error()，可获取错误代码和错误消息。
        // MYSQL_NO_DATA: 不存在行／数据。
        // MYSQL_DATA_TRUNCATED: 出现数据截短。
        if(ret != 0 && ret != MYSQL_DATA_TRUNCATED) break;

        int start = 0;
        while(start < (int)total_length) {
            result.buffer = buffer + start; //
            result.buffer_length = 1;
            mysql_stmt_fetch_column(stmt, &result, 0, start);
            start += result.buffer_length;
        }
    }
    mysql_stmt_close(stmt);
    return total_length;
}
int main() {

    MYSQL mysql;

    // 对mysql初始化
    if(NULL == mysql_init(&mysql)) {
        printf("mysql_init : %s\n", mysql_error(&mysql));
        return -1;
    }

    // mysql连接时，返回0，不成功
    if(!mysql_real_connect(&mysql, XIAOMO_DB_SERVER_IP, XIAOMO_DB_USERNAME, 
        XIAOMO_DB_PASSWD, XIAOMO_DB_DEFAULTDB, XIAOMO_DB_SERVER_PORT, NULL, 0)) {
        printf("mysql_real_connect : %s\n", mysql_error(&mysql));
        goto Exit;
    }

#if 0
    // mysql查询时，返回0，成功
    if(mysql_real_query(&mysql, SQL_INSERT_TBL_USER, strlen(SQL_INSERT_TBL_USER))) {
        printf("mysql_real_query : %s\n", mysql_error(&mysql));
        goto Exit;
    }
#endif

    // mysql --> insert
    xiaomo_mysql_select(&mysql);

#if 0
    printf("case : mysql --> delete \n");
    // DELETE 操作
    if(mysql_real_query(&mysql, SQL_DELETE_TBL_USER, strlen(SQL_DELETE_TBL_USER))) {
        printf("mysql_real_query : %s\n", mysql_error(&mysql));
        goto Exit;
    }
#endif

    xiaomo_mysql_select(&mysql);

    printf("case : mysql --> read image and write mysql\n");
    char buffer[FILE_IMAGE_LENGTH] = {0};
    int length = read_image("test.jpg", buffer);
    if(length < 0) goto Exit;
    
    mysql_write(&mysql, buffer, length); //插入数据库

    printf("case : mysql --> read mysql and write image\n");
    memset(buffer, 0, FILE_IMAGE_LENGTH);
    length = mysql_read(&mysql, buffer, FILE_IMAGE_LENGTH);

    write_image("d.jpg", buffer, length);

Exit:
    mysql_close(&mysql);
    
    return 0;
}
```