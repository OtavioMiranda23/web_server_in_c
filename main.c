#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <regex.h>
#include <strings.h>
#include <asm-generic/socket.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_LEN 1024


char *get_mime_type(const char *file_ext) {
    if (strcasecmp(file_ext, "html") == 0 || strcasecmp(file_ext, "htm") == 0) {
        return "text/html"; 
    } else if (strcasecmp(file_ext, "txt") == 0) {
        return "text/plain";
    } else if (strcasecmp(file_ext, "jpg") == 0) {
        return "image/jpeg";
    } else if (strcasecmp(file_ext, "png") == 0) {
        return "image/png";
    } else {
        return "application/octet-stream";
    }
}
char *url_decode(const char *src) {
    size_t src_len = strlen(src);
    char *decoded = malloc(src_len + 1);
    size_t decoded_len = 0;
    for (size_t i = 0; i < src_len; i++) {
        if (src[i] == '%' && i +2  < src_len) {
            int hex_val;
            sscanf(src + i + 1, "%2x", &hex_val);
            decoded[decoded_len++] = hex_val;
            i +=2;
        } else {
            decoded[decoded_len++] = src[i];
        }
        decoded[decoded_len] = '\0';
    }
    return decoded;    
}

void build_http_response(const char *file_name,
     const char *file_ext,
     char *response,
     size_t *response_len) {
    //comparar se a extensão é igual a html/htm/txt/jpg/png
    char *mime_type = get_mime_type(file_ext);
    char *header = (char *) malloc(BUFFER_LEN * sizeof(char));
    snprintf(header, BUFFER_LEN,
        "HTTP/1.1 200 OK\r\n"
        "Content-type: %s\r\n"
        "\r\n",
        mime_type);
    //se o recurso não existir, resposta é 404 Not Found
    if (strcmp(file_name, ".well-known/appspecific/com.chrome.devtools.json") == 0) {
        snprintf(response,BUFFER_LEN,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-type: text/plain\r\n"
            "Connection: close\r\n"
            "\r\n"
            "404 Not Found\n"
        );
    }
    int file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1) {
        snprintf(response, BUFFER_LEN,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-type: text/plain\r\n"
            "\r\n"
            "404 Not Found\n");
            // *response_len = strlen(response);
            return;
        }
        //pega o file size para Content-Length
        struct stat file_stat;
        fstat(file_fd, &file_stat);
        off_t file_size = file_stat.st_size;

        //copy header to response buffer
        *response_len = 0;
        memcpy(response, header, strlen(header));
        *response_len += strlen(header);
        size_t bytes_read;
        while ((bytes_read = read(file_fd, response + *response_len, BUFFER_LEN - *response_len)) > 0) {
            *response_len += bytes_read;
        }
        free(header);
        close(file_fd);
        
    }

char *get_file_extension(const char *file_name) {
    char *dot = strrchr(file_name, '.');
    if (!dot || dot == file_name) {
        return "";
    }
    return dot+1;
}
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
        // write(client_fd, html_response, strlen(html_response));
        regex_t regex;
        regcomp(&regex, "GET /([^ ]*) HTTP/1", REG_EXTENDED);
        regmatch_t matches[2];
        if ((regexec(&regex, buffer, 2, matches, 0)) == 0) {
            int start = matches[1].rm_so;
            int end = matches[1].rm_eo;
            buffer[end] = '\0';
            const char *url_encoded_file_name = buffer + start;
            char *file_name = url_decode(url_encoded_file_name);
            char file_ext[32];
            strcpy(file_ext, get_file_extension(file_name));
            char *response = (char *)malloc(BUFFER_LEN * 2 * sizeof(char));
            size_t response_len;
            build_http_response(file_name, file_ext, response, &response_len);
            //send http response to client
            send(client_fd, response, response_len, 0);
            free(response);
            free(file_name);
        }
        regfree(&regex);
    }    
    close(server_fd);
    return 0;
}