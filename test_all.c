#include "influxdb.h"
#include <stdio.h>

int main() 
{
    int status;
    influxdb_string_s outstr;	
    influxdb_client_s *client = influxdb_client_new("localhost:8086", "root", "toor", "mydb", 0);
    
    /*create db*/
    status = influxdb_create_database(client, "mydb", WITH_LOGIN);	
	printf("status=%d\n",status);
    /*do insert*/
    status = influxdb_insert(client,"cpu_load,host=server_1,region=us-west value=0.2", WITH_LOGIN);		
	printf("status=%d\n",status);
    
    /*do query*/
    influxdb_query(client,"select * from cpu_load limit 10",&outstr, WITH_LOGIN);
	printf("%s\n",outstr.str);
    
    /*delete db*/
    status = influxdb_delete_database(client,"mydb", WITH_LOGIN);	
	printf("status=%d\n",status);
    
    influxdb_client_free(client);	
    return 0;
}

