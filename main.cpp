#include "header.h"

void init(char* path);
int put_into_mem(page_table_item* page_table, int size);   //����ҳ�������ַ
int find_empty_frame();   //�ҵ�һ����������ҳ������������
void access_mem(char* path);
int address_translation(int process_no, int logic_address);     //�����߼���ַ���������ַ��ȱҳ����-1
int lookup_page_table(int process_no, int logic_address);      //���ݽ��̺�+�߼���ַ��ҳ��������һҳ����ĵ�ַ
int put_page_into_mem(int disk_address);    //ҳ�����ڴ�,����������
void put_page_out_mem(int process_no, int page_no);    //ҳ�滻�����������ҳ���������
int LRU(int process_no, int mem_address);   //����Ҫ������ҳ��
int Clock(int process_no);
int FIFO(int process_no, int page_no);    //���������̺š����ʵ�ҳ��
int lookup_cache(int process_no, int page_no);   //���ҿ������Ϊ���̺ź�ҳ�ţ�����������
void show();
int lookup_cache(int process_no, int page_no);   //���������ţ����ɹ�����-1
void put_page_into_cache(int process_no, int page_no);

int main()
{
	init("process.txt");
	printf("ҳ�������\n");
	printf("���̺�\tҳ��������\tҳ�������ַ\n");
	for (int i = 0; i < PROCESS_NUM; ++i)
	{
		printf("%d\t\t%d\t\t%d", i, process_info[i].page_table_address/FRAME_SIZE, process_info[i].page_table_address);
		printf("\n");
	}
	for (int i = 0; i < PROCESS_NUM; ++i)
	{
		printf("����%d��ҳ��\n", i);
		printf("ҳ�� ������ ����ַ פ��λ �޸�λ ����λ\n");
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
	printf("��ȱҳ%d��,ȱҳ��%.2f\n", sum, (float)sum/access_cnt);
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
		fscanf(f, "%d %d %d", &no, &size, &address);  //��ȡ�����̵���Ϣ
		page_table_item* page_table = (page_table_item*)malloc(sizeof(page_table_item)*size);   //����ҳ��
		for (int j = 0; j < size; ++j)   //������̵�ҳ����size����Ŀ,size<=4
		{
			page_table[j].page_no = j;   //ҳ��
			page_table[j].mark = 0;
			page_table[j].disk_address = address + j * FRAME_SIZE;   //����ַ
			page_table[j].frame_no = -1;   //����ַ

		}
		process_info[i].page_table_address = put_into_mem(page_table, size);   //������̵�ҳ���ַ
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
	int pos = find_empty_frame();   //ҳ�����ڵ�������
	int index = pos * FRAME_SIZE;    //���*���С = ��ַ
	for (int i = 0; i < size; ++i)   //ÿ��ҳ��������ڴ�
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
	while ((bitmap & mask) != 0)   //����bitmap�ĵڼ�λ��0
	{
		++no;
		mask <<= 1;
	}
	bitmap |= (unsigned long long)pow(2,no);   //���ҳ��ռ�ã��޸�bitmap
	return no;
}

void access_mem(char* path)
{
	int no; char op; int logic_address;
	int mem_address;   //�����ַ
	FILE* f = fopen(path, "r+");
	while (fscanf(f,"%d %c %d", &no, &op, &logic_address) != EOF)  //����һ���ô�����
	{
		++access_cnt;
		int page_no = logic_address / FRAME_SIZE;   //����Ҫ���ʵ�ҳ��
		int order = process_info[no].access_order++;
		process_info[no].access_log[order].access_pageno = page_no;
		mem_address = address_translation(no, logic_address);   //�����ַ
		if (mem_address != -1)    //��ȱҳ
		{
			process_info[no].access_log[order].absence = 0;
			process_info[no].access_log[order].put_out_page = -1;
			process_info[no].access_log[order].cur_mem_page[0] = process_info[no].q.front();
			process_info[no].access_log[order].cur_mem_page[1] = process_info[no].q.back();
			printf("���ʳɹ�������Ϊ%d\n\n\n", memory[mem_address]);
			process_info[no].time[page_no] = 0;
			continue;
		}
		else   //ȱҳ
		{
			process_info[no].access_log[order].absence = 1;
			printf("����ȱҳ�ж�\n");
			process_info[no].absence++;
			if (process_info[no].rss_size > 0)   //����Ҫҳ���û�
			{
				process_info[no].access_log[order].put_out_page = -1;
				--process_info[no].rss_size;
				int page_item_address = lookup_page_table(no, logic_address);
				int disk_address = memory[page_item_address + 2];    //�������ַ
				int mem_no = put_page_into_mem(disk_address);
				memory[page_item_address + 1] = mem_no;    //��дҳ��
				memory[page_item_address + 3] |= RESIDENT_MASK;   //פ��λ��1
				memory[page_item_address + 3] |= ACCESS_MASK;   //����λ��1
				process_info[no].q.push(memory[page_item_address]);   //���̵�ҳ�������
				process_info[no].access_log[order].cur_mem_page[0] = process_info[no].q.front();
				process_info[no].access_log[order].cur_mem_page[1] = process_info[no].q.back();
				if(op == 'w')
					memory[page_item_address + 3] |= REVISE_MASK;   //�޸�λ��1
				printf("����%d�ĵ�%dҳ�����ڴ棬ռ���ڴ��%d\n", no, page_no, mem_no);
				process_info[no].time[page_no] = 0;
			}
			else   //ҳ���û�
			{
				int out_no;   //������ҳ��
				
#if defined(USE_LRU)
				out_no = LRU(no, mem_address);
#elif defined(USE_CLOCK)
				out_no = Clock(no);
#else 
				out_no = FIFO(no, page_no);
#endif
				process_info[no].access_log[order].put_out_page = out_no;
				printf("ҳ���û�������%d�ĵ�%dҳ����\n", no, out_no);
				put_page_out_mem(no, out_no);
				int page_item_address = lookup_page_table(no, logic_address);
				int disk_address = memory[page_item_address + 2];    //�������ַ
				int mem_no = put_page_into_mem(disk_address);
				memory[page_item_address + 1] = mem_no;    //��дҳ��
				memory[page_item_address + 3] |= RESIDENT_MASK;   //פ��λ��1
				memory[page_item_address + 3] |= ACCESS_MASK;   //����λ��1
				process_info[no].q.push(memory[page_item_address]);
				process_info[no].access_log[order].cur_mem_page[0] = process_info[no].q.front();
				process_info[no].access_log[order].cur_mem_page[1] = process_info[no].q.back();
				if (op == 'w')
					memory[page_item_address + 3] |= REVISE_MASK;   //�޸�λ��1
				printf("����%d�ĵ�%dҳ�����ڴ棬ռ���ڴ��%d\n", no, logic_address / FRAME_SIZE, mem_no);
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
	int offset = logic_address % FRAME_SIZE;    //ҳ��ƫ����
	//���ҿ��
	int frame_no = lookup_cache(process_no, logic_address / FRAME_SIZE);
	if (frame_no > 0)   //�ɹ��ҵ�
	{
		printf("���ҿ��ɹ�\n");
		return frame_no*FRAME_SIZE + offset;
	}
	int page_item_address = lookup_page_table(process_no, logic_address);   //ҳ�����ַ
	int mark = memory[page_item_address + 3];   //��һ��ı�־λ
	if ((mark & RESIDENT_MASK) == 0)   //פ��λΪ0��ȱҳ
		return -1;
	return memory[page_item_address + 1] * FRAME_SIZE + offset;    //�����ַ
}


int lookup_page_table(int process_no, int logic_address)
{
	int page_no = logic_address / FRAME_SIZE;   //ҳ��
	return process_info[process_no].page_table_address + page_no * PAGE_ITEM_SIZE;
}


int put_page_into_mem(int disk_address)
{
	page p;
	FILE* f = fopen("data.txt", "r");
	fseek(f, disk_address, 0);
	for (int i = 0; i < FRAME_SIZE; ++i)
		fscanf(f, "%1d", &p.data[i]);
	int mem_no = find_empty_frame();   //�ҵ������
	int mem_address = mem_no * FRAME_SIZE;  
	int index = mem_address;
	for (int i = 0; i < FRAME_SIZE; ++i)    //���ݷ����ڴ�
		memory[index++] = p.data[i];
	fclose(f);
	return mem_no;
}

void put_page_out_mem(int process_no, int page_no)
{
	int page_item_address = process_info[process_no].page_table_address + page_no * PAGE_ITEM_SIZE;   //ҳ�����ַ
	memory[page_item_address + 3] &= (~RESIDENT_MASK);   //פ��λ��0
	if ((memory[page_item_address + 3] & REVISE_MASK) == 1)   //ҳ���й��޸ģ�д�����
	{
		FILE* f = fopen("data.txt", "r+");
		int disk_address = memory[page_item_address + 2];
		int mem_address = memory[page_item_address + 1] * FRAME_SIZE;  //�����ַ
		fseek(f, disk_address, 0);
		for (int i = 0; i < FRAME_SIZE;++i)    //д�����
			fprintf(f, "%1d", memory[mem_address++]);
		fclose(f);
	}
	int frame_no = memory[page_item_address + 1];
	bitmap -= (unsigned long long)pow(2, frame_no);    //λʾͼ��Ӧλ��0
}

int FIFO(int process_no, int page_no)
{
	int no = process_info[process_no].q.front();
	process_info[process_no].q.pop();
	return no;
}

int LRU(int process_no, int mem_address)
{

	//LRU�㷨�Ĵ���
	static const int NUM = 4;
	static int i, j, k, t, p, flag;
	page_table_item page_no[NUM];//��¼ҳ���
	process pagesno;//��¼ҳδ��ʹ��ʱ��ʱ��
	pagesno.absence = 0;  //��¼ȱҳ����
	p = 0;
	//printf("----------------------------------------------------\n");
	//printf("�������ʹ�õ����㷨��LRU��ҳ�������:");
	for (i = 0; i < NUM; i++)  //ǰ4�������ڴ��ҳ��
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
		for (j = 0; j < NUM; j++)  //�жϵ�ǰ�����ҳ���Ƿ����ڴ�
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
	//printf("��ȱҳ��:%d", &pagesno[i].absence);
	process_no = pagesno.absence;
	return process_no;
}


int Clock(int process_no)
{
	bool flag = true;
	int pointer = 0;//�ڴ��в�����ʼҳ��
	int adress = process_info[process_no].page_table_address;
	for (int j = pointer; j<process_info[process_no].lenth; j++)
	{
		if ((memory[adress + 3] & ACCESS_MASK) == 0 && (memory[adress + 3] & REVISE_MASK) == 0)
		{
			pointer = j;   //��¼�ҵ���ҳ��
			flag = false;
			break;//����ѭ��
		}
		adress += PAGE_ITEM_SIZE;
	}
	if (flag == false)
		return pointer;//����ҳ��
	adress = process_info[process_no].page_table_address;//δ�ҵ����ڶ�����ѯ
	for (int j = pointer; j<process_info[process_no].lenth; j++)
	{
		memory[adress + 3] &= (~ACCESS_MASK);    //����λ��0
		if ((memory[adress + 3] & ACCESS_MASK) == 0 && (memory[adress + 3] & REVISE_MASK) == 1)
		{
			pointer = j;//��¼�ҵ���ҳ��
			flag = false;
			break;
		}
		adress += PAGE_ITEM_SIZE;
	}
	if (flag == false)
		return pointer;//����
	else
		return Clock(process_no);//�ݹ��ѯ
}


void show() {
	printf("\n\n*******������־**************\n");
	char ch[2] = { 'n','y' };
	for (int i = 0; i<PROCESS_NUM; i++) {
		//��i������
		printf("��%d�����̷�����־��\n", i);
		printf("����ҳ��\tȱҳ��\t\t����ҳ��\t��1��\t\t��2��\n");
		for (int j = 0; j<process_info[i].access_order; j++)
		{
			printf("%d\t\t%c\t\t%d\t\t%d\t\t%d\n", process_info[i].access_log[j].access_pageno, \
				ch[process_info[i].access_log[j].absence], \
				process_info[i].access_log[j].put_out_page, \
				process_info[i].access_log[j].cur_mem_page[0], \
				process_info[i].access_log[j].cur_mem_page[1]);

		}
		printf("��%d�����̷��ʴ�����%d ȱҳ������%d\n", i, process_info[i].access_order, \
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
			cache[i].frame_no = memory[process_info[process_no].page_table_address + page_no * PAGE_ITEM_SIZE + 1];   //��ֵ������
			//return cache[i].frame_no;
		}
	}
	ans = (cache_pointer % 4);   //ȡ�࣬�ҵ����Ƚ���cache��ҳ�ţ��滻��ȥ
	cache_pointer++;            //ָ���1
	cache[ans].page_no = page_no;
	cache[ans].process = process_no;
	cache[ans].flag = true;
	cache[ans].frame_no = memory[process_info[process_no].page_table_address + page_no * PAGE_ITEM_SIZE + 1];   //��ֵ������
	//return cache[ans].frame_no;
}