#ifndef _INFLUXDB_H_
#define _INFLUXDB_H_

#ifdef __cplusplus
extern "C" {
#endif

#define INFLUXDB_URL_MAX_LEN 4096
#define WITHOUT_LOGIN        0
#define WITH_LOGIN           1

typedef struct _influxdb_string {
    char *str;
    int len;
}influxdb_string_s;

typedef struct _influxdb_client {
    char schema[8];
    char host[32];
    char username[32];
    char password[32];
    char dbname[32];
    char ssl;
}influxdb_client_s;

/* 是否打印调试信息 */
void set_influxdb_debug(int _debug);
/* 数据库对象初始化 */
influxdb_client_s *influxdb_client_new(char *host, char *username, char *password, char *dbname, char ssl);
/* 数据库对象释放空间 */
void influxdb_client_free(influxdb_client_s *client);
/* 创建数据库 */
int influxdb_create_database(influxdb_client_s *client, char *dbname, char login);
/* 删除数据库 */
int influxdb_delete_database(influxdb_client_s *client, char *dbname, char login);
/* 数据库插入数据 */
int influxdb_insert(influxdb_client_s *client, char *query, char login);
/* 数据库删除数据 */
int influxdb_delete(influxdb_client_s *client, char *query, char login);
/* 数据库查询数据 */
int influxdb_query(influxdb_client_s *client, char *query, influxdb_string_s *outstr, char login);

#ifdef __cplusplus
}
#endif

#endif
