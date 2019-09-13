#include "header.h"

void init(char* path);
int put_into_mem(page_table_item* page_table, int size);   //返回页表物理地址
int find_empty_frame();   //找到一个空闲物理页，返回物理块号
void access_mem(char* path);
int address_translation(int process_no, int logic_address);     //根据逻辑地址返回物理地址，缺页返回-1
int lookup_page_table(int process_no, int logic_address);      //根据进程号+逻辑地址查页表，返回这一页表项的地址
int put_page_into_mem(int disk_address);    //页换入内存,返回物理块号
void put_page_out_mem(int process_no, int page_no);    //页面换出，返回这个页面的物理块号
int LRU(int process_no, int mem_address);   //返回要换出的页号
int Clock(int process_no);
int FIFO(int process_no, int page_no);    //参数：进程号、访问的页号
int lookup_cache(int process_no, int page_no);   //查找快表，参数为进程号和页号，返回物理块号
void show();
int lookup_cache(int process_no, int page_no);   //返回物理块号，不成功返回-1
void put_page_into_cache(int process_no, int page_no);

int main()
{
	init("process.txt");
	printf("页表建立完成\n");
	printf("进程号\t页表物理块号\t页表物理地址\n");
	for (int i = 0; i < PROCESS_NUM; ++i)
	{
		printf("%d\t\t%d\t\t%d", i, process_info[i].page_table_address/FRAME_SIZE, process_info[i].page_table_address);
		printf("\n");
	}
	for (int i = 0; i < PROCESS_NUM; ++i)
	{
		printf("进程%d的页表：\n", i);
		printf("页号 物理块号 外存地址 驻留位 修改位 访问位\n");
		for (int j = 0; j < process_info[i].lenth; ++j)
		{
			printf("%d\t%d\t%d\t%d\t%d\t%d\n", j, memory[process_info[i].page_table_address+j*PAGE_ITEM_SIZE+1], \
				    memory[process_info[i].page_table_address + j*PAGE_ITEM_SIZE + 2], memory[process_info[i].page_table_address + j*PAGE_ITEM_SIZE + 3]&RESIDENT_MASK\
			, memory[process_info[i].page_table_address + j*PAGE_ITEM_SIZE + 3]&REVISE_MASK, memory[process_info[i].page_table_address + j*PAGE_ITEM_SIZE + 3]&ACCESS_MASK);
		}
	}
	printf("\n");
	access_mem("access.txt");
	int sum = 0;
	for (int i = 0; i < PROCESS_NUM; ++i)
		sum += process_info[i].absence;
	printf("共缺页%d次,缺页率%.2f\n", sum, (float)sum/access_cnt);
	show();
	system("pause");
	return 0;
}

void init(char* path)
{
	FILE* f = fopen(path, "r");
	int index = 0;
	for (int i = 0; i < PROCESS_NUM; ++i)
	{
		int no, size, address;
		fscanf(f, "%d %d %d", &no, &size, &address);  //读取本进程的信息
		page_table_item* page_table = (page_table_item*)malloc(sizeof(page_table_item)*size);   //建立页表
		for (int j = 0; j < size; ++j)   //这个进程的页表有size个项目,size<=4
		{
			page_table[j].page_no = j;   //页号
			page_table[j].mark = 0;
			page_table[j].disk_address = address + j * FRAME_SIZE;   //外存地址
			page_table[j].frame_no = -1;   //外存地址

		}
		process_info[i].page_table_address = put_into_mem(page_table, size);   //这个进程的页表地址
		process_info[i].lenth = size;
		process_info[i].rss_size = RSS_SIZE;   
		process_info[i].absence = 0;
		for (int j = 0; j < size; ++j)
			process_info[i].time[j] = -1;
	}
	fclose(f);
}

