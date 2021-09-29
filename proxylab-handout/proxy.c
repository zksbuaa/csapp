#include <stdio.h>
#include "csapp.h"
#include "cache.h"


static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";
static const char *http_end = "\r\n";
static const char *host_hdr_format = "Host: %s\r\n";
static const char *requestline_hdr_format = "GET %s HTTP/1.0\r\n";
static const char *connection_key = "Connection";
static const char *user_agent_key= "User-Agent";
static const char *proxy_connection_key = "Proxy-Connection";
static const char *host_key = "Host";

void service(int);
void clienterror(int, char *, char *, char *, char *);
void parse_url(char *, char *, char *, int *);
void build_request(char *, char *, char *, int, rio_t *);
void handle_responses(rio_t *, int, char *);
void sigpipe_handler(int sig);
void *thread(void *vargp);  // thread routine
extern sem_t mutex, w;

int main(int argc, char **argv) {
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    Signal(SIGPIPE, sigpipe_handler);
    listenfd = Open_listenfd(argv[1]);
    cache_init();
    Sem_init(&mutex, 0 , 1);
    Sem_init(&w, 0 , 1);
    while (1) {
	    clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Pthread_create(&tid, NULL, thread, (void *)connfd);
    }
    return 0;
}

void service(int connfd)
{
    char buf[MAXLINE], method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char uri[MAXLINE], host[MAXLINE];
    char http_request[MAXLINE];
    int port;
    int clientfd;
    rio_t rio, clirio;

    Rio_readinitb(&rio, connfd);
    Rio_readlineb(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, url, version);
    if (strcasecmp(method, "GET")) {
        clienterror(connfd, method, "501", "Not implemented", "Tiny does not implement this method");
        return;
    }

    if (read_cache(url, connfd))
        return;

    // parse url to get uri, host and port
    parse_url(url, uri, host, &port);

    build_request(http_request, uri, host, port, &rio);
    char portname[MAXLINE];
    sprintf(portname, "%d", port);
    clientfd = Open_clientfd(host, portname);
    if (clientfd < 0)
        return;
    Rio_readinitb(&clirio, clientfd);
    Rio_writen(clientfd, http_request, strlen(http_request));
    handle_responses(&clirio, connfd, url);  // write cache
    Close(clientfd);
}


void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg)
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));
    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}

void parse_url(char *url, char *uri, char *host, int *port) {
    *port = 80;
    char *pos = strstr(url, "//");
    pos = (pos)?pos+2:url;
    char *pos2 = strstr(pos, ":");
    if (pos2) {
        *pos2 = 0;
        sscanf(pos, "%s", host);
        sscanf(pos2+1, "%d%s", port, uri);
        *pos2 = ':';
    } else {
        pos2 = strstr(pos, "/");
        if (pos2) {
            *pos2 = 0;
            sscanf(pos, "%s", host);
            *pos = '/';
            sscanf(pos2, "%s", uri);
        } else {
            sscanf(pos, "%s", host);
        }
    }
}

void build_request(char *http_request, char *uri, char *host, int port, rio_t *rp) {
    char buf[MAXLINE], request_line[MAXLINE], host_header[MAXLINE], other_header[MAXLINE];
    sprintf(request_line, requestline_hdr_format, uri);
    while (rio_readlineb(rp, buf, MAXLINE) > 0) {
        if (!strcmp(buf, http_end))
            break;
        if (!strncasecmp(buf, host_key, strlen(host_key))) {
            strcpy(host_header, buf);
            continue;
        }
        if(strncasecmp(buf,connection_key,strlen(connection_key))
                &&strncasecmp(buf,proxy_connection_key,strlen(proxy_connection_key))
                &&strncasecmp(buf,user_agent_key,strlen(user_agent_key)))
        {
            strcat(other_header,buf);
        }
    }
    if (strlen(host_header) == 0) {
        sprintf(host_header, host_hdr_format, host);
    }
    sprintf(http_request, "%s%s%s%s%s%s%s",
            request_line,
            host_header,
            connection_hdr,
            proxy_connection_hdr,
            user_agent_hdr,
            other_header,
            http_end);
    return;
}



void handle_responses(rio_t *rp, int connfd, char *url) {
    char buf[MAX_OBJECT_SIZE];
    int n = Rio_readnb(rp, buf, MAX_OBJECT_SIZE);
    if (n < MAX_OBJECT_SIZE) {
        Rio_writen(connfd, buf, n);
        write_cache(url, buf, n);
    } else {
        Rio_writen(connfd, buf, n);
        while ((n = Rio_readnb(rp, buf, MAX_OBJECT_SIZE)) > 0)
            Rio_writen(connfd, buf, n);
    }
}

void sigpipe_handler(int sig) {
    return;
}

void *thread(void *vargp) {
    Pthread_detach(pthread_self());
    int connfd = (int)vargp;
    service(connfd);
    Close(connfd);
}

