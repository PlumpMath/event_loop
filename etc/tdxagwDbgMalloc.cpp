#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////Protected&Debugging Malloc/Fee/////////////
extern "C" {
//92,12,26
#define DBG_WARN_ALLOC 		0
#define DBG_INFO_ALLOC 			0
#define DBG_INFO_POLLALLOC 	1
#define TMALLOC_RET_NULL() do{printf("tmalloc:ret NULL\n");return NULL;}while(0)
#define TCALLOC_RET_NULL() do{printf("tcalloc:ret NULL\n");return NULL;}while(0)
#define TREALLOC_RET_NULL() do{printf("trealloc:ret NULL\n");return NULL;}while(0)
#define TFREE_RET_NULL() do{if(DBG_WARN_ALLOC)printf("tfree:InputNULL\n");return;}while(0)

#if 1
#define TALLOC_GET_ADJUSTEDSIZE(size) 	((((size)+1023)/1024)*1024)
#define TALLOC_GET_REALBASE(ptr) ((unsigned int*)ptr-3)
#define TALLOC_GET_RETBASE(ptr0) ((unsigned int*)ptr0+3)
#define TALLOC_GET_REALSIZE(size) (TALLOC_GET_ADJUSTEDSIZE(size)+3*sizeof(unsigned int))
#define TALLOC_SET_SIZE(ptr0,size) do{*((unsigned int*)ptr0)=(size);*((unsigned int*)ptr0+1)=TALLOC_GET_ADJUSTEDSIZE(size);}while(0)
#define TALLOC_GET_ALLOCSIZE(ptr0) *((unsigned int*)ptr0+1)
#define TALLOC_GET_SIZE(ptr0) *((unsigned int*)ptr0)
#define TALLOC_GET_SIZE_DIRECT(ptr) TALLOC_GET_SIZE(TALLOC_GET_REALBASE(ptr))
#define TALLOC_GET_ALLOCSIZE_DIRECT(ptr) TALLOC_GET_ALLOCSIZE(TALLOC_GET_REALBASE(ptr))
#else
#define TALLOC_GET_ADJUSTEDSIZE(size) (size)
#define TALLOC_GET_REALBASE(ptr) ((unsigned int*)ptr-1)
#define TALLOC_GET_RETBASE(ptr0) ((unsigned int*)ptr0+1)
#define TALLOC_GET_REALSIZE(size) ((size)+1*sizeof(unsigned int))
#define TALLOC_SET_SIZE(ptr0,size) *((unsigned int*)ptr0)=(size)
#define TALLOC_GET_ALLOCSIZE(ptr0) *((unsigned int*)ptr0)
#define TALLOC_GET_SIZE(ptr0) *((unsigned int*)ptr0)
#define TALLOC_GET_SIZE_DIRECT(ptr) TALLOC_GET_SIZE(TALLOC_GET_REALBASE(ptr))
#define TALLOC_GET_ALLOCSIZE_DIRECT(ptr) TALLOC_GET_SIZE_DIRECT(ptr)
#endif
void* tmalloc(size_t size)
{
	unsigned int* ptr0=(unsigned int*)malloc(TALLOC_GET_REALSIZE(size));

	if(!ptr0)TMALLOC_RET_NULL();
	// no clear
	TALLOC_SET_SIZE(ptr0,size);
	return TALLOC_GET_RETBASE(ptr0);
}
void tfree(void* ptr)
{
	if(!ptr)TFREE_RET_NULL();// many
	{
		unsigned int* ptr0=TALLOC_GET_REALBASE(ptr);

		free(ptr0);
	}
}
void* tcalloc(size_t size)
{
	void* ptr=tmalloc(size);

	if(!ptr)TCALLOC_RET_NULL();
	if(ptr)memset(ptr,0,size);// clear
	return ptr;
}
void* trealloc(void* old_ptr,size_t new_size)
{
	if(!old_ptr){
		if(DBG_WARN_ALLOC)printf("trealloc:InputNULL\n");// exist
		return tmalloc(new_size);
	}
#if 1	
	else if(TALLOC_GET_SIZE_DIRECT(old_ptr)>new_size)return old_ptr;//dangerous
#endif	
	else{
		size_t old_size=(size_t) *(TALLOC_GET_REALBASE(old_ptr));
		void* new_ptr=tmalloc(new_size);

		if(!new_ptr){tfree(old_ptr);TREALLOC_RET_NULL();}		
		memcpy(new_ptr,old_ptr,old_size);//copy!!
		tfree(old_ptr);
		return new_ptr;
	}
}

int g_initOneAlloc=1;
#define is_pool_size(size)	(size==92||size==12||size==26)
#define get_pool_num(size)  (size==92? 0:(size==12? 1:size==26? 2:-1))
void* pool_bufs[3][1024];
int pool_uses[3][1024];
int pool_idxs[3];
void* tpool_alloc(size_t size)
{
	int poolId=get_pool_num(size);
	int poolIdx;int j;

	if(poolId<0||g_initOneAlloc){return tmalloc(size);}
	poolIdx=pool_idxs[poolId];pool_idxs[poolId]=(pool_idxs[poolId]+1)%1024;
	for(j=0;j<1024;j++)
		if(!pool_uses[poolId][(poolIdx+j)%1024]){
			pool_uses[poolId][(poolIdx+j)%1024]=1;
			pool_idxs[poolId]=(poolIdx+j+1)%1024;
			if(DBG_INFO_POLLALLOC)printf("ret pool%d/%d\n",poolId,(poolIdx+j)%1024);
			return pool_bufs[poolId][(poolIdx+j)%1024];
		}
       printf("WARN poolIdx%d FULL alloced!\n",poolIdx);		
	return tmalloc(size);
}
void tpool_free(void* ptr)
{
	int i,j;

	if(g_initOneAlloc){tfree(ptr);return;}
	for(i=0;i<3;i++)
		for(j=0;j<1024;j++)
			if(ptr==pool_bufs[i][j]){if(DBG_INFO_POLLALLOC)printf("free pool%d/%d\n",i,j);pool_uses[i][j]=0;return;}//SLOW!!
	tfree(ptr);
}
void* tpool_calloc(size_t size)
{
	void* ptr=tpool_alloc(size);

	if(!ptr)TCALLOC_RET_NULL();
	if(ptr)memset(ptr,0,size);// clear
	return ptr;
}
void* tpool_realloc(void* old_ptr,size_t new_size)
{
	if(!old_ptr){
		if(DBG_WARN_ALLOC)printf("tpool_realloc:InputNULL\n");// exist
		return tpool_alloc(new_size);
	}
	else{
		size_t old_size=(size_t) *(TALLOC_GET_REALBASE(old_ptr));
		void* new_ptr=tpool_alloc(new_size);

		if(!new_ptr){tpool_free(old_ptr);TREALLOC_RET_NULL();}		
		memcpy(new_ptr,old_ptr,old_size);//copy!!
		tpool_free(old_ptr);
		return new_ptr;
	}
}

void tinit_pool(void)
{
	int j;
	
	for(j=0;j<1024;j++){pool_bufs[0][j]=tmalloc(92);pool_uses[0][j]=0;}
	for(j=0;j<1024;j++){pool_bufs[1][j]=tmalloc(12);pool_uses[1][j]=0;}
	for(j=0;j<1024;j++){pool_bufs[2][j]=tmalloc(25);pool_uses[2][j]=0;}
}


void* my_malloc(size_t size)
{
if(DBG_INFO_ALLOC)printf("size:%d\n",size);
	if(g_initOneAlloc){tinit_pool();g_initOneAlloc=0;}
	return tpool_alloc(size);
	// return tmalloc(size);
}
void my_free(void* ptr)
{
	if(g_initOneAlloc){tinit_pool();g_initOneAlloc=0;}
	tpool_free(ptr);
	// tfree(ptr);
}
void* my_realloc(void* old_ptr,size_t new_size)
{
if(DBG_INFO_ALLOC)printf("re-size:%d\n",new_size);
	if(g_initOneAlloc){tinit_pool();g_initOneAlloc=0;}
	return tpool_realloc(old_ptr,new_size);
	// return trealloc(old_ptr,new_size);
}
void* my_calloc(size_t nmemb,size_t size)
{
if(DBG_INFO_ALLOC)printf("c-size:%d\n",size*nmemb);
	if(g_initOneAlloc){tinit_pool();g_initOneAlloc=0;}
	return tpool_calloc(nmemb*size);
	// return tcalloc(nmemb*size);
}
} /*extern "C" */
////////////////////////////////////////////////////////////////////////
