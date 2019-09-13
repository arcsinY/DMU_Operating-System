#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <queue>
using namespace std;

#define PROCESS_NUM 3   //3������
#define FRAME_NUM 64    //����ҳ����
#define RSS_SIZE 3    //Ĭ��פ������С
#define FRAME_SIZE 16   //���С   
#define RESIDENT_MASK 1   //פ��λ��1
#define REVISE_MASK 2    //�޸�λ��1
#define ACCESS_MASK 4   //����λ��1
#define PAGE_ITEM_SIZE 4   //ҳ�����С
#define USE_CLOCK

typedef struct {
	int page_no;   //ҳ��
	int frame_no = -1;   //������
	int disk_address;    //���ʼַ
	int mark;    //������־λ���ӵ͵���Ϊפ��λ���޸�λ������λ
}page_table_item;

struct log{
	int access_pageno;
	int absence;
	int put_out_page;
	int cur_mem_page[RSS_SIZE];
};

typedef struct{
	int page_table_address;   //ҳ��ʼַ
	int lenth;   //ҳ����(���̵�ҳ��)
	int rss_size;   //פ������С
	int absence;   //ȱҳ����
	queue<int> q;  //ҳ����У�����FIFO
	int time[4];   //ÿ��ҳδ�����ʵ�ʱ�䡣����LRU������һҳ�����ڴ��У�time = -1
	int access_order = 0;
	struct log access_log[100];
}process;    //ÿ�����̼�¼����Ϣ

typedef struct {
	int data[FRAME_SIZE];
}page;     //ҳ�棬16�ֽڵ�����

struct Node
{
	int page_no;
	int process;
	int frame_no;
	bool flag = false;
}cache[4];



int memory[1024];   //1024�ֽڵ��ڴ�
unsigned long long bitmap = 364134;   //λʾͼ��ÿһ��bit����һ���顣�����趨һ����ʼֵ�������ڴ����ڵ�ռ�����
process process_info[PROCESS_NUM];
int residue_frame = FRAME_NUM;   //ʣ��ҳ����
int access_cnt = 0;     //�����ڴ��ܴ���



int cache_pointer = 0;