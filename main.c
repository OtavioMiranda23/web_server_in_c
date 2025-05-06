#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <regex.h>

#define PORT 8080
#define BUFFER_LEN 1024
int main() {
    //sockets, file descriptor
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_LEN] = { 0 };
    const char* html_body = "<html><head><title>Web Server C</title></head></html>"
    "<body><header><h1>Bem-vindo ao meu site!</h1></header>";
    char html_response[BUFFER_LEN];
    sprintf(html_response,
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: %d\r\n"
    "\r\n"
    "%s", (int)strlen(html_body), html_body);
    //IPV4, TCP/IP, sem config add;
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Error trying to create a socket!\n");
        exit(EXIT_FAILURE);    
    } 

    //define opções p/ o socket
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    //configuraçao com o mundo exterior
    address.sin_family = AF_INET; //IPV4
    address.sin_addr.s_addr = INADDR_ANY; //qualquer lugar pode se conectar conosco
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3); //Faz uma fila de 3 antes de retornar que esta com a fila cheia

    printf("Web serve listening on port 8080\n");
    while (1) {
        //aceita a conexao e cria um novo socket para conversar com o client
        client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        int byteCount = read(client_fd, buffer, sizeof(buffer));
        if (byteCount > 0) {
            // printf("Received message:\n %s\n", buffer);
        }
        write(client_fd, html_response, strlen(html_response));
        regex_t regex;
        regcomp(&regex, "GET /([^ ]*) HTTP/1", REG_EXTENDED);
        regmatch_t matches[2];
        if ((regexec(&regex, buffer, 2, matches, 0)) == 0) {
            int start = matches[1].rm_so;
            int end = matches[1].rm_eo;
            buffer[end] = '\0';
            const char *url_encoded_file_name = buffer + start;
            char *file_name = url_decode(url_encoded_file_name);
            printf("--%s\n", file_name);
            // printf("%.*s\n", end - start, buffer + start);



        }
        close(client_fd);
    }
    
    close(server_fd);
    return 0;
}