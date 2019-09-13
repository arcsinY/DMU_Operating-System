#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <queue>
using namespace std;

#define PROCESS_NUM 3   //3个进程
#define FRAME_NUM 64    //物理页个数
#define RSS_SIZE 3    //默认驻留集大小
#define FRAME_SIZE 16   //块大小   
#define RESIDENT_MASK 1   //驻留位置1
#define REVISE_MASK 2    //修改位置1
#define ACCESS_MASK 4   //访问位置1
#define PAGE_ITEM_SIZE 4   //页表项大小
#define USE_CLOCK

typedef struct {
	int page_no;   //页号
	int frame_no = -1;   //物理块号
	int disk_address;    //外存始址
	int mark;    //三个标志位，从低到高为驻留位，修改位、访问位
}page_table_item;

struct log{
	int access_pageno;
	int absence;
	int put_out_page;
	int cur_mem_page[RSS_SIZE];
};

typedef struct{
	int page_table_address;   //页表始址
	int lenth;   //页表长度(进程的页数)
	int rss_size;   //驻留集大小
	int absence;   //缺页次数
	queue<int> q;  //页面队列，用于FIFO
	int time[4];   //每个页未被访问的时间。用于LRU。若这一页不在内存中，time = -1
	int access_order = 0;
	struct log access_log[100];
}process;    //每个进程记录的信息

typedef struct {
	int data[FRAME_SIZE];
}page;     //页面，16字节的数据

struct Node
{
	int page_no;
	int process;
	int frame_no;
	bool flag = false;
}cache[4];



int memory[1024];   //1024字节的内存
unsigned long long bitmap = 364134;   //位示图，每一个bit代表一个块。任意设定一个初始值，代表内存现在的占用情况
process process_info[PROCESS_NUM];
int residue_frame = FRAME_NUM;   //剩余页面数
int access_cnt = 0;     //访问内存总次数



int cache_pointer = 0;