#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/types.h>
#include <sys/stat.h>

#pragma comment(lib,"Ws2_32.lib")

#define PRINTF(str) printf("[%s - %d]"#str"=%s\n" ,__func__, __LINE__,str);

void error_die(const char* str){
    perror(str);
    exit(1);
}

int startup(unsigned short *port){
    // 1.网络通信初始化
    WSADATA data;
    int ret = WSAStartup(MAKEWORD(2,2),&data);
    if (ret != 0){
        error_die("WSAstartup失败！");
    }
    // 2.创建套接字
    int server_socket = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(server_socket == INVALID_SOCKET){
        error_die("套接字创建失败!");
    }

    // 3.设置端口属性可复用
    int opt = 1;
    setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt,sizeof(opt));
    if (opt == -1){
        error_die("端口可复用设置失败");
    }

    //配置服务器端网络地址
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(*port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);


    // 4.绑定套接字
    if(bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        error_die("绑定失败!");
    }


    if(*port == 0){
        // 动态分配一个端口
        int nameLen = sizeof(server_addr);
        if(getsockname(server_socket,(struct sockaddr*)&server_addr,&nameLen)<0){
            error_die("动态分配端口错误!");
        }
        *port = ntohs(server_addr.sin_port); // 转换为主机字节顺序
    }

    // 5.创建监听队列
    if(listen(server_socket, 5) <0){
        error_die("监听队列失败");
    }

    return server_socket;

}

//从指定的客户端套接字，读取一行数据，保存到buff中
//返回实际读取到的字节数
int get_line(int sock,char* buff, int size){

    char c = 0;
    int i = 0;
    while(i<size-1 && c!='\n'){
        int n = recv(sock,&c,1,0);
        if (n>0){
            if (c == '\r'){
                n = recv(sock,&c,1,MSG_PEEK);
                if(n > 0 && c == '\n'){
                    recv(sock,&c,1,0);
                }
                else{
                    c = '\n';
                }
            }
            buff[i++] = c;
        }
        else{
            //to do
            c = '\n';
        }
    }
    buff[i] = 0;
    return 1;
}

void cat(int client, FILE *resource){
    char buff[4096];
    int count = 0;
    while(1){
        int ret = fread(buff,sizeof(char),sizeof(buff),resource);
        if (ret <= 0){
            break;
        }
        send(client,buff,ret,0);
        count += ret;
    }
    printf("一共发送[%d]字节给浏览器\n",count);
}

void headers(int client,const char* type){
    char buff[1024];

    strcpy(buff, "HTTP/1.0 200 OK\r\n");
    send(client,buff,strlen(buff),0);

    strcpy(buff, "Server:MyHttpd/0.1\r\n");
    send(client,buff,strlen(buff),0);

    strcpy(buff, "Content-type:text/html\n");
    send(client,buff,strlen(buff),0);

    strcpy(buff,"\r\n");
    send(client,buff,strlen(buff),0);
}

void unimplement(int client){
    // 向指定的套接字，发送一个提示还没有实现的错误页面
    char buff[1024];

    sprintf(buff,
        "HTTP/1.1 501 Not Implemented\r\n"
        "Server: MyHttpd/0.1\r\n"
        "Content-type: text/html\r\n"
        "\r\n"
        "<HTML><BODY><H1>501 Not Implemented</H1></BODY></HTML>\r\n"
    );

    send(client, buff, strlen(buff), 0);
}


void not_found(int client){
    //发送404
    char buff[1024];

    FILE *resource = NULL;

    const char* fileName = "htdocs/404.html";

    strcpy(buff, "HTTP/2.2 404 NOT FOUND\r\n");
    send(client,buff,strlen(buff),0);

    strcpy(buff, "Server:MyHttpd/0.1\r\n");
    send(client,buff,strlen(buff),0);

    strcpy(buff, "Content-type:text/html\n");
    send(client,buff,strlen(buff),0);

    strcpy(buff,"\r\n");
    send(client,buff,strlen(buff),0);

    if(strcmp(fileName,"htdocs/404.html")==0){
    resource = fopen(fileName,"r");
    }
    else{
    resource = fopen(fileName,"rb");
    }

   // 发送404网页内容
    if (resource != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), resource) != NULL) {
            send(client, line, strlen(line), 0);
        }
        fclose(resource);
    }
}

