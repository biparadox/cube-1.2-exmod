/**
 * Copyright [2015] Tianfu Ma (matianfu@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * File: main.c
 *
 * Created on: Jun 5, 2015
 * Author: Tianfu Ma (matianfu@gmail.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#include "../include/kernel/errno.h"
#include "../include/kernel/data_type.h"
#include "../include/kernel/alloc.h"
#include "../include/kernel/basefunc.h"
#include "../include/kernel/struct_deal.h"
#include "../include/kernel/channel.h"
/*
#include "../include/memdb.h"
#include "../include/message.h"
#include "../include/routine.h"
*/

#include "../include/ex_module.h"
#include "websocket_func.h"

static struct timeval time_val={0,50*1000};

struct test_para
{
	void * channel;	
};

int test_module_init(void * ex_module,void * para)
{
	struct test_para * my_para=para;
	

	ex_module_setpointer(ex_module,para);

	return 0;
}

int test_module_start(void * ex_module,void * para)
{
	printf("test ex_module start !\n");
	int i;
	int offset;
	int total=0;
	struct test_para  * my_para= ex_module_getpointer(ex_module);
	if(my_para==NULL)
		return -EINVAL;
	char buf[2000];

	for(i=0;i<500*1000;i++)
	{
		offset=channel_inner_read(my_para->channel,buf,2000);
		if(offset>0)
		{
			channel_inner_write(my_para->channel,buf,offset);
			total+=offset;
		}
		
		usleep(time_val.tv_usec);
	}	
	return total;
}

int main() {
  	static unsigned char alloc_buffer[4096*(1+1+4+1+16+1+256)];	

	void * ex_module;
	void * port_module;
	int ret;
	void * channel;	
	struct test_para  test_para;
	struct ws_port_para ws_para;

  	mem_init(alloc_buffer);
	struct_deal_init();

	ex_module_list_init();
	ex_module_create("test",0,NULL,&ex_module);


	ex_module_setinitfunc(ex_module,&test_module_init);
	ex_module_setstartfunc(ex_module,&test_module_start);

	ex_module_create("ws_port",0,NULL,&port_module);
	ex_module_setinitfunc(port_module,&websocket_port_init);
	ex_module_setstartfunc(port_module,&websocket_port_start);


	channel=channel_create("test_channel",CHANNEL_RDWR);
	if(channel==NULL)
		return -EINVAL;
	test_para.channel=channel;
	ws_para.channel=channel;
	ws_para.websocket_addr="0.0.0.0:13888";
	
	
	ex_module_init(ex_module,&test_para);
	ex_module_init(port_module,&ws_para);

	ex_module_start(ex_module,NULL);
	usleep(time_val.tv_usec);

	ex_module_start(port_module,NULL);
	ex_module_join(ex_module,&ret);
	ex_module_join(port_module,&ret);
	return ret;

}
