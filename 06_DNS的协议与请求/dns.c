#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h> 

#define DNS_SERVER_PORT      53
#define DNS_SERVER_IP        "114.114.114.114"
#define DNS_HOST		  	0x01
#define DNS_CNAME			0x05

struct dns_header{
    //16位，两个字节 --> short
    unsigned short id; //会话id
    unsigned short flags; //标志
    unsigned short questions; //问题数
    unsigned short answers; //回答资源记录数
    unsigned short authority; //授权资源记录数
    unsigned short additional; //附加资源记录数
};

struct dns_question{
    int length;
    unsigned short qtype;
    unsigned short qclass;
    unsigned char *name; //长度不固定, 存储域名
};

struct dns_item {
	char *domain;
	char *ip;
};

// client send to dns_server
int dns_creat_header(struct dns_header *header) {
    if(header == NULL) return -1;
    memset(header, 0, sizeof(struct dns_header));

    //通过random生成id
    srandom(time(NULL)); //随机值范围，1970年到现在的秒数
    header->id = random();

    header->flags = htons(0x0100); //转换为网络字节序
    header->questions = htons(1); //查询域名个数为1

    return 0;
}

// hostname: www.0voice.com ==> name: 3www60voice3com0
int dns_create_question(struct dns_question *question, const char *hostname) {

    if(question == NULL || hostname == NULL) return -1;
    memset(question, 0, sizeof(struct dns_question));

    question->name = (char*)malloc(strlen(hostname)+2); 
    if(question->name == NULL) {
        return -2;
    }

    question->length = strlen(hostname)+2;

    question->qtype = htons(1); //域名获得 IPv4 地址
    question->qclass = htons(1); //通常为1，表明是Internet数据

    // name
    const char delim[2] = ".";
    char *qname = question->name; //指向question->name的首地址

    char *hostname_dup = strdup(hostname); //复制一份，附带malloc
    char *token = strtok(hostname_dup, delim); 

    while(token != NULL) {
        size_t len = strlen(token);

        *qname = len; //定位到第len个地址
        qname ++;
        strncpy(qname, token, len+1); //strncpy区别于strcpy，其可以指定复制长度，len+1是复制到'\0'
        qname += len;

        token = strtok(NULL, delim);
    }
    free(hostname_dup);
}

//struct dns_header *header, struct dns_question *question, char *request
//建立的数据是，在header和question基础上，把request合并起来，然后一起发出去
int dns_build_request(struct dns_header *header, struct dns_question *question, char *request, int rlen) {
    if(header == NULL || question == NULL || request == NULL) return -1;

    memset(request, 0, rlen);
    // header --> request
    
    memcpy(request, header, sizeof(struct dns_header));
    int offset = sizeof(struct dns_header);

    // question --> request
    memcpy(request+offset, question->name, question->length);
    offset += question->length;

    memcpy(request+offset, &question->qtype, sizeof(question->qtype));
    offset += sizeof(question->qtype);

    memcpy(request+offset, &question->qclass, sizeof(question->qclass));
    offset += sizeof(question->qclass);

    return offset;

}

static int is_pointer(int in) {
	return ((in & 0xC0) == 0xC0);
}

static void dns_parse_name(unsigned char *chunk, unsigned char *ptr, char *out, int *len) {

	int flag = 0, n = 0, alen = 0;
	char *pos = out + (*len);

	while (1) {

		flag = (int)ptr[0];
		if (flag == 0) break;

		if (is_pointer(flag)) {
			
			n = (int)ptr[1];
			ptr = chunk + n;
			dns_parse_name(chunk, ptr, out, len);
			break;
			
		} else {

			ptr ++;
			memcpy(pos, ptr, flag);
			pos += flag;
			ptr += flag;

			*len += flag;
			if ((int)ptr[0] != 0) {
				memcpy(pos, ".", 1);
				pos += 1;
				(*len) += 1;
			}
		}
	
	}
	
}