const char* getHeadType(const char* fileName){
    const char* ret = "text/html";
    const char* p = strrchr(fileName,'.');
    if (!p) return ret;

    p++;
    if (strcmp(p, "css") == 0) ret = "text/css";
    else if (strcmp(p, "jpg") == 0) ret = "image/jpeg";
    else if (strcmp(p, "jpeg") == 0) ret = "image/jpeg";
    else if (strcmp(p, "png") == 0) ret = "image/png";
    else if (strcmp(p,"svg")==0) ret = "image/svg+xml";
    else if (strcmp(p, "html") == 0) ret = "text/html";
    else if (strcmp(p, "js") == 0) ret = "application/javascript";
    else if (strcmp(p, "json") == 0) ret = "application/json";
    else if (strcmp(p, "xml") == 0) ret = "application/xml";
    else if (strcmp(p, "gif") == 0) ret = "image/gif";
    else if (strcmp(p, "svg") == 0) ret = "image/svg+xml";
    else if (strcmp(p, "txt") == 0) ret = "text/plain";
    else if (strcmp(p, "scss") == 0) ret = "text/x-scss";

    return ret;
}

void server_file(int client, const char* fileName){
    int numchars = 1; //注意要把流程走完
    char buff[1024];
    
    //把请求数据包的剩余数据行，读完
    while (numchars > 0 && strcmp(buff, "\n")){
    numchars = get_line(client, buff, sizeof(buff));
    PRINTF(buff);
    }

    FILE *resource = NULL;

    if(strcmp(fileName,"htdocs/index.html")==0){
    resource = fopen(fileName,"r");
    }
    else{
    resource = fopen(fileName,"rb");
    }

    if (resource == NULL){
        not_found(client);
    }
    else{
        headers(client, getHeadType(fileName));
        //发送请求的资源信息
        cat(client,resource);

        printf("资源发送完毕!\n");
    }

    fclose(resource);
}


//处理用户请求的线程函数
DWORD WINAPI accept_request(LPVOID arg){
    char buff[1024];

    int client = (SOCKET)arg;
    // 读取一行数据
    // 0x015ffad8 "GET / HTTP/1.1\n"
    int numchars = get_line(client,buff,sizeof(buff));
    PRINTF(buff); // [accept_request-53]buff="EGT ...."

    char method[255];

    int j = 0,i = 0;
    while(!isspace(buff[j]) && i < sizeof(method)-1){
        method[i++] = buff[j++];
    }
    method[i] = 0;  //'\0'
    PRINTF(method);

    // 检查请求的方法，本服务器是否支持
    if(stricmp(method,"GET") && stricmp(method,"POST")){
        unimplement(client);
        return 0;
    }

    // 解析资源文件的路径
    char url[255]; // 存放请求的资源的完整路径
    i = 0;
    //"GET /test/abc.html HTTP/1.1\n" 扫描的是空格GET后面
    while(isspace(buff[j]) && j < sizeof(buff)){
        j++;
    }

    //读到不是白字符,即HTTP前面
    while(!isspace(buff[j]) && i < sizeof(url)-1 && j < sizeof(buff)){
        url[i++] = buff[j++];
    }
    url[i] = 0;
    PRINTF(url);

    // www.baidu.com/networks
    // 127.0.0.1/networks
    // url /networks
    // htdocs/networks

    char path[512] = "";

    sprintf(path,"htdocs%s",url);

    if (path[strlen(path)-1] == '/'){
        strcat(path,"index.html");  //拼接
    }

    PRINTF(path);

    struct stat status;
    if (stat(path, &status) == -1){ //不存在的情况
        // 请求包的剩余数据读取完毕
        while (numchars > 0 && strcmp(buff, "\n")){
        numchars = get_line(client, buff, sizeof(buff));
        }

        not_found(client);

    }

    else {
        //检查是否是文件还是目录，否则拼接一个/index.html
        if ((status.st_mode & S_IFMT) == S_IFDIR){
            strcat(path, "/index.html");
        }
        // 然后发回服务端文件给服务器

        server_file(client,path);
    }

    closesocket(client);


    return 0;
}

int main(void){
    unsigned short port = 8080; //如果端口是0，则触发动态分配端口。
    int server_sock = startup(&port);
    printf("正在监听:%d",port);


    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    //to do
    while(1){
        // 阻塞
        int client_sock = accept(server_sock,(struct sockaddr*)&client_addr,&client_addr_len);
        if (client_sock == -1){
            error_die("接收失败!");
        }

        // 创建新的线程
        DWORD threadId = 0; //DWORD = unsigned __LONG32;
        CreateThread(0,0,accept_request,(void*)client_sock,0,&threadId);

    }

    closesocket(server_sock);

    return 0;
}
