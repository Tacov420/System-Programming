#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
    int id;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

typedef struct {
    int id;          //902001-902020
    int AZ;          
    int BNT;         
    int Moderna;     
}registerRecord;

int handle_read(request* reqP) {
    int r;
    char buf[512];

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
    char* p1 = strstr(buf, "\015\012");
    int newline_len = 2;
    if (p1 == NULL) {
       p1 = strstr(buf, "\012");
        if (p1 == NULL) {
            ERR_EXIT("this really should not happen...");
        }
    }
    size_t len = p1 - buf + 1;
    memmove(reqP->buf, buf, len);
    reqP->buf[len - 1] = '\0';
    reqP->buf_len = len-1;
    return 1;
}

void gen_lock(struct flock *lptr, short type, off_t offset, short whence, off_t len){
	lptr->l_type = type;
	lptr->l_start = offset;
	lptr->l_whence = whence;
	lptr->l_len = len;
	return;
}

void print_preference(registerRecord *r, int conn_fd, int type){
    char s[256];
    if(!type) sprintf(s, "Your preference order is");
    else sprintf(s, "Preference order for %d modified successed, new preference order is", r->id);
    write(conn_fd, s, strlen(s));
    for(int i = 1;i <= 3;i++){
        if(r->AZ == i) sprintf(s, " AZ");
        else if(r->BNT == i) sprintf(s, " BNT");
        else sprintf(s, " Moderna");
        write(conn_fd, s, strlen(s));

        if(i < 3) sprintf(s, " >");
        else sprintf(s, ".\n");
        write(conn_fd, s, strlen(s));
    }
    return;
}

void read_rrcd(registerRecord *rrcd_ptr, int file_fd, int id){
    lseek(file_fd, (id - 902001) * sizeof(registerRecord), SEEK_SET);
    read(file_fd, rrcd_ptr, sizeof(registerRecord));
    lseek(file_fd, 0, SEEK_SET);    //recover the reading pointer
    return;
}

void write_rrcd(registerRecord *rrcd_ptr, int file_fd, int id){
    lseek(file_fd, (id - 902001) * sizeof(registerRecord), SEEK_SET);
    write(file_fd, rrcd_ptr, sizeof(registerRecord));
    lseek(file_fd, 0, SEEK_SET);    //recover the reading pointer
    return;
}

int check_order(request *requestP, int conn_fd, int file_fd, bool local_lock[]){
    char s[128];
    int id;
    struct flock lock;
    if(strlen(requestP[conn_fd].buf) == 6 && ((id = atoi(requestP[conn_fd].buf)) >= 902001) && id <= 902020){
    	gen_lock(&lock, F_RDLCK, (id - 902001) * sizeof(registerRecord), SEEK_SET, sizeof(registerRecord));
        fcntl(file_fd, F_GETLK, &lock);
        if(lock.l_type != F_UNLCK || local_lock[id - 902001]){
            sprintf(s, "Locked.\n");
            write(conn_fd, s, strlen(s));
            return -1;
        }
        else{
            gen_lock(&lock, F_RDLCK, (id - 902001) * sizeof(registerRecord), SEEK_SET, sizeof(registerRecord));
            fcntl(file_fd, F_SETLK, &lock);
            local_lock[id - 902001] = true;
            registerRecord rrcd;
            read_rrcd(&rrcd, file_fd, id);
            print_preference(&rrcd, conn_fd, 0);
            gen_lock(&lock, F_UNLCK, (id - 902001) * sizeof(registerRecord), SEEK_SET, sizeof(registerRecord));
            fcntl(file_fd, F_SETLK, &lock);
            local_lock[id - 902001] = false;
            return id;
        }
	}
    else{
        sprintf(s, "[Error] Operation failed. Please try again.\n");
        write(conn_fd, s, strlen(s));
        return -1;
	}
}

int c2i(char c){
    return c - '0';
}

bool valid_order(char s[]){
    bool used[3] = {0};
    if(strlen(s) == 5){
        for(int i = 0;i < 5;i++){
            if(i % 2){
                if(s[i] != ' ') return false;
            }
            else{
                int x = c2i(s[i]);
                if(x < 1 || x > 3 || used[x - 1]) return false;
                else used[x - 1] = true;
            }
        }
        return true;
    }
    else return false;
}

