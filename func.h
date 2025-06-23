#include "struct.h"

unsigned char* Read_Name(unsigned char* reader,unsigned char* dns_message,int *count)
{
    unsigned char* name;
    unsigned int p = 0,jumped = 0,offset;
    int i,j;

    int bytes_read = 0;
    name=(unsigned char*)malloc(256);
    if (name == NULL)
    {
        perror("malloc for DNS name failed");
        return NULL;
    }

    name[0]='\0';

    while (*reader!=0)
    {
        if(*reader>=192) //  192 = 1100 0000(first two bytes) 
        {
            offset=((*reader & 63) << 8) | *(reader + 1); // 63 = 0011 1111
            bytes_read += 2;
            reader = dns_message + offset;
            break;
        }
        else
        {
            int label_len=*reader;
            strncat((char*)name,reader+1,label_len);
            strcat((char*)name, ".");
            reader = reader + label_len + 1;
            bytes_read += (label_len+1);
        }
    }

    if(*reader==0)
    {
        bytes_read += 1;
    }
        *count = bytes_read;

    if (strlen((const char*)name) > 0 && name[strlen((const char*)name) - 1] == '.') 
    {
        name[strlen((const char*)name) - 1] = '\0';
    }
    return name;
}

config* import_config()
{
    config* res=(config*)malloc(sizeof(config));
    if(res == NULL)
    {
        fputs("Error allocating memory for config structure\n",stderr);
        exit(1);
    }

    res->black_list = NULL;
    res->bl_tokens = 0;
    res->upstream_dns_ip = NULL;
    res->response = NULL;

    FILE *config_ptr;
    config_ptr=fopen("./config.txt","r");
    if(config_ptr == NULL)
    {
        fputs("Error opening config file\n",stderr);
        exit(1);
    }

    fseek(config_ptr,0,SEEK_END);
    long lSize = ftell(config_ptr);
    rewind(config_ptr);

    char * config_file_buffer = (char*)malloc(sizeof(char)*(lSize+1));
    if(config_file_buffer==NULL)
    {
        fputs("Error allocating memory for config_file_buffer\n",stderr);
        exit(1);
    }

    long result = fread(config_file_buffer,1,lSize,config_ptr);
    if(result!=lSize)
    {
        fputs("Error reading\n",stderr);
        exit(1);
    }
    fclose(config_ptr);

    config_file_buffer[lSize] = '\0';

    char* start_upstr = strstr(config_file_buffer,"upstream_dns_ip=");
    if (start_upstr == NULL) {
        fputs("Error: 'upstream_dns_ip=' not found in config file\n", stderr);
        exit(1);
    }
    char* start_bl = strstr(config_file_buffer,"blacklist=");
    if (start_bl == NULL) {
        fputs("Error: 'blacklist=' not found in config file\n", stderr);
        exit(1);
    }    
    char* start_resp = strstr(config_file_buffer,"response=");
    if (start_resp == NULL) {
        fputs("Error: 'response=' not found in config file\n", stderr);
        exit(1);
    }   
        
    start_upstr+=strlen("upstream_dns_ip=");
    start_bl+=strlen("blacklist=");
    start_resp+=strlen("response=");
    char* end_upstr =strchr(start_upstr,'\n');
    char* end_bl =strchr(start_bl,'\n');
    char* end_resp =strchr(start_resp,'\n');

    if(start_upstr==end_upstr)
    {
        fputs("Error: upstream_dns_ip is blank\n", stderr);
        exit(1);
    }

    int len_upstr=end_upstr-start_upstr;
    int len_bl=end_bl-start_bl;
    int len_resp=end_resp-start_resp;

    char* upstream_dns_ip=(char*)malloc(len_upstr+1);
    upstream_dns_ip[len_upstr] = '\0';
    char* blacklist=(char*)malloc(len_bl+1);
    blacklist[len_bl] = '\0';
    char* response=(char*)malloc(len_resp+1);
    response[len_resp] = '\0';

    strncpy(upstream_dns_ip,start_upstr,len_upstr);
    strncpy(blacklist,start_bl,len_bl);
    strncpy(response,start_resp,len_resp);

    free(config_file_buffer);

    char* temp_blacklist=malloc(strlen(blacklist)+1);
    strcpy(temp_blacklist, blacklist);

    int num_tokens = 0;
    char* temp_token = strtok(temp_blacklist, ",");
    while (temp_token != NULL) {
        num_tokens++;
        temp_token = strtok(NULL, ",");
    }
    free(temp_blacklist);

    char** blacklist_filtered;
    char* token;
    if(num_tokens>0)
    {
        blacklist_filtered = (char**)malloc(num_tokens*sizeof(char*));
        if(blacklist_filtered==NULL)
        {
            fputs("Error allocating memory for blacklist_filtered\n",stderr);
            exit(1);
        }
        token=strtok(blacklist,",");
        for(int i=0;token!=NULL;i++)
        {
            int token_len=strlen(token);
            blacklist_filtered[i]=(char*)malloc(token_len+1);
            if(blacklist_filtered[i]==NULL)
            {
                fputs("Error allocating memory for blacklist_filtered\n",stderr);
                exit(1);
            }
            strcpy(blacklist_filtered[i],token);
            token=strtok(NULL,",\n");
        }    
        res->black_list=blacklist_filtered;
    }
    else
    {
        fputs("Blacklist is blank\n",stdout);
        res->black_list=NULL;
    }

    
    free(blacklist);
    
    res->upstream_dns_ip=upstream_dns_ip;
    res->response=response;
    res->bl_tokens=num_tokens;
    

    return res;
}

