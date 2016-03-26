#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <pthread.h>

#include "../include/kernel/data_type.h"
#include "../include/kernel_comp.h"
#include "../include/kernel/list.h"
#include "../include/kernel/attrlist.h"
#include "../include/kernel/struct_deal.h"
#include "../include/connector.h"
#include "../include/ex_module.h"


typedef struct proc_ex_module
{
	EX_MODULE_HEAD head;
	pthread_t proc_thread; 
	pthread_attr_t thread_attr; 

	void * context;
	void * context_template;
	int  (*init)(void *,void *);
	int  (*start)(void *,void *);
	int  retval;
}__attribute__((packed)) EX_MODULE;

struct ex_module_list
{
	int state;
	Record_List head;
	struct list_head * curr;
}; 

static struct ex_module_list * ex_module_list;

int entity_comp_uuid(void * list_head, void * uuid) 
{                                                             
	struct list_head * head;
	EX_MODULE_HEAD * entity_head;    
	head=(struct list_head *)list_head;
	if(head==NULL)
		return -EINVAL;	
	Record_List * record;                             
	record = list_entry(head,Record_List,list);              
	entity_head = (EX_MODULE_HEAD *) record->record;                      
	if(entity_head == NULL)
		return -EINVAL;
	if(entity_head->uuid==NULL)
		return -EINVAL;
	return memcmp(entity_head->uuid,uuid,DIGEST_SIZE);        
}

int entity_comp_name(void * list_head, void * name) 
{                                                             
	struct list_head * head;
	EX_MODULE_HEAD * entity_head;    
	head=(struct list_head *)list_head;
	if(head==NULL)
		return -EINVAL;	
	Record_List * record;                             
	char * string;
	string=(char *)name;
	record = list_entry(head,Record_List,list);              
	entity_head = (EX_MODULE_HEAD *) record->record;                      
	if(entity_head == NULL)
		return -EINVAL;
	if(entity_head->uuid==NULL)
		return -EINVAL;
	return strncmp(entity_head->uuid,string,DIGEST_SIZE*2);        
}

int ex_module_list_init()
{
	int ret;
	ex_module_list=kmalloc(sizeof(struct ex_module_list),GFP_KERNEL);
	if(ex_module_list==NULL)
		return -ENOMEM;
	INIT_LIST_HEAD(&(ex_module_list->head.list));
	ex_module_list->head.record=NULL;
	ex_module_list->curr=&(ex_module_list->head.list);
	return 0;
}

int find_ex_module(char * name,void ** ex_mod)
{
	struct list_head * curr_head;
	Record_List * record_elem;
	Record_List * record_list;
	record_list=&(ex_module_list->head);
	int ret;

	curr_head = find_elem_with_tag(record_list,
		entity_comp_name,name);
	if(curr_head == NULL)
	{
		return 0;
	}
	if(IS_ERR(curr_head))
	{
		return curr_head;
	}
	record_elem=list_entry(curr_head,Record_List,list);
	*ex_mod=record_elem->record;
	return 1;	
}

int get_first_ex_module(void **ex_mod)
{
	Record_List * recordhead;
	Record_List * newrecord;
	struct list_head * curr_head;

	recordhead = &(ex_module_list->head);
	if(recordhead==NULL)
	{
		*ex_mod=NULL;
		return 0;
	}
	curr_head = recordhead->list.next;
	ex_module_list->curr = curr_head;
	newrecord = list_entry(curr_head,Record_List,list);
	*ex_mod=newrecord->record;
	return 1;
}

int get_next_ex_module(void **ex_mod)
{
	Record_List * recordhead;
	Record_List * newrecord;
	struct list_head * curr_head;

	recordhead = &(ex_module_list->head);
	if(recordhead==NULL)
	{
		*ex_mod=NULL;
		return 0;
	}
	curr_head = ex_module_list->curr->next;
	if(curr_head==recordhead)
	{
		*ex_mod=NULL;
		return 0;
	}
	ex_module_list->curr = curr_head;
	newrecord = list_entry(curr_head,Record_List,list);
	*ex_mod=newrecord->record;
	return 1;
}

int add_ex_module(void * ex_module)
{
	Record_List * recordhead;
	Record_List * newrecord;

	recordhead = &(ex_module_list->head);
	if(recordhead==NULL)
		return -ENOMEM;

	newrecord = kmalloc(sizeof(Record_List),GFP_KERNEL);
	if(newrecord==NULL)
		return -ENOMEM;
	INIT_LIST_HEAD(&(newrecord->list));
	newrecord->record=ex_module;
	list_add_tail(&(newrecord->list),recordhead);
	return 0;
}	