int put_into_mem(page_table_item* page_table, int size)
{
	int pos = find_empty_frame();   //页表所在的物理块号
	int index = pos * FRAME_SIZE;    //块号*块大小 = 地址
	for (int i = 0; i < size; ++i)   //每个页表项放入内存
	{
		memory[index++] = page_table[i].page_no;
		memory[index++] = page_table[i].frame_no;
		memory[index++] = page_table[i].disk_address;
		memory[index++] = page_table[i].mark;
	}
	return pos * FRAME_SIZE;
}

int find_empty_frame()
{
	int no = 0;
	unsigned long long mask = 1;
	while ((bitmap & mask) != 0)   //计算bitmap的第几位是0
	{
		++no;
		mask <<= 1;
	}
	bitmap |= (unsigned long long)pow(2,no);   //这个页面占用，修改bitmap
	return no;
}

void access_mem(char* path)
{
	int no; char op; int logic_address;
	int mem_address;   //物理地址
	FILE* f = fopen(path, "r+");
	while (fscanf(f,"%d %c %d", &no, &op, &logic_address) != EOF)  //读入一个访存请求
	{
		++access_cnt;
		int page_no = logic_address / FRAME_SIZE;   //计算要访问的页号
		int order = process_info[no].access_order++;
		process_info[no].access_log[order].access_pageno = page_no;
		mem_address = address_translation(no, logic_address);   //物理地址
		if (mem_address != -1)    //不缺页
		{
			process_info[no].access_log[order].absence = 0;
			process_info[no].access_log[order].put_out_page = -1;
			process_info[no].access_log[order].cur_mem_page[0] = process_info[no].q.front();
			process_info[no].access_log[order].cur_mem_page[1] = process_info[no].q.back();
			printf("访问成功，数据为%d\n\n\n", memory[mem_address]);
			process_info[no].time[page_no] = 0;
			continue;
		}
		else   //缺页
		{
			process_info[no].access_log[order].absence = 1;
			printf("发生缺页中断\n");
			process_info[no].absence++;
			if (process_info[no].rss_size > 0)   //不需要页面置换
			{
				process_info[no].access_log[order].put_out_page = -1;
				--process_info[no].rss_size;
				int page_item_address = lookup_page_table(no, logic_address);
				int disk_address = memory[page_item_address + 2];    //获得外存地址
				int mem_no = put_page_into_mem(disk_address);
				memory[page_item_address + 1] = mem_no;    //填写页表
				memory[page_item_address + 3] |= RESIDENT_MASK;   //驻留位置1
				memory[page_item_address + 3] |= ACCESS_MASK;   //访问位置1
				process_info[no].q.push(memory[page_item_address]);   //进程的页号入队列
				process_info[no].access_log[order].cur_mem_page[0] = process_info[no].q.front();
				process_info[no].access_log[order].cur_mem_page[1] = process_info[no].q.back();
				if(op == 'w')
					memory[page_item_address + 3] |= REVISE_MASK;   //修改位置1
				printf("进程%d的第%d页换入内存，占用内存块%d\n", no, page_no, mem_no);
				process_info[no].time[page_no] = 0;
			}
			else   //页面置换
			{
				int out_no;   //换出的页号
				
#if defined(USE_LRU)
				out_no = LRU(no, mem_address);
#elif defined(USE_CLOCK)
				out_no = Clock(no);
#else 
				out_no = FIFO(no, page_no);
#endif
				process_info[no].access_log[order].put_out_page = out_no;
				printf("页面置换，进程%d的第%d页换出\n", no, out_no);
				put_page_out_mem(no, out_no);
				int page_item_address = lookup_page_table(no, logic_address);
				int disk_address = memory[page_item_address + 2];    //获得外存地址
				int mem_no = put_page_into_mem(disk_address);
				memory[page_item_address + 1] = mem_no;    //填写页表
				memory[page_item_address + 3] |= RESIDENT_MASK;   //驻留位置1
				memory[page_item_address + 3] |= ACCESS_MASK;   //访问位置1
				process_info[no].q.push(memory[page_item_address]);
				process_info[no].access_log[order].cur_mem_page[0] = process_info[no].q.front();
				process_info[no].access_log[order].cur_mem_page[1] = process_info[no].q.back();
				if (op == 'w')
					memory[page_item_address + 3] |= REVISE_MASK;   //修改位置1
				printf("进程%d的第%d页换入内存，占用内存块%d\n", no, logic_address / FRAME_SIZE, mem_no);
			}
			put_page_into_cache(no, logic_address / FRAME_SIZE);
		}
		printf("\n\n");
		getchar();
	}
	fclose(f);
}