void free_config(config* conf) 
{
    if (conf == NULL) return;

    if (conf->black_list != NULL) 
    {
        for (int i = 0; i < conf->bl_tokens; i++) 
        {
            free(conf->black_list[i]);
        }
        free(conf->black_list); 
    }
    free(conf->upstream_dns_ip);
    free(conf->response);
    free(conf); 
}

void start(config *conf)
{
    int client_sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if (client_sockfd < 0) {
        perror("client_socket creation failed");
        exit(1);
    }

    struct sockaddr_in servaddr,cliaddr,upstream_dns_addr; 
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    memset(&upstream_dns_addr, 0, sizeof(upstream_dns_addr));

    upstream_dns_addr.sin_family=AF_INET;
    upstream_dns_addr.sin_addr.s_addr=inet_addr(conf->upstream_dns_ip);
    upstream_dns_addr.sin_port=htons(53);

    servaddr.sin_family=AF_INET;
    servaddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    servaddr.sin_port=htons(53);

    if (bind(client_sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("client socket bind failed");
        exit(1);
    }


    int n;
    char buff[512];
    char buff_copy[512];

    socklen_t cli_len,upstream_len;
    while(1)
    {
        cli_len=sizeof(cliaddr);
        n=recvfrom(client_sockfd,buff,sizeof(buff),0,(struct sockaddr*)&cliaddr,&cli_len);
        if(n<0)
        {
            puts("recvfrom failed");
            continue;
        }

        memcpy(buff_copy,buff,n);

        DNS_HEADER *dns_hdr=(DNS_HEADER*)buff_copy;

        dns_hdr->id = ntohs(dns_hdr->id);
        dns_hdr->flags = ntohs(dns_hdr->flags);
        dns_hdr->qd_count = ntohs(dns_hdr->qd_count);
        dns_hdr->an_count = ntohs(dns_hdr->an_count);
        dns_hdr->ns_count = ntohs(dns_hdr->ns_count);
        dns_hdr->ar_count = ntohs(dns_hdr->ar_count);

        unsigned char*qname_ptr=(unsigned char*)(buff_copy+sizeof(DNS_HEADER));
        int qname_size=0;
        unsigned char* name =Read_Name(qname_ptr,(unsigned char*)buff_copy,&qname_size);

        bool found=0;
        for(int i=0;i<conf->bl_tokens;i++)
        {
            if (strcmp((const char*)name, conf->black_list[i]) == 0) {
                found = 1; 
                break;
            }
        }
        free(name);
        if(found)
        {
            DNS_HEADER *response_hdr=(DNS_HEADER*)buff_copy;
            short rcode=5;
            char* response=conf->response;
            if (strcasecmp(response, "noerror") == 0) {rcode = 0;} 
            else if (strcasecmp(response, "formerror") == 0) {rcode = 1;} 
            else if (strcasecmp(response, "servfail") == 0) {rcode = 2;} 
            else if (strcasecmp(response, "nxdomain") == 0) {rcode = 3;}
            else if (strcasecmp(response, "notimp") == 0) {rcode = 4;} 
            else if (strcasecmp(response, "refused") == 0) {rcode = 5;} 
            else 
            {
                fprintf(stderr, "Warning: Unrecognized 'response' value in config.txt: '%s'. Defaulting to NXDOMAIN (RCODE 3).\n", response);
                rcode = 3;
            }

            unsigned short response_flags=0;// QR(1 bit) Opcode(4 bits) AA(1 bit) TC(1 bit) RD(1 bit) RA(1 bit) Z(3 bit) Rcode(4 bits)
            response_flags |= (1<<15); // QR
            response_flags |= (dns_hdr->flags & (30720)); // 11-14 bits Opcode, 30720 = 0111 1000 0000 0000 
            response_flags |= (dns_hdr->flags & (1<<8)); // RD 0000 0001 0000 0000
            response_flags |= rcode;
            
            response_hdr->id = htons(dns_hdr->id);
            response_hdr->flags = htons(response_flags);
            response_hdr->qd_count = htons(dns_hdr->qd_count);
            response_hdr->an_count = htons(0); 
            response_hdr->ns_count = htons(0);
            response_hdr->ar_count = htons(0);

            sendto(client_sockfd, (char *)buff_copy, sizeof(DNS_HEADER) + qname_size + sizeof(QUESTION),0, (const struct sockaddr *)&cliaddr,cli_len);
            
            continue;
        }
        else
        {
            upstream_len = sizeof(upstream_dns_addr);

            int temp_socket=socket(AF_INET,SOCK_DGRAM,0);
            if(temp_socket<1){fputs("Erro creating temp_socket\n",stderr);continue;}

            struct timeval time;
            time.tv_sec=2; // 2 seconds
            time.tv_usec=0;

            if (setsockopt(temp_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof time) < 0) {
                perror("setsockopt SO_RCVTIMEO failed");
                close(temp_socket); 
                continue;
            }

            char *upstream_response=(char*)malloc(512); 
            if(upstream_response==NULL)
            {
                fputs("Failed allocating memory for up_stream_response\n",stderr);
                close(temp_socket);
                continue;
            }
        
            int bytes_sent_upstream=sendto(temp_socket, (char *)buff, n ,0, (const struct sockaddr *)&upstream_dns_addr, upstream_len);
            if(bytes_sent_upstream<1){fputs("Erro sending packet to upstream_dns\n",stderr);continue;}  

            int bytes_received_from_upstream = recvfrom(temp_socket, (char *)upstream_response, 512, 0, (struct sockaddr *)&upstream_dns_addr, (socklen_t*)&upstream_len);
            if(bytes_received_from_upstream<1)
            {
            fputs("Erro reciving packet from upstream_dns\n",stderr);
            
            DNS_HEADER* error_response_hdr=(DNS_HEADER*)buff_copy;
            
            unsigned short response_flags=0;// QR(1 bit) Opcode(4 bits) AA(1 bit) TC(1 bit) RD(1 bit) RA(1 bit) Z(3 bit) Rcode(4 bits)
            response_flags |= (1<<15); // QR
            response_flags |= (dns_hdr->flags & (30720)); // 11-14 bits Opcode, 30720 = 0111 1000 0000 0000 
            response_flags |= (dns_hdr->flags & (1<<8)); // RD 0000 0001 0000 0000
            response_flags |= 2; // Rcode (SERVFAIL)
            
            error_response_hdr->id = htons(dns_hdr->id);
            error_response_hdr->flags = htons(response_flags);
            error_response_hdr->qd_count = htons(dns_hdr->qd_count);
            error_response_hdr->an_count = htons(0); 
            error_response_hdr->ns_count = htons(0);
            error_response_hdr->ar_count = htons(0);

            int bytes_sent_to_client = sendto(client_sockfd,(char*)buff_copy,sizeof(DNS_HEADER) + qname_size + sizeof(QUESTION),0,(const struct sockaddr*)&cliaddr,cli_len);
            if(bytes_sent_to_client<0){fputs("Erro sending packet to client\n",stderr);}  

            free(upstream_response);
            close(temp_socket);
            continue;
            }  

            int bytes_sent_to_client = sendto(client_sockfd,upstream_response,bytes_received_from_upstream,0,(const struct sockaddr*)&cliaddr,cli_len);
            if(bytes_sent_to_client<0){fputs("Erro sending packet to client\n",stderr);}  

            free(upstream_response);
            close(temp_socket); 
        }
    }

}