int remove_ex_module(char * name,void **ex_mod)
{
	Record_List * recordhead;
	Record_List * record_elem;
	struct list_head * curr_head;
	void * record;

	recordhead = &(ex_module_list->head);
	if(recordhead==NULL)
		return 0;

	curr_head=find_elem_with_tag(recordhead,entity_comp_uuid,name);
	if(curr_head==NULL)
	{
		return 0;
	}
	record_elem=list_entry(curr_head,Record_List,list);
	list_del(curr_head);
	record=record_elem->record;
	kfree(record_elem);
        *ex_mod=record;	
	return 1;
}	

 
int ex_module_create(char * name,int type,struct struct_elem_attr *  context_desc, void ** ex_mod)
{
	int ret;
	EX_MODULE * ex_module;
	if(name==NULL)
		return -EINVAL;


	// alloc mem for ex_module
	ex_module=kmalloc(sizeof(EX_MODULE),GFP_KERNEL);
	if(ex_module==NULL)
		return -ENOMEM;
	memset(ex_module,0,sizeof(EX_MODULE));

	// assign some  value for ex_module
	strncpy(ex_module->head.name,name,DIGEST_SIZE*2);

	// init the proc's mutex and the cond
	if(ret!=0)
	{
		kfree(ex_module);
		return -EINVAL;
	}
	if(ret!=0)
	{
		kfree(ex_module);
		return -EINVAL;
	}

	ex_module->context_template=create_struct_template(context_desc);
	if(ex_module->context_template==NULL)
	{
		kfree(ex_module);
		return -EINVAL;
	}

	*ex_mod=ex_module;		

	pthread_attr_init(&(ex_module->thread_attr));
	ex_module->init=NULL;
	ex_module->start=NULL;

	return 0;
}

int ex_module_setinitfunc(void * ex_mod,void * init)
{
	int ret;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
		return -EINVAL;
	ex_module = (EX_MODULE *)ex_mod;
	ex_module->init=init;
	return 0;
}



int ex_module_setstartfunc(void * ex_mod,void * start)
{
	int ret;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
		return -EINVAL;
	ex_module = (EX_MODULE *)ex_mod;
	ex_module->start=start;
	return 0;
}

void * ex_module_getname(void * ex_mod)
{
	int ret;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
		return NULL;
	ex_module = (EX_MODULE *)ex_mod;

	return &ex_module->head.name;
}

int ex_module_getcontext(void * ex_mod,void ** context)
{
	int ret;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
		return -EINVAL;
	ex_module = (EX_MODULE *)ex_mod;
	*context=ex_module->context;
	return 0;
}

int _ex_module_passpara(void * pointer)
{
	struct subject_para_struct
	{
		EX_MODULE * ex_module;
		void * para;
		int (*start)(void *,void *);
	};
	
	struct subject_para_struct * trans_pointer=pointer;
	
	if((trans_pointer==NULL) ||IS_ERR(trans_pointer))
	pthread_exit((void *)-EINVAL);
	trans_pointer->ex_module->retval=trans_pointer->start(trans_pointer->ex_module,trans_pointer->para);
	pthread_exit((void *)&(trans_pointer->ex_module->retval));

}

int ex_module_init(void * ex_mod,void * para)
{
	int ret=0;
	EX_MODULE * ex_module=(EX_MODULE *)ex_mod;
	if(ex_mod==NULL)
	{
		return -EINVAL;
	}

	// judge if the ex_module's state is right
	if(ex_module->init ==NULL)
	{
		return ret;
	}
	ret=ex_module->init(ex_mod,para);

	return ret;
}

int ex_module_start(void * ex_mod,void * para)
{
	struct subject_para_struct
	{
		EX_MODULE * ex_module;
		void * para;
		int (*start)(void *,void *);
	};
	
	struct subject_para_struct * trans_pointer;

	int ret;
	
	EX_MODULE * ex_module=(EX_MODULE *)ex_mod;
	if(ex_mod==NULL)
		return -EINVAL;
	if(ex_module->start==NULL)
		return -EINVAL;

	trans_pointer=kmalloc(sizeof(struct subject_para_struct),GFP_KERNEL);
	if(trans_pointer==NULL)
	{
		kfree(trans_pointer);
		return -ENOMEM;
	}


	ex_module = (EX_MODULE *)ex_mod;
	trans_pointer->ex_module=ex_mod;
	trans_pointer->para=para;
	trans_pointer->start=ex_module->start;
	
	ret=pthread_create(&(ex_module->proc_thread),NULL,_ex_module_passpara,trans_pointer);
	return ret;

}

int ex_module_join(void * ex_mod,int * retval)
{
	int ret;
	int * thread_return;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
	{
		return -EINVAL;
	}
	ex_module = (EX_MODULE *)ex_mod;
	ret=pthread_join(ex_module->proc_thread,&thread_return);
	ex_module->retval=*thread_return;
	*retval=*thread_return;
	
	return ret;
}

int ex_module_proc_getpara(void * arg,void ** ex_mod,void ** para)
{
	struct subject_para_struct
	{
		void * ex_module;
		void * para;
	};
	
	if((arg==NULL) || IS_ERR(arg))
		return -EINVAL;
	printf("subject getpara!,arg=%x\n",arg);
	struct subject_para_struct * trans_pointer=(struct subject_para_struct * )arg;
	
	printf("ex_module =%x\n",trans_pointer->ex_module);
	if((trans_pointer->ex_module==NULL)||IS_ERR(trans_pointer->ex_module))
	{
		printf("sec subject get para err!\n");
		return -EINVAL;
	}

	*ex_mod=trans_pointer->ex_module;
	*para=trans_pointer->para;
	kfree(trans_pointer);
	return 0;	
}

void ex_module_destroy(void * ex_mod)
{
	int ret;
	EX_MODULE * ex_module;
	if(ex_mod==NULL)
		return ;
	ex_module = (EX_MODULE *)ex_mod;
	kfree(ex_module);
	return;
}