int address_translation(int process_no, int logic_address)
{
	int offset = logic_address % FRAME_SIZE;    //页内偏移量
	//查找快表
	int frame_no = lookup_cache(process_no, logic_address / FRAME_SIZE);
	if (frame_no > 0)   //成功找到
	{
		printf("查找快表成功\n");
		return frame_no*FRAME_SIZE + offset;
	}
	int page_item_address = lookup_page_table(process_no, logic_address);   //页表项地址
	int mark = memory[page_item_address + 3];   //这一项的标志位
	if ((mark & RESIDENT_MASK) == 0)   //驻留位为0，缺页
		return -1;
	return memory[page_item_address + 1] * FRAME_SIZE + offset;    //物理地址
}


int lookup_page_table(int process_no, int logic_address)
{
	int page_no = logic_address / FRAME_SIZE;   //页号
	return process_info[process_no].page_table_address + page_no * PAGE_ITEM_SIZE;
}


int put_page_into_mem(int disk_address)
{
	page p;
	FILE* f = fopen("data.txt", "r");
	fseek(f, disk_address, 0);
	for (int i = 0; i < FRAME_SIZE; ++i)
		fscanf(f, "%1d", &p.data[i]);
	int mem_no = find_empty_frame();   //找到物理块
	int mem_address = mem_no * FRAME_SIZE;  
	int index = mem_address;
	for (int i = 0; i < FRAME_SIZE; ++i)    //数据放入内存
		memory[index++] = p.data[i];
	fclose(f);
	return mem_no;
}

void put_page_out_mem(int process_no, int page_no)
{
	int page_item_address = process_info[process_no].page_table_address + page_no * PAGE_ITEM_SIZE;   //页表项地址
	memory[page_item_address + 3] &= (~RESIDENT_MASK);   //驻留位置0
	if ((memory[page_item_address + 3] & REVISE_MASK) == 1)   //页面有过修改，写回外存
	{
		FILE* f = fopen("data.txt", "r+");
		int disk_address = memory[page_item_address + 2];
		int mem_address = memory[page_item_address + 1] * FRAME_SIZE;  //物理地址
		fseek(f, disk_address, 0);
		for (int i = 0; i < FRAME_SIZE;++i)    //写出外存
			fprintf(f, "%1d", memory[mem_address++]);
		fclose(f);
	}
	int frame_no = memory[page_item_address + 1];
	bitmap -= (unsigned long long)pow(2, frame_no);    //位示图对应位置0
}

int FIFO(int process_no, int page_no)
{
	int no = process_info[process_no].q.front();
	process_info[process_no].q.pop();
	return no;
}

int LRU(int process_no, int mem_address)
{

	//LRU算法的代码
	static const int NUM = 4;
	static int i, j, k, t, p, flag;
	page_table_item page_no[NUM];//记录页面号
	process pagesno;//记录页未被使用时的时间
	pagesno.absence = 0;  //记录缺页次数
	p = 0;
	//printf("----------------------------------------------------\n");
	//printf("最近最少使用调度算法（LRU）页面调出流:");
	for (i = 0; i < NUM; i++)  //前4个进入内存的页面
	{
		page_no[p].page_no = memory[i];
		//printf("%d ", page_no[p].page_no);
		for (j = 0; j <= p; j++) {
			pagesno.time[j]++;
		}
		p = (p + 1) % NUM;
	}
	//printf("\n");
	pagesno.absence = 4;
	for (i = NUM; i < mem_address; i++)
	{
		flag = 0;
		for (j = 0; j < NUM; j++)  //判断当前需求的页面是否在内存
		{
			pagesno.time[j]++;
			if (page_no[j].page_no == memory[i])
			{
				flag = 1;
				pagesno.time[j] = 0;
			}
		}
		if (flag == 0)
		{
			int max = 0;
			for (k = 1; k < NUM; k++)
			{
				if (pagesno.time[max] < pagesno.time[k])
					max = k;
			}
		//	printf("%d ", page_no[max].page_no);
			page_no[max].page_no = memory[i];
			pagesno.time[max] = 0;
			pagesno.absence++;
		}
	}
	//printf("总缺页数:%d", &pagesno[i].absence);
	process_no = pagesno.absence;
	return process_no;
}


