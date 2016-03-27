
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../include/kernel_comp.h"
#include "../include/kernel/data_type.h"
#include "../include/kernel/list.h"
#include "../include/kernel/attrlist.h"
#include "../include/connector.h"

#define MAX_CHANNEL_SIZE	1024 
//struct connectorectod_ops connector_af_inet_info_ops;
struct connector_af_inet_info
{
	struct sockaddr_in adr_inet;
	int len_inet;
	void * sem_struct;
};

struct connector_af_inet_server_info
{
	int channel_num;
	Record_List channel_list;
	struct list_head * curr_channel;
};

struct connector_af_inet_channel_info
{
	void * server;
};

void * create_channel_af_inet_info(void * server_info)
{
	struct connector_af_inet_info * src_info;
	struct connector_af_inet_info * new_info;
	src_info=(struct connector_af_inet_info *)server_info;
	new_info=(struct connector_af_inet_info *)kmalloc(sizeof (struct connector_af_inet_info),GFP_KERNEL);
	if(new_info == NULL)
		return NULL;
	memset(new_info,0,sizeof (struct connector_af_inet_info));
	memcpy(&new_info->adr_inet,&src_info->adr_inet,src_info->len_inet);
	new_info->len_inet=src_info->len_inet;
	return new_info;
}


int	af_inet_formaddr(struct sockaddr_in * adr_inet, int * len,char * addrstring)
{
	char addrbuf[108];
	int i,retval;
	int namelen;

	memset(adr_inet,0,sizeof(struct sockaddr_in));

	adr_inet->sin_family = AF_INET;

	namelen=strlen(addrstring);
	for(i=0;i<namelen;i++)
	{
		if(addrstring[i]==':')
			break;
		addrbuf[i]=addrstring[i];
	}
	if(i==namelen)
	{
		printf("no port number!\n");
		return -EINVAL;
	}
		
	addrbuf[i++]=0;

	*len = sizeof (struct sockaddr_in);

	adr_inet->sin_addr.s_addr=inet_addr(addrbuf);
	if(adr_inet->sin_addr.s_addr==INADDR_NONE)
		return -EINVAL;

	memcpy(addrbuf,addrstring+i,namelen-i+1);

	adr_inet->sin_port=htons(atoi(addrbuf));
	
	return 0;
}


char *  af_inet_getaddrstring(struct sockaddr_in * adr_inet)
{
	char addrbuf[128];
	int i,retval;
	int namelen;
	char *cp;
	char * addrstring;

	cp=inet_ntoa(adr_inet->sin_addr);

	sprintf(addrbuf,":%d",adr_inet->sin_port);

	namelen=strlen(cp)+strlen(addrbuf);

	addrstring=kmalloc(namelen+1,GFP_KERNEL);
	if(addrstring==NULL)
		return -ENOMEM;

	strcpy(addrstring,cp);
	strcat(addrstring,addrbuf);

	return addrstring;
}




int  connector_af_inet_info_init (void * connector,char * addr)
{

	// name format: X.X.X.X:port

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	char addrbuf[108];
	int len_inet;
	int retval;
	int i;
	int namelen;
	


	this_conn=(struct tcloud_connector *)connector;


	if(this_conn->conn_protocol!=AF_INET)
		return -EINVAL;
	if(this_conn->conn_type==CONN_CHANNEL)
		return 0;


	base_info=kmalloc(sizeof(struct connector_af_inet_info),GFP_KERNEL);
	if(base_info==NULL)
		return -ENOMEM;
	memset(base_info,0,sizeof(struct connector_af_inet_info));
	this_conn->conn_base_info=base_info;
	

    	this_conn->conn_fd  = socket(AF_INET,SOCK_STREAM,0);
	if(this_conn->conn_fd <0)
		return this_conn->conn_fd;

	int yes=1;
	if(setsockopt(this_conn->conn_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))==-1)
	{
			printf("setsockopt reuseaddr error!\n");
			return -EINVAL;
	}
	struct timeval timeout ={0,1000};   // we hope each read only delay 10 microsecond
	if(setsockopt(this_conn->conn_fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))==-1)
	{
			printf("setsockopt timeout error!\n");
			return -EINVAL;
	}
/*
* Form an AF_INET Address:
*/

	af_inet_formaddr(&base_info->adr_inet,&base_info->len_inet,addr);

/*
* Now bind the address to the socket:
*/
	retval = bind(this_conn->conn_fd,
		(struct sockaddr *)&(base_info->adr_inet),
		base_info->len_inet);
	if ( retval == -1 )
		return -ENONET;

	return this_conn->conn_fd;
}