void change_order(request *requestP, int conn_fd, int file_fd, int id){
    if(valid_order(requestP[conn_fd].buf)){
        registerRecord rrcd;
        read_rrcd(&rrcd, file_fd, id);
        rrcd.AZ = c2i(requestP[conn_fd].buf[0]);
        rrcd.BNT = c2i(requestP[conn_fd].buf[2]);
        rrcd.Moderna = c2i(requestP[conn_fd].buf[4]);
        print_preference(&rrcd, conn_fd, 1);
        write_rrcd(&rrcd, file_fd, rrcd.id);
    }
    else{
        char s[256];
        sprintf(s, "[Error] Operation failed. Please try again.\n");
        write(conn_fd, s, strlen(s));
    }
    return;
}

int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd = open("registerRecord", O_RDWR);  // fd for file that we open for reading
    char buf[512];
    int buf_len;

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    bool local_lock[20] = {0};
    fd_set master_set, working_set;
    FD_ZERO(&master_set);
    FD_SET(svr.listen_fd, &master_set);
    int end = svr.listen_fd;
    while (1) {
        // TODO: Add IO multiplexing
        memcpy(&working_set, &master_set, sizeof(master_set));
        select(end + 1, &working_set, NULL, NULL, NULL);
        
        // Check new connection
        if(FD_ISSET(svr.listen_fd, &working_set)){
            clilen = sizeof(cliaddr);
            conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
            if (conn_fd < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;  // try again
                if (errno == ENFILE) {
                    (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                    continue;
                }
                ERR_EXIT("accept");
            }
            requestP[conn_fd].conn_fd = conn_fd;
            strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
            fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
            
            end++;   //update the range of conn_fds
            FD_SET(conn_fd, &master_set);   //set conn_fd in readfds
            requestP[conn_fd].wait_for_write = 0;   //wait for read

            sprintf(buf, "Please enter your id (to check your preference order):\n");
            write(conn_fd, buf, strlen(buf));
        }
        
        for(int i = svr.listen_fd + 1;i <= end;i++){
            if(FD_ISSET(i, &working_set)){
                int ret = handle_read(&requestP[i]); // parse data from client to requestP[i].buf
                fprintf(stderr, "ret = %d\n", ret);
                if (ret < 0) {
                    fprintf(stderr, "bad request from %s\n", requestP[i].host);
                    continue;
                }

                // TODO: handle requests from clients
                #ifdef READ_SERVER     
                fprintf(stderr, "%s", requestP[i].buf);
                sprintf(buf,"%s : %s",accept_read_header,requestP[i].buf);

                int id = check_order(requestP, i, file_fd, local_lock);
                #elif defined WRITE_SERVER
                fprintf(stderr, "%s", requestP[i].buf);
                sprintf(buf,"%s : %s",accept_write_header,requestP[i].buf);
                
                struct flock lock;
                if(!requestP[i].wait_for_write){
                    int id;
                    if((id = check_order(requestP, i, file_fd, local_lock)) > 0){
                        //write lock
                        gen_lock(&lock, F_WRLCK, (id - 902001) * sizeof(registerRecord), SEEK_SET, sizeof(registerRecord));
                        fcntl(file_fd, F_SETLK, &lock);
                        local_lock[id - 902001] = true;
                        requestP[i].wait_for_write = 1; //wait for write
                        requestP[i].id = id;

                        sprintf(buf, "Please input your preference order respectively(AZ,BNT,Moderna):\n");
                        write(i, buf, strlen(buf));
                    }
                }
                else{
                    change_order(requestP, i, file_fd, requestP[i].id);

                    //unlock
                    gen_lock(&lock, F_UNLCK, (requestP[i].id - 902001) * sizeof(registerRecord), SEEK_SET, sizeof(registerRecord));
                    fcntl(file_fd, F_SETLK, &lock);
                    local_lock[requestP[i].id - 902001] = false;

                    requestP[i].wait_for_write = 0; //finish writing
                    FD_CLR(i, &master_set);  //we don't need to check it anymore
                }
                #endif

                // read or finished writing
                if(!requestP[i].wait_for_write){
                    FD_CLR(i, &master_set);
                    close(requestP[i].conn_fd);
                    free_request(&requestP[i]);
                }
            }
        }
    }
    free(requestP);
    return 0;
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}
