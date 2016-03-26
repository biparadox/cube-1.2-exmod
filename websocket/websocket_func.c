#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <libwebsockets.h>

#include "../include/kernel/data_type.h"
#include "../include/kernel/struct_deal.h"
#include "../include/kernel/crypto_func.h"
#include "../include/ex_module.h"

#include "websocket_func.h"

struct websocket_server_context
{
       void * server_context;  //interface's hub
       void * callback_context;
       void * callback_interface;
       void * syn_template;
       void * connect_syn;
       char * websocket_message;
       int message_len;
       BYTE *read_buf;
       int  readlen;
       BYTE *write_buf;
       int  writelen;
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
			ws_context->read_buf = (unsigned char *)malloc(len);
			if(ws_context->read_buf==NULL)
				return -EINVAL;
			ws_context->readlen=len;
			memcpy(ws_context->read_buf,in,len);
			break;
		}
		case LWS_CALLBACK_SERVER_WRITEABLE:
		{
			BYTE * buf= (unsigned char *)malloc(
				LWS_SEND_BUFFER_PRE_PADDING+ws_context->writelen+
				LWS_SEND_BUFFER_POST_PADDING);
			if(buf==NULL)
				return -EINVAL;			
			memcpy(&buf[LWS_SEND_BUFFER_PRE_PADDING],
				ws_context->write_buf,
				ws_context->writelen);
			libwebsocket_write(wsi,
				&buf[LWS_SEND_BUFFER_PRE_PADDING],
				ws_context->writelen,LWS_WRITE_TEXT);
			free(buf);
			break;
		}
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
		"cube_wsport",
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
    char * server_addr=local_websocketserver_addr;
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

    int stroffset=0;

    struct_template=create_struct_template(&connect_syn_desc);
    if(struct_template==NULL)
	return -EINVAL;

   str_offset=struct_2_json(syn_info,buffer,struct_template);
   if(str_offset<0)
	return str_offset;
	

    ws_context->websocket_message=malloc(str_offset+1);
    memcpy(ws_context->websocket_message,buffer,str_offset);
   ws_context->websocket_message[str_offset]=0;
    ws_context->message_len=str_offset;

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
    conn_val.tv_usec=time_val.tv_usec;

    char local_uuid[DIGEST_SIZE*2+1];
    char proc_name[DIGEST_SIZE*2+1];
    char buffer[4096];
    memset(buffer,0,4096);
    int stroffset;
	
    printf("begin websocket server process!\n");
    ret=proc_share_data_getvalue("uuid",local_uuid);
    if(ret<0)
        return ret;
    ret=proc_share_data_getvalue("proc_name",proc_name);

    if(ret<0)
	return ret;

    printf("starting wsport server ...\n");

    for(i=0;i<500*1000;i++)
    {
	 libwebsocket_service(ws_context->server_context,50);
	 // check if there is something to read
	 if(ws_context->readlen>0)
	{
		do {
	    	 	void * message;
			ret=json_2_message(ws_context->read_buf,&message);
		   	if(ret>=0)
		    	{
				if(message_get_state(message)==0)
					message_set_state(message,MSG_FLOW_INIT);
				set_message_head(message,"sender_uuid",local_uuid);
	    	    		sec_subject_sendmsg(sub_proc,message);	
		    	}
			break;
		}while(1);
	}
	// send message to the remote
	while(sec_subject_recvmsg(sub_proc,&message_box)>=0)
	{
		if(message_box==NULL)
			break;
    		stroffset=message_2_json(message_box,buffer);
		if(stroffset>0)
			memcpy(ws_context->write_buf,buffer,stroffset);
			ws_context->writelen=stroffset;
		libwebsocket_callback_on_writable(ws_context->callback_context	
,ws_context->callback_interface);

	}

    }
    return 0;
}