int  connector_af_inet_client_init (void * connector,char * name,char * addr)
{

	struct tcloud_connector * this_conn;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	this_conn->conn_name=kmalloc(strlen(name)+1,GFP_KERNEL);
	if(this_conn->conn_name==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_name,name);

	this_conn->conn_addr=kmalloc(strlen(addr)+1,GFP_KERNEL);
	if(this_conn->conn_addr==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_addr,addr);
	
	connector_af_inet_info_init (connector,addr);
	
	return 0;
};

int  connector_af_inet_server_init (void * connector,char * name,char * addr)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	struct connector_af_inet_server_info * server_info;

	int retval;

	this_conn=(struct tcloud_connector *)connector;

	this_conn->conn_name=kmalloc(strlen(name)+1,GFP_KERNEL);
	if(this_conn->conn_name==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_name,name);
	
	this_conn->conn_addr=kmalloc(strlen(addr)+1,GFP_KERNEL);
	if(this_conn->conn_addr==NULL)
		return -ENOMEM;
	strcpy(this_conn->conn_addr,addr);
	
	connector_af_inet_info_init (connector,addr);
	base_info=(struct connector_af_inet_info *)(this_conn->conn_base_info);

	server_info=kmalloc(sizeof(struct connector_af_inet_server_info),GFP_KERNEL);
	if(server_info==NULL)
		return -ENOMEM;
	memset(server_info,0,sizeof(struct connector_af_inet_server_info));

	INIT_LIST_HEAD(&(server_info->channel_list.list));	
	server_info->curr_channel=&(server_info->channel_list.list);

	this_conn->conn_var_info=server_info;
	
	return 0;
}


int  connector_af_inet_listen (void * connector)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_type!=CONN_SERVER)
		return -EINVAL;
	if(this_conn->conn_protocol!=AF_INET)
		return -EINVAL;
	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	this_conn->conn_state=CONN_SERVER_LISTEN;
	return listen(this_conn->conn_fd,10);
}

void * connector_af_inet_accept (void * connector)
{

	struct tcloud_connector * this_conn;
	struct tcloud_connector * channel_conn;
	struct connector_af_inet_info * base_info;
	struct connector_af_inet_server_info * server_info;
	struct connector_af_inet_info * channel_base_info;
	struct connector_af_inet_channel_info * channel_info;
	int retval;
	int accept_fd;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_type!=CONN_SERVER)
		return -EINVAL;
	if(this_conn->conn_protocol!=AF_INET)
		return -EINVAL;
	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	server_info=(struct connector_af_inet_server_info * )this_conn->conn_var_info;

	accept_fd= accept(this_conn->conn_fd,NULL,0);
	if(accept_fd<=0)
		return NULL;

	channel_conn=get_connector(CONN_CHANNEL,AF_INET);
	if(channel_conn==NULL)
		return NULL;
	channel_base_info = create_channel_af_inet_info(base_info);
	channel_conn->conn_fd=accept_fd;

	channel_info=kmalloc(sizeof(struct connector_af_inet_channel_info),GFP_KERNEL);
	if(channel_info==NULL)
		return NULL;
	channel_info->server=this_conn;
	channel_conn->conn_base_info=channel_base_info;
	channel_conn->conn_var_info=channel_info;

//      get channel's peer addr begin
	struct sockaddr_in peeraddr;
	int len_peeraddr=128;
	getpeername(accept_fd,&peeraddr,&len_peeraddr);

	channel_conn->conn_peeraddr=af_inet_getaddrstring(&peeraddr);



//      get channel's peer addr end


	struct list_head * head, *currlib;
	Record_List * record_elem;

	record_elem = kmalloc(sizeof(Record_List),GFP_KERNEL);
	if(record_elem==NULL)
		return -ENOMEM;
	INIT_LIST_HEAD(&(record_elem->list));
	record_elem->record=channel_conn;

	server_info->channel_num++;
	channel_conn->conn_state=CONN_CHANNEL_ACCEPT;

	return channel_conn;

}

int connector_af_inet_close_channel(void * connector,void * channel)
{
	struct tcloud_connector * this_conn;
	struct tcloud_connector * channel_conn;
	struct connector_af_inet_info * base_info;
	struct connector_af_inet_server_info * server_info;
	struct connector_af_inet_info * channel_base_info;
	struct connector_af_inet_channel_info * channel_info;
	int retval;
	int accept_fd;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_type!=CONN_SERVER)
		return -EINVAL;
	if(this_conn->conn_protocol!=AF_INET)
		return -EINVAL;
	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	server_info=(struct connector_af_inet_server_info * )this_conn->conn_var_info;

	channel_conn= (struct tcloud_connector *)channel;

	channel_base_info = channel_conn->conn_base_info;

	struct list_head * head, *curr;
	Record_List * record_elem;
	Record_List * conn_list;


	conn_list=(Record_List *)&server_info->channel_list;
	head = &(conn_list->list);

	curr=head->next;
	while(curr!=head)
	{
		record_elem=(Record_List *)curr;
		if(record_elem->record ==channel)
			break;
		curr=curr->next;
	}

	if(curr==head)
		return -EINVAL;

	if(server_info->curr_channel==curr)
		server_info->curr_channel=curr->next;
	list_del(curr);
	kfree(record_elem);
	channel_conn->conn_state=CONN_CHANNEL_CLOSE;
	return 0;
}

