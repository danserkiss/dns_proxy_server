#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct 
{
    char** black_list;
    int bl_tokens;
    char* upstream_dns_ip;
    char* response;
}config;

config import_config()
{
    config res;
    FILE *config_ptr;
    config_ptr=fopen("config.txt","r");
    if(config_ptr == NULL)
    {
        fputs("Error opening config file\n",stderr);
        exit(1);
    }

    fseek(config_ptr,0,SEEK_END);
    long lSize = ftell(config_ptr);
    rewind(config_ptr);

    char * config_file_buffer = (char*)malloc(sizeof(char)*lSize);
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

    char* start_upstr = strstr(config_file_buffer,"upstream_dns_ip=");
    if (start_upstr == NULL) {
        fputs("Error: 'upstream_dns_ip=' not found in config file\n", stderr);
        free(config_file_buffer);
        exit(1);
    }
    char* start_bl = strstr(config_file_buffer,"blacklist=");
    if (start_upstr == NULL) {
        fputs("Error: 'blacklist=' not found in config file\n", stderr);
        free(config_file_buffer);
        exit(1);
    }    
    char* start_resp = strstr(config_file_buffer,"response=");
    if (start_resp == NULL) {
        fputs("Error: 'response=' not found in config file\n", stderr);
        free(config_file_buffer);
        exit(1);
    }   
        
    start_upstr+=strlen("upstream_dns_ip=");
    start_bl+=strlen("blacklist=");
    start_resp+=strlen("response=");
    char* end_upstr =strstr(start_upstr,"\n");
    char* end_bl =strstr(start_bl,"\n");
    char* end_resp =strstr(start_resp,"\n");

    int len_upstr=strlen(start_upstr)-strlen(end_upstr);
    int len_bl=strlen(start_bl)-strlen(end_bl);
    int len_resp=strlen(start_resp)-strlen(end_resp);

    char* upstream_dns_ip=(char*)malloc(len_upstr);
    char* blacklist=(char*)malloc(len_bl);
    char* response=(char*)malloc(len_resp);

    strncpy(upstream_dns_ip,start_upstr,len_upstr);
    strncpy(blacklist,start_bl,len_bl);
    strncpy(response,start_resp,len_resp);

    free(config_file_buffer);

    char temp_blacklist[strlen(blacklist) + 1];
    strcpy(temp_blacklist, blacklist);

    int num_tokens = 0;
    char* temp_token = strtok(temp_blacklist, ",");
    while (temp_token != NULL) {
        num_tokens++;
        temp_token = strtok(NULL, ",");
    }

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
            blacklist_filtered[i]=(char*)malloc(strlen(token)+1);
            if(blacklist_filtered[i]==NULL)
            {
                fputs("Error allocating memory for blacklist_filtered\n",stderr);
                exit(1);
            }
            strcpy(blacklist_filtered[i],token);
            token=strtok(NULL,",");
        }    
        res.black_list=blacklist_filtered;
    }
    else
    {
        fputs("Blacklist is blank\n",stdout);
        res.black_list=NULL;
    }

    
    free(blacklist);
    
    res.upstream_dns_ip=upstream_dns_ip;
    res.response=response;
    res.bl_tokens=num_tokens;
    

    return res;
}

int main(int argc,char *argv[])
{
    config conf= import_config();
    printf("%s\n",conf.black_list[0]);
    printf("%s\n",conf.black_list[1]);
    printf("%s\n",conf.black_list[2]);

    free(conf.upstream_dns_ip);
    free(conf.response);
    if (conf.black_list != NULL) { 
        for (int i = 0; i < conf.bl_tokens; ++i) {
            if (conf.black_list[i] != NULL) {
                free(conf.black_list[i]);
            }
        }
        free(conf.black_list); 
    }
    return 0;
}

