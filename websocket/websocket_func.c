#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <libwebsockets.h>

#include "../include/kernel/data_type.h"
#include "../include/kernel_comp.h"
#include "../include/kernel/struct_deal.h"
#include "../include/kernel/channel.h"
#include "../include/kernel/crypto_func.h"
#include "../include/ex_module.h"

#include "websocket_func.h"

static struct timeval time_val={0,50*1000};
struct websocket_server_context
{
       void * server_context;  //interface's hub
       void * callback_context;
       void * callback_interface;
       void * syn_template;
       void * connect_syn;
       char * websocket_message;
       int message_len;
	
       void * channel;
};


static struct websocket_server_context * ws_context;  

struct connect_syn
{
	char uuid[DIGEST_SIZE*2];
	char * server_name;
	char * service;
	char * server_addr;
	int  flags;
	char nonce[DIGEST_SIZE];
}__attribute__((packed));

static struct struct_elem_attr connect_syn_desc[]=
{
	{"uuid",CUBE_TYPE_STRING,DIGEST_SIZE*2,NULL},
	{"server_name",CUBE_TYPE_ESTRING,256,NULL},
	{"service",CUBE_TYPE_ESTRING,64,NULL},
	{"server_addr",CUBE_TYPE_ESTRING,256,NULL},
	{"flags",CUBE_TYPE_INT,sizeof(UINT32),NULL},
	{"nonce",CUBE_TYPE_STRING,DIGEST_SIZE,NULL},
	{NULL,CUBE_TYPE_ENDDATA,0,NULL}
};

static int callback_http(	struct libwebsocket_context * this,
				struct libwebsocket * wsi,
				enum libwebsocket_callback_reasons reason,
				void * user,void * in,size_t len)
{
	return 0;
}
		
static int callback_cube_wsport(	struct libwebsocket_context * this,
				struct libwebsocket * wsi,
				enum libwebsocket_callback_reasons reason,
				void * user,void * in,size_t len)
{
	int i;
	switch(reason) {
		case LWS_CALLBACK_ESTABLISHED:
			ws_context->callback_interface=wsi;
			ws_context->callback_context=this;
			printf("connection established\n");
			BYTE * buf= (unsigned char *)malloc(
				LWS_SEND_BUFFER_PRE_PADDING+
				ws_context->message_len+
				LWS_SEND_BUFFER_POST_PADDING);
			if(buf==NULL)
				return -EINVAL;			
			memcpy(&buf[LWS_SEND_BUFFER_PRE_PADDING],
				ws_context->websocket_message,
				ws_context->message_len);
			libwebsocket_write(wsi,
				&buf[LWS_SEND_BUFFER_PRE_PADDING],
				ws_context->message_len,LWS_WRITE_TEXT);
			free(buf);
			break;
		case LWS_CALLBACK_RECEIVE:
		{
			channel_write(ws_context->channel,in,len);
			break;
		}
		case LWS_CALLBACK_SERVER_WRITEABLE:
			break;
		default:
			break;
	}
	return 0;
}

static struct libwebsocket_protocols protocols[] = {
	{
		"http_only",
		callback_http,
		0	
	},
	{
		"cube-wsport",
		callback_cube_wsport,
		0
	},
	{
		NULL,NULL,0
	}
};


int websocket_port_init(void * sub_proc,void * para)
{

    int ret;
    struct libwebsocket_context * context;
    struct lws_context_creation_info info;
    void * struct_template;

    struct ws_port_para * my_para=para;
    if(para==NULL)
	return -EINVAL;
		

    ws_context=malloc(sizeof(struct websocket_server_context));
    if(ws_context==NULL)
	return -ENOMEM;
    memset(ws_context,0,sizeof(struct websocket_server_context));

    memset(&info,0,sizeof(info));
    info.port=13888;
    info.iface=NULL;
    info.protocols=protocols;
    info.extensions=libwebsocket_get_internal_extensions();
    info.ssl_cert_filepath=NULL;
    info.ssl_private_key_filepath=NULL;
    info.gid=-1;
    info.uid=-1;
    info.options=0;

    // parameter deal with
    char * server_name="websocket_server";
    char * service="ws_port";
    char * server_addr=my_para->websocket_addr;
    char * nonce="AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

    struct connect_syn * syn_info;
    syn_info=malloc(sizeof(struct connect_syn));
    if(syn_info==NULL)
        return -ENOMEM;
    memset(syn_info,0,sizeof(struct connect_syn));

    syn_info->server_name=server_name;
    syn_info->service=service;
    syn_info->server_addr=server_addr;

    char buffer[1024];
    memset(buffer,0,1024);

    int str_offset=0;

    struct_template=create_struct_template(&connect_syn_desc);
    if(struct_template==NULL)
	return -EINVAL;

   str_offset=struct_2_json(syn_info,buffer,struct_template);
   if(str_offset<0)
	return str_offset;
	

    ws_context->websocket_message=kmalloc(str_offset+1,GFP_KERNEL);
    memcpy(ws_context->websocket_message,buffer,str_offset);
    ws_context->websocket_message[str_offset]=0;
    ws_context->message_len=str_offset;

    ws_context->channel=my_para->channel;

    context = libwebsocket_create_context(&info);
    if(context==NULL)
    {
	printf(" wsport context create error!\n");
	return -EINVAL;
    }
    ws_context->server_context=context;
	
    return 0;
}

int websocket_port_start(void * sub_proc,void * para)
{
    int ret;
    int retval;
    void * message_box;
    void * context;
    int i;
    struct timeval conn_val;
    int offset;
    conn_val.tv_usec=time_val.tv_usec;

    char local_uuid[DIGEST_SIZE*2+1];
    char proc_name[DIGEST_SIZE*2+1];
    char buf[4096];
    memset(buf,0,4096);
    int stroffset;
	
    printf("starting wsport server ...\n");

    for(i=0;i<500*1000;i++)
    {
	 libwebsocket_service(ws_context->server_context,50);
	 // check if there is something to read

	// send message to the remote

	do
	{
	
		offset=channel_read(ws_context->channel,
			&buf[LWS_SEND_BUFFER_PRE_PADDING],1024);

		if(offset<=0)
			break;
		if(offset>0)
			libwebsocket_write(ws_context->callback_interface,
				&buf[LWS_SEND_BUFFER_PRE_PADDING],
				offset,LWS_WRITE_TEXT);

	}while(1);
	usleep(time_val.tv_usec);

    }
    return 0;
}
