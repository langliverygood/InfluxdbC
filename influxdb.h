#ifndef _INFLUXDB_H_
#define _INFLUXDB_H_

#ifdef __cplusplus
extern "C" {
#endif

#define INFLUXDB_URL_MAX_LEN 4096
#define WITHOUT_LOGIN 0
#define WITH_LOGIN 1

#define influxdb_assign_string(dst,src) \
    dst=malloc(sizeof(char)*(strlen(src)+1));\
    if(dst)\
    {\
        strcpy(dst,src);\
    }

typedef struct _influxdb_string {
	char *str;
	size_t len;
}influxdb_string_s;

typedef struct _influxdb_client {
	char schema[8];
	char host[16];
	char username[32];
	char password[32];
	char dbname[32];
	char ssl;
}influxdb_client_s;

influxdb_client_s *influxdb_client_new(char *host, char *username, char *password, char *dbname, char ssl);
char *influxdb_strdup(const char *s);
void init_string(s_influxdb_string *s);
void influxdb_client_free(s_influxdb_client *client);
size_t writefunc(void *ptr,size_t size,size_t nmemb,s_influxdb_string *s);
int get_response_body(char* url,s_influxdb_string *outstr);
int influxdb_client_get(s_influxdb_client *client,char *path,char **res);
int influxdb_client_post(s_influxdb_client *client,
						char *path,
						char *body,
						char **res);

typedef void *(*influxdb_client_object_extract)(char *obj);

size_t influxdb_client_list_something(
	s_influxdb_client             *client,
	char                           *path,
	void                           ***list,
	influxdb_client_object_extract extractor
);

int influxdb_create_database(s_influxdb_client *client,char *database_name);
int influxdb_delete_database(s_influxdb_client *client,char *database_name);
int influxdb_insert(s_influxdb_client *client,char *query);	
int influxdb_delete(s_influxdb_client *client,char *query);	
int influxdb_query(s_influxdb_client *client,char *query,s_influxdb_string *outstr);

#ifdef __cplusplus
}
#endif
#endif
