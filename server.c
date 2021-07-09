/**
 * Simple File Transfer Server For 2016 Introduction to Computer Network.
 * Author: vicky-sunshine @ HSNL
 * 2016/10
 * **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>  /* contains a number of basic derived types */
#include <sys/socket.h> /* provide functions and data structures of socket */
#include <arpa/inet.h>  /* convert internet address and dotted-decimal notation */
#include <netinet/in.h> /* provide constants and structures needed for internet domain addresses*/
#include <unistd.h>     /* `read()` and `write()` functions */
#include <dirent.h>     /* format of directory entries */

#define MAX_SIZE 2048
#define MAX_CONNECTION 5

void connection_handler(int sockfd);
void hello_msg_handler(int sockfd);
void file_listing_handler(int sockfd);
void file_sending_handler(int sockfd, char filename[]);
int port;




int main(int argc, char **argv) {
  int svr_fd;                   // socket file descriptor, return by `socket()`
  struct sockaddr_in svr_addr;  // address of server, used by `bind()`

  int cli_fd;                   // descriptor of incomming client, return by `accept()`
  struct sockaddr_in cli_addr;  // address of client, used by `accept()`
  socklen_t addr_len;           // size of address, used by `accept()`


  /* create the socket, use `socket()` */
  svr_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (svr_fd < 0) {
    perror("Create socket failed.");
    exit(1);
  }

  /**TODO 1:
    preparing sockaddr_in **/
  port = atoi(argv[1]);
  bzero(&svr_addr, sizeof(svr_addr));
  svr_addr.sin_family = AF_INET;
  svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  svr_addr.sin_port = htons(port);
  /****/

  /**TODO 2:
    bind the socket to port, with prepared sockaddr_in structure**/
  if (bind(svr_fd, (struct sockaddr*)&svr_addr , sizeof(svr_addr)) < 0) {
    perror("Bind socket failed.");
    exit(1);
  }

  /****/

  /**TODO 3:
    listen on socket**/
  if (listen(svr_fd, MAX_CONNECTION) < 0) {
    perror("Listen socket failed.");
    exit(1);
  }

  /****/

  printf("File transfer server started\n");
  printf("Maximum connections set to %d\n", MAX_CONNECTION);
  printf("Listening on %s:%d\n", inet_ntoa(svr_addr.sin_addr), atoi(argv[1]));
  printf("Waiting for client...\n\n");


  addr_len = sizeof(cli_addr);
  while(1) {
    /**TODO 4:
      accept client connections**/
    cli_fd = accept(svr_fd, (struct sockaddr*)&cli_addr, (socklen_t*)&addr_len);
	if (cli_fd < 0){
	  	perror("Accept failed");
      	exit(1);
    }
/****/

    printf("[INFO] Connection accepted (id: %d)\n", cli_fd);
    printf("[INFO] Client is from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

    connection_handler(cli_fd);//呼叫connection_handler,傳入cli_fd 

    close(cli_fd);
  }

  close(svr_fd);
  return 0;
}

void connection_handler(int sockfd) {
  char filename[MAX_SIZE];
  memset(filename, '\0', MAX_SIZE);

  /* send hello msg to client */
  hello_msg_handler(sockfd);//傳入的是client 

  /* listing file info to client */
  file_listing_handler(sockfd);//傳入的是client 

  /* read request filename from client*/
  while ((read(sockfd, filename, MAX_SIZE)) > 0) {
    /* client want to exit*/
    if (strcmp(filename, ".exit") == 0) {
      break;
    }
    printf("[INFO] Client send `%s` request\n", filename);//當讀到client的要求 

    /* sending this file */
    file_sending_handler(sockfd, filename);//開始傳送client要求的檔案 
    memset(filename, '\0', MAX_SIZE);
  }

  printf("[INFO] Connection closed (id: %d)\n", sockfd);
  close(sockfd);
}

void hello_msg_handler(int sockfd) {
  char buf[MAX_SIZE];

  printf("[INFO] Send hello msg to client\n");

  /* send hello msg to client */
  sprintf(buf, "%s", "[���] Connect to server.\n[���] Server reply!\n-----------\nFiles on server:\n");
  if (write(sockfd, buf, MAX_SIZE) < 0) {
      perror("Write failed!\n");
  }
}

void file_listing_handler(int sockfd) {
  DIR* pDir;                      // 目錄 
  struct dirent* pDirent = NULL;  // 初始目錄和子檔案 in this dir
  char buf[MAX_SIZE];             // buffer to store msg

  printf("[INFO] List file to client\n");

  /* 打開remote storage directory */
  if ((pDir = opendir("./remote_storage")) == NULL) {
      perror("Open directory failed\n");
  }

  /* traversing files in remote storage and sending filenames to client*/
  memset(buf, '\0', MAX_SIZE);//空字元填入buf 
  while ((pDirent = readdir(pDir)) != NULL) {//read目錄 //readdir回傳的pDirent=(dirent的指標 ) 
      /* ignore current directory and parent directory */
      if (strcmp(pDirent->d_name, ".") == 0 || strcmp(pDirent->d_name, "..") == 0) {
        continue;//跳過現在目錄跟上層目錄，找子資料夾
      }

      /**TODO 5:
        send filenames to client
        //server client 達成協議 知道write/read幾次 **/
      //當有檔案符合條件就寄給client，所以client需要設迴圈來多次接收  
      strcpy(buf,pDirent->d_name);
      send(sockfd, buf, sizeof(buf), 0);
      }
      char finish[MAX_SIZE]={"E"};
      send(sockfd,finish,sizeof(finish),0);

  closedir(pDir);
  /****/
}

void file_sending_handler(int sockfd, char filename[]) {
  char fail_msg[17] = "Download failed\n";
  char buf[MAX_SIZE];   // buffer to store msg
  char path[MAX_SIZE];  // path to this file
  FILE *fp;             // file want to send //fp是要send的file 

  int file_size = 0;    //  size of this file
  int write_byte = 0;   //  bytes this time sent
  int write_sum = 0;    //  bytes have been sent

  sprintf(path, "remote_storage/%s", filename);
  fp = fopen(path, "rb");//以可讀寫方式開啟二進位元檔案 
  if (fp) {//成功開啟fb 
    /* send start downloading message */
    memset(buf, '\0', MAX_SIZE);
    sprintf(buf, "[-] Downloading `%s` ...\n", filename);
    if (write(sockfd, buf, MAX_SIZE) < 0) {//write把buf寫到sockfd裡 
        printf("Send downloading message failed");
        return;
    }

    /* get file size, store in file_size */
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    memset(buf, '\0', MAX_SIZE);
    sprintf(buf, "%d", file_size);//sprintf把數字填到字串中，現在buf=檔案大小 

    /**TODO 6:
      send file size to client**/
    send(sockfd, buf, sizeof(buf), 0);

    /****/

    /* read file data and send to client */
    write_sum = 0;
    while (write_sum < file_size) {

      /* read local file to buf */
      memset(buf, '\0', MAX_SIZE);
      write_byte = fread(&buf, sizeof(char), MAX_SIZE, fp);//把fb存入buf的位址，就可以傳送buf給client 

      /**TODO 7:
        send file data to client**/
		send(sockfd, buf, write_byte, 0);//這裡是一次傳一個嗎? 
      /****/
      write_sum += write_byte;
    }

    fclose(fp);

    /* sleep for a while */
    sleep(2);

    /* send download successful message */
    memset(buf, '\0', MAX_SIZE);
    sprintf(buf, "%s", "[���] Download successfully!\n");
    write(sockfd, buf, strlen(buf));

  } else {
    /* fp is null*/
    printf("[ERROR] %s\n", fail_msg);
    write(sockfd, fail_msg, MAX_SIZE);
    return;
  }
}
