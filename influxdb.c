#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "influxdb.h"

static int	_influxdb_client_get_url(influxdb_client_s *client, char (*buffer)[], int size, char *path, char login)
{
    char *uname_enc, *password_enc;

    uname_enc = curl_easy_escape(NULL, client->username, 0);
    password_enc = curl_easy_escape(NULL, client->password, 0);
    (*buffer)[0] = '\0';
    strncat(*buffer, client->schema, size);
    strncat(*buffer, "://", size);
    strncat(*buffer, client->host, size);
    strncat(*buffer, path, size);

	/* 如果需要登录，则加上账号和密码 */
	if(login != WITHOUT_LOGIN)
	{
		if(strchr(*buffer, '?') == NULL)
		{
			strncat(*buffer, "?u=", size);
		}
	    else
	    {
			strncat(*buffer, "&u=", size);
	    }
	    strncat(*buffer, uname_enc, size);
	    strncat(*buffer, "&p=", size);
	    strncat(*buffer, password_enc, size);
	}
    free(uname_enc);
    free(password_enc);

	if(IS_DEBUG)
	{
		printf("Do SQL:%s\n", *buffer);
	}
	
    return strlen(*buffer);
}

static int _writedata_curl(char *ptr, int size, int nmemb, void *userdata)
{
	char **buffer;
    int realsize;
	
	realsize = size * nmemb;
    if (userdata != NULL)
    {
        buffer = userdata;
        *buffer = realloc(*buffer, strlen(*buffer) + realsize + 1);
        strncat(*buffer, ptr, realsize);
    }
	
    return realsize;
}

static int _influxdb_curl(char *url, char *reqtype, char *body, char **response)
{
	long status_code;
    CURLcode c;
    CURL *handle;

	handle = curl_easy_init();
    if (reqtype != NULL)
    {
		/* 来设置自定义的请求方式，libcurl默认以GET方式提交请求。*/
        curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, reqtype);
    }
    curl_easy_setopt(handle, CURLOPT_URL, url); /* 设置URL */
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, _writedata_curl); /* 设置写数据的函数 */
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, response); /* 设置写数据的变量 */
    if (body != NULL)
    {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, body); /* 传递一个作为HTTP “POST”操作的所有数据的字符串 */
    }

    c = curl_easy_perform(handle);
    if (c == CURLE_OK)
    {
        status_code = 0;
        if (curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status_code) == CURLE_OK)
        {
            c = status_code;
        }
    }
    curl_easy_cleanup(handle);
	
    return c;
}

static int _influxdb_client_post(influxdb_client_s *client, char *path, char *body, char **res, char login)
{
    int status;
    char url[INFLUXDB_URL_MAX_LEN];
    char *buffer = calloc(1, sizeof (char));

    _influxdb_client_get_url(client, &url, sizeof(url), path, login);
    status = _influxdb_curl(url, NULL, body, &buffer);
    if (status >= 200 && status < 300 && res != NULL)
    {
        *res = buffer; 
    }
    free(buffer);
	
    return status;
}


/* 初始化字符串结构 */
static void _init_string(influxdb_string_s *s) 
{
	s->len = 0;
	s->str = (char*)malloc(s->len + 1);
	if (s->str == NULL) 
	{
		fprintf(stderr, "malloc() failed\n");
		exit(EXIT_FAILURE);
	}
	s->str[0] = '\0';

	return;
}

/* get_response_body 的写数据函数 */
static int _writedate_response(void *ptr, int size, int nmemb, void *userdata)
{
	int new_len;
	influxdb_string_s *outstr;

	outstr = (influxdb_string_s *)userdata;
	new_len = outstr->len + size * nmemb;
	outstr->str = (char*)realloc(outstr->str, new_len + 1);
	if (outstr->str == NULL) 
	{
		fprintf(stderr, "realloc() failed\n");
		exit(EXIT_FAILURE);
	}
	memcpy(outstr->str + outstr->len, ptr, size * nmemb);
	outstr->str[new_len] = '\0';
	outstr->len = new_len;
	
	return size * nmemb;
}

/* 供influxdb_query使用，查询结果以json串格式返回到outstr中 */
static int _get_response_body(char* url, influxdb_string_s *outstr)
{
	CURL *curl;

	curl = curl_easy_init();
	if(curl)
	{
		_init_string(outstr);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _writedate_response);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)outstr);
		curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	
	return 0;
}



/* 数据库对象初始化 */
influxdb_client_s *influxdb_client_new(char *host, char *username, char *password, char *dbname, char ssl)
{
    influxdb_client_s *client;

	client = malloc(sizeof (influxdb_client_s));
	if (!username)
	{
		username = "";
	}
	if (!password)
	{
		password = "";
	}
	if (!dbname)
	{
		free(client);
		client = NULL;
	}
	else 
	{
        if(ssl != 0)
        {
            sprintf(client->schema, "%s", "https");
        }
		else
		{
			sprintf(client->schema, "%s", "http");
		}
		sprintf(client->host, "%s", host);
		sprintf(client->username, "%s", username);
		sprintf(client->password, "%s", password);
		sprintf(client->dbname, "%s", dbname);
		client->ssl = ssl;
	}
	
    return client;
}

/* 数据库对象释放空间 */
void influxdb_client_free(influxdb_client_s *client)
{
    if (client)
    {
        free(client);
    }
}

/* 创建数据库 */
int influxdb_create_database(influxdb_client_s *client, char *dbname, char login)
{
    int ret;
	char buf[1024];
	
	sprintf(buf, "q=CREATE DATABASE %s", dbname);
    ret = _influxdb_client_post(client, "/query", buf, NULL, login);
	
    return ret;
}

/* 删除数据库 */
int influxdb_delete_database(influxdb_client_s *client, char *dbname, char login)
{
    int ret;
	char buf[1024];
	
	sprintf(buf, "q=DROP DATABASE %s", dbname);
	ret = _influxdb_client_post(client, "/query", buf, NULL, login);
	
	return ret;
}

/* 数据库插入数据 */
int influxdb_insert(influxdb_client_s *client, char *query, char login)
{
	int ret;
	char path[INFLUXDB_URL_MAX_LEN];

	ret = -1;
	if(client && query)
	{
		sprintf(path, "/write?db=%s", client->dbname);	
		ret = _influxdb_client_post(client, path, query, NULL, login);
	}
	
	return ret == 204;
}

/* 数据库删除数据 */
int influxdb_delete(influxdb_client_s *client, char *query, char login)
{	
	int ret;	
	char path[INFLUXDB_URL_MAX_LEN];
	char body[INFLUXDB_URL_MAX_LEN];	
	
	sprintf(path, "/query?db=%s", client->dbname);
	sprintf(body, "q=%s", query);	
	ret = _influxdb_client_post(client, path, body, NULL, login);	

	return ret;
}

/* 数据库查询数据 */
int influxdb_query(influxdb_client_s *client, char *query, influxdb_string_s *outstr, char login)
{
    int status;
	char *output;
    char url[INFLUXDB_URL_MAX_LEN];
	char path[INFLUXDB_URL_MAX_LEN];
	CURL *curl;

	curl = curl_easy_init();
	sprintf(path, "/query?db=%s&q=", client->dbname);
	if(curl) 
	{
		output = curl_easy_escape(curl, query, strlen(query));
		if(output) 
		{
			strncat(path, output, INFLUXDB_URL_MAX_LEN);
			curl_free(output);
		}
		curl_easy_cleanup(curl);
	}	
    _influxdb_client_get_url(client, &url, INFLUXDB_URL_MAX_LEN, path, login);    
	status = _get_response_body(url, outstr);	
	
    return status;
}