static int dns_parse_response(char *buffer, struct dns_item **domains) {

	int i = 0;
	unsigned char *ptr = buffer;

	ptr += 4;
	int querys = ntohs(*(unsigned short*)ptr);

	ptr += 2;
	int answers = ntohs(*(unsigned short*)ptr);

	ptr += 6;
	for (i = 0;i < querys;i ++) {
		while (1) {
			int flag = (int)ptr[0];
			ptr += (flag + 1);

			if (flag == 0) break;
		}
		ptr += 4;
	}

	char cname[128], aname[128], ip[20], netip[4];
	int len, type, ttl, datalen;

	int cnt = 0;
	struct dns_item *list = (struct dns_item*)calloc(answers, sizeof(struct dns_item));
	if (list == NULL) {
		return -1;
	}

	for (i = 0;i < answers;i ++) {
		
		bzero(aname, sizeof(aname));
		len = 0;

		dns_parse_name(buffer, ptr, aname, &len);
		ptr += 2;

		type = htons(*(unsigned short*)ptr);
		ptr += 4;

		ttl = htons(*(unsigned short*)ptr);
		ptr += 4;

		datalen = ntohs(*(unsigned short*)ptr);
		ptr += 2;

		if (type == DNS_CNAME) {

			bzero(cname, sizeof(cname));
			len = 0;
			dns_parse_name(buffer, ptr, cname, &len);
			ptr += datalen;
			
		} else if (type == DNS_HOST) {

			bzero(ip, sizeof(ip));

			if (datalen == 4) {
				memcpy(netip, ptr, datalen);
				inet_ntop(AF_INET , netip , ip , sizeof(struct sockaddr));

				printf("%s has address %s\n" , aname, ip);
				printf("\tTime to live: %d minutes , %d seconds\n", ttl / 60, ttl % 60);

				list[cnt].domain = (char *)calloc(strlen(aname) + 1, 1);
				memcpy(list[cnt].domain, aname, strlen(aname));
				
				list[cnt].ip = (char *)calloc(strlen(ip) + 1, 1);
				memcpy(list[cnt].ip, ip, strlen(ip));
				
				cnt ++;
			}
			
			ptr += datalen;
		}
	}

	*domains = list;
	ptr += 2;

	return cnt;
	
}

// 按照DNS的协议发送给DNS服务器 UDP编程实现
// 服务器会按照DNS协议返回数据
int dns_client_commit(const char *domain) {

    int socfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(socfd < 0) return -1;

    struct sockaddr_in servaddr = {0};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(DNS_SERVER_PORT);
    servaddr.sin_addr.s_addr = inet_addr(DNS_SERVER_IP);

    // 正常情况下， UDP是无连接的，可以不用connect操作，直接sendto给服务器，但是不能保证成功发送给服务器，
    // 如果在UDP sendto之前添加connect，相当于给sendto探路，保证能够成功发送给服务器
    int ret = connect(socfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    printf("%d\n", ret);

    struct dns_header header = {0};
    dns_creat_header(&header);

    struct dns_question question = {0};
    dns_create_question(&question, domain);

    char request[1024] = {0};
    int len = dns_build_request(&header, &question, request, 1024);

    // request
    int slen = sendto(socfd, request, len, 0, (struct sockaddr*)&servaddr, sizeof(struct sockaddr));

    // recvfrom 得到服务器返回的数据
    char response[1024] = {0};
    struct sockaddr_in addr;
    size_t addr_len = sizeof(struct sockaddr_in);
    int n = recvfrom(socfd, response, sizeof(response), 0, (struct sockaddr*)&addr, (socklen_t*)&addr_len);
    //printf("recvfrom: %d %s\n", n, response);

    //解析response
    struct dns_item *dns_domain = NULL;
	dns_parse_response(response, &dns_domain);

	free(dns_domain);
    return n;
}

int main(int argc, char *argv[]) {
    if(argc < 2) return -1;
    dns_client_commit(argv[1]); 
}