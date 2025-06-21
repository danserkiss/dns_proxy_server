#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

typedef struct 
{
    char** black_list;
    int bl_tokens;
    char* upstream_dns_ip;
    char* response;
}config;

config* import_config()
{
    config *res=(config*)malloc(sizeof(config));
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
    free(conf);


    return 0;
}

