// =====================================================================================
//	Copyright (C) 2019 by Jiaxing Shao.All rights reserved.
//	文 件 名:  main.cpp
//	作    者:  Jiaxing Shao, 13315567369@163.com
//	版 本 号:  1.0
//	创建时间:  2019年04月15日 17时07分20秒
//	Compiler:  g++
//	描    述:  
// =====================================================================================

#include <iostream>
using namespace std;
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void bubble_sort(int a[], int n);
void insertion_sort(int a[], int n);
void selection_sort(int a[], int n);
void quick_sort(int a[], int n);
void merge_sort(int a[], int n);
void print_sort(int a[], int n);

int main(int argc, char *argv[])
{
	int x[10] = {30, 2, 8, 96, 6, 15, 27, 66, 9, 0};
	int a[10], b[10], c[10], d[10], e[10];
	memcpy(a, x, sizeof(int) * 10);
	memcpy(b, x, sizeof(int) * 10);
	memcpy(c, x, sizeof(int) * 10);
	memcpy(d, x, sizeof(int) * 10);
	memcpy(e, x, sizeof(int) * 10);

	print_sort(x, 10);
	printf("***************** 冒泡排序 *******************\n");
	bubble_sort(a, 10);
	printf("***************** 插入排序 *******************\n");
	insertion_sort(b, 10);
	printf("***************** 选择排序 *******************\n");
	selection_sort(c, 10);
	printf("***************** 快速排序 *******************\n");
	quick_sort(d, 10);
	printf("***************** 归并排序 *******************\n");
	merge_sort(e, 10);
	printf("************************************\n");
	return EXIT_SUCCESS;
}

void bubble_sort(int a[], int n)
{
	int tmp = 0;
	for(int i = 0; i < n - 1; i++)
	{
		for(int j = 1; j < n - i; j++)
		{
			if(a[j - 1] > a[j])
			{
				tmp = a[j];
				a[j] = a[j - 1];
				a[j - 1] = tmp;
			}
		}
		print_sort(a, 10);
	}
}

void insertion_sort(int a[], int n)
{
	int tmp = 0;
	for(int i = 1; i < n; i++)
	{
		for(int j = 0; j < i; j++)
		{
			if(a[j] > a[i])
			{
				tmp = a[i];
				a[i] = a[j];
				a[j] = tmp;
			}
		}
		print_sort(a, 10);
	}
}

void selection_sort(int a[], int n)
{
	int tmp = 0;
	for(int i = 0; i < n - 1; i++)
	{
		for(int j = i + 1; j < n; j++)
		{
			if(a[i] > a[j])
			{
				tmp = a[i];
				a[i] = a[j];
				a[j] = tmp;
			}
		}
		print_sort(a, 10);
	}
}

void quick_sort(int a[], int n)
{
	{
		print_sort(a, 10);
	}
}

void merge_sort(int a[], int n)
{
	{
		print_sort(a, 10);
	}
}

void print_sort(int a[], int n)
{
	for(int i = 0; i < 10; i++)
	{
		cout << a[i] << " ";
	}
	cout << endl;
}