int  connector_af_inet_connect (void * connector)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_type!=CONN_CLIENT)
		return -EINVAL;

	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	retval = connect(this_conn->conn_fd,&base_info->adr_inet,base_info->len_inet);
	if(retval<0)
		return retval;
	this_conn->conn_state=CONN_CLIENT_CONNECT;
	return retval;

}

int  connector_af_inet_read (void * connector,void * buf, size_t count)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;

	retval= read(this_conn->conn_fd,buf,count);
	if(retval<0)
		return retval;
	if((this_conn->conn_type == CONN_CLIENT) &&(this_conn->conn_type == CONN_CLIENT_CONNECT))
		this_conn->conn_state=CONN_CLIENT_RESPONSE;
	return retval;

}

int  connector_af_inet_write (void * connector,void * buf,size_t count)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	return write(this_conn->conn_fd,buf,count);
}

int  connector_af_inet_wait (void * connector,struct timeval * timeout)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_protocol!=AF_INET)
		return -EINVAL;
	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	return 0;
}

int  connector_af_inet_disconnect (void * connector)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_info * base_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;

	if(this_conn->conn_protocol!=AF_INET)
		return -EINVAL;
	base_info=(struct connector_af_inet_info * )this_conn->conn_base_info;
	shutdown(this_conn->conn_fd,SHUT_RDWR);
	close (this_conn->conn_fd);

	this_conn->conn_state=0x1000;
	return 0;
}

void * connector_af_inet_get_server(void * connector)
{

	struct tcloud_connector * this_conn;
	struct connector_af_inet_channel_info * channel_info;
	int retval;

	this_conn=(struct tcloud_connector *)connector;
	if(this_conn->conn_protocol!=AF_INET)
		return -EINVAL;
	if(this_conn->conn_type !=CONN_CHANNEL)
		return NULL;

	channel_info=(struct connector_af_inet_channel_info *)(this_conn->conn_var_info);
	return channel_info->server;

}


struct connector_ops connector_af_inet_server_ops = 
{
	.conn_type = CONN_SERVER,
	.init=connector_af_inet_server_init,
	.ioctl=NULL,	
	.getname=connector_getname,	
	.getaddr=connector_getaddr,	
	.getpeeraddr=connector_getpeeraddr,	
	.setname=connector_setname,	
	.listen=connector_af_inet_listen,	
	.accept=connector_af_inet_accept,	
//	.connect=connector_af_inet_connect,	
	.close_channel=connector_af_inet_close_channel,
	.read=connector_af_inet_read,	
	.write=connector_af_inet_write,	
	.getfd=connector_getfd,	
	.wait=connector_af_inet_wait,	
	.disconnect=connector_af_inet_disconnect,	
};

struct connector_ops connector_af_inet_client_ops = 
{
	.conn_type = CONN_CLIENT,
	.init=connector_af_inet_client_init,
	.ioctl=NULL,	
	.getname=connector_getname,	
	.getaddr=connector_getaddr,	
	.getpeeraddr=connector_getpeeraddr,	
	.setname=connector_setname,	
	.connect=connector_af_inet_connect,	
	.read=connector_af_inet_read,	
	.write=connector_af_inet_write,	
	.getfd=connector_getfd,	
	.wait=connector_af_inet_wait,	
	.disconnect=connector_af_inet_disconnect	
};

struct connector_ops connector_af_inet_channel_ops = 
{
	.conn_type = CONN_CHANNEL,
	.init=NULL,
	.ioctl=NULL,	
	.getname=connector_getname,	
	.getaddr=connector_getaddr,	
	.getpeeraddr=connector_getpeeraddr,	
	.setname=connector_setname,	
	.connect=connector_af_inet_connect,	
	.read=connector_af_inet_read,	
	.write=connector_af_inet_write,	
	.getfd=connector_getfd,	
	.wait=connector_af_inet_wait,	
	.getserver=connector_af_inet_get_server,
	.disconnect=connector_af_inet_disconnect	
};