int Clock(int process_no)
{
	bool flag = true;
	int pointer = 0;//内存中查找起始页号
	int adress = process_info[process_no].page_table_address;
	for (int j = pointer; j<process_info[process_no].lenth; j++)
	{
		if ((memory[adress + 3] & ACCESS_MASK) == 0 && (memory[adress + 3] & REVISE_MASK) == 0)
		{
			pointer = j;   //记录找到的页号
			flag = false;
			break;//跳出循环
		}
		adress += PAGE_ITEM_SIZE;
	}
	if (flag == false)
		return pointer;//返回页号
	adress = process_info[process_no].page_table_address;//未找到，第二步查询
	for (int j = pointer; j<process_info[process_no].lenth; j++)
	{
		memory[adress + 3] &= (~ACCESS_MASK);    //访问位置0
		if ((memory[adress + 3] & ACCESS_MASK) == 0 && (memory[adress + 3] & REVISE_MASK) == 1)
		{
			pointer = j;//记录找到的页号
			flag = false;
			break;
		}
		adress += PAGE_ITEM_SIZE;
	}
	if (flag == false)
		return pointer;//返回
	else
		return Clock(process_no);//递归查询
}


void show() {
	printf("\n\n*******访问日志**************\n");
	char ch[2] = { 'n','y' };
	for (int i = 0; i<PROCESS_NUM; i++) {
		//第i个进程
		printf("第%d个进程访问日志：\n", i);
		printf("访问页号\t缺页否\t\t换出页号\t第1块\t\t第2块\n");
		for (int j = 0; j<process_info[i].access_order; j++)
		{
			printf("%d\t\t%c\t\t%d\t\t%d\t\t%d\n", process_info[i].access_log[j].access_pageno, \
				ch[process_info[i].access_log[j].absence], \
				process_info[i].access_log[j].put_out_page, \
				process_info[i].access_log[j].cur_mem_page[0], \
				process_info[i].access_log[j].cur_mem_page[1]);

		}
		printf("第%d个进程访问次数：%d 缺页次数：%d\n", i, process_info[i].access_order, \
			process_info[i].absence);
		printf("\n\n");
	}
	printf("******************************\n");
}


int lookup_cache(int process_no, int page_no)
{
	int ans = 0;
	for (int i = 0; i<4; i++)
	{
		if (cache[i].process == process_no && cache[i].page_no == page_no)
			return cache[i].frame_no;
	}
	return -1;
}


void put_page_into_cache(int process_no, int page_no)
{
	int ans = 0;
	for (int i = 0; i<4; i++)
	{
		if (cache[i].flag == true)
		{
			cache[i].page_no = page_no;
			cache[i].process = process_no;
			cache[i].frame_no = memory[process_info[process_no].page_table_address + page_no * PAGE_ITEM_SIZE + 1];   //赋值物理块号
			//return cache[i].frame_no;
		}
	}
	ans = (cache_pointer % 4);   //取余，找到最先进入cache的页号，替换出去
	cache_pointer++;            //指针加1
	cache[ans].page_no = page_no;
	cache[ans].process = process_no;
	cache[ans].flag = true;
	cache[ans].frame_no = memory[process_info[process_no].page_table_address + page_no * PAGE_ITEM_SIZE + 1];   //赋值物理块号
	//return cache[ans].frame_no;
}