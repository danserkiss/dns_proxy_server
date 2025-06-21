#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>


typedef struct 
{
    unsigned short id:16;

    unsigned char qr:1;
    unsigned char opcode:4;
    unsigned char aa:1;
    unsigned char tc:1;
    unsigned char rd:1;
    unsigned char ra:1;
    unsigned char z:3;
    unsigned char rcode:4;

    unsigned short qd_count:16;
    unsigned short an_count:16;
    unsigned short ns_count:16;
    unsigned short ar_count:16;

}DNS_HEADER;

typedef struct 
{
    unsigned short QT;
    unsigned short QC;

}QUESTION;

#pragma pack(push, 1)
struct R_DATA
{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
};
#pragma pack(pop)

struct RES_RECORD
{
    unsigned char *name;
    struct R_DATA *resource;
    unsigned char *rdata;
};

typedef struct 
{
    unsigned char* name;
    QUESTION *ques;
}QUERY;


typedef struct 
{
    char** black_list;
    int bl_tokens;
    char* upstream_dns_ip;
    char* response;
}config;

char* Read_Name(unsigned char* reader,unsigned char* buffer,int *count)
{
    unsigned char* name;
    unsigned int p=0,jumped=0,offset;
    int i,j;

    *count=1;
    name=(unsigned char*)malloc(256);

    name[0]='\0';

    while (*reader!=0)
    {
        if(*reader>=192) //  192 = 1100 0000(first two bytes) 
        {
            offset=(*reader)+*(reader+1)-49152; // 49152 = 1100 0000 0000 0000   (14 bits after 11 it`s offset)
            reader=buffer + offset-1;
            jumped=1;
        }
        else
        {
            name[p++]=*reader;
        }
        reader=reader+1;

        if(jumped == 0)
        {
            *count=*count+1;
        }
    }
    name[p]='\0';

    if(jumped==1)
    {
        *count=*count+1;
    }

    for(i=0;i<(int)strlen((const char*)name);i++) 
    {
        p=name[i];
        for(j=0;j<(int)p;j++) 
        {
            name[i]=name[i+1];
            i=i+1;
        }
        name[i]='.';
    }
    name[i-1]='\0';
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

    if(start_upstr==end_upstr || start_resp==end_resp)
    {
        fputs("Error: upstream_dns_ip or response is blank\n", stderr);
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



int main(int argc,char *argv[])
{
    config *conf= import_config();   

    int sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }

    struct sockaddr_in servaddr,cliaddr; 
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family=AF_INET;
    servaddr.sin_addr.s_addr=INADDR_ANY;
    servaddr.sin_port=53;

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(1);
    }


    int n;
    char buff[512];
    while(1)
    {
        n=recvfrom(sockfd,buff,sizeof(buff),0,&cliaddr,sizeof(cliaddr));
        if(n<0)
        {
            puts("recvfrom failed");
            continue;
        }

        DNS_HEADER *dns_hdr=(DNS_HEADER*)buff;

        dns_hdr->id = ntohs(dns_hdr->id);
        dns_hdr->qd_count = ntohs(dns_hdr->qd_count);
        dns_hdr->an_count = ntohs(dns_hdr->an_count);
        dns_hdr->ns_count = ntohs(dns_hdr->ns_count);
        dns_hdr->ar_count = ntohs(dns_hdr->ar_count);

        if(dns_hdr->qr !=0)
        {
            puts("Ignored non-query");
            continue;
        }

        char*qname_ptr=(char*)(buff+sizeof(DNS_HEADER));
        short qname_size=0;
        char* name =Read_Name(qname_ptr,buff,&qname_size);

        bool found=0;
        for(int i=0;i<conf->bl_tokens;i++)
        {
            if(name==conf->black_list[i]){found=1;break;};
        }
        if(found)
        {
            dns_hdr->qr=1;
            dns_hdr->aa=1;
            dns_hdr->rd=1;
            dns_hdr->tc=1;
            dns_hdr->z=1;
            dns_hdr->ra=1;

            dns_hdr->rcode=5;

            dns_hdr->an_count=0;
            dns_hdr->ns_count=0;
            dns_hdr->ar_count=0;

            dns_hdr->id=htons(dns_hdr->id);
            dns_hdr->qd_count=htons(dns_hdr->qd_count);
            dns_hdr->an_count=htons(dns_hdr->an_count);
            dns_hdr->ns_count=htons(dns_hdr->ns_count);
            dns_hdr->ar_count=htons(dns_hdr->ar_count);

            sendto(sockfd, (char *)buff, sizeof(DNS_HEADER) + qname_size + sizeof(QUESTION),0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr));            continue;
        }



    }


    free(conf);

    return 0;
}

