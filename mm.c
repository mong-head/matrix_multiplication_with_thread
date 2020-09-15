#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <Windows.h> //thread ��������
#include <synchapi.h>
#include <stdlib.h>
#include <string.h>
#include <WinBase.h>

#define MAX_ROW 100
#define MAX_COL 100
#define DELIM		" \r\t"
#define MAX_LINE	1000

typedef struct _matrix {
	int row; //���� ����
	int col; //���� ���� 
	int element[MAX_ROW][MAX_COL];
}matrix;

typedef struct _C_element_index {
	int row; //���� �ε���
	int col; //���� �ε���
}matrix_index;

//thread���� �Ѱ��� �Ű����� ����ü
typedef struct _arg{
	matrix_index element_index;
	matrix* A; //�б� ����
	matrix* B; //�б� ����
	matrix* C; //�����ϱ� ����
}arg;

void read_matrix(matrix* M, char *filename);
void matrix_multiplication(matrix* A, matrix* B, matrix* C);
DWORD WINAPI ThreadProc(void* Arg);

int main(int argc, char** argv)
{
	//PTP_TIMER timer = CreateThreadpoolTimer()

	FILE *fp = stdout;
	double T1 = 0, T2 = 0;

	matrix A;
	matrix B;
	matrix C;
	C.col = 0;
	C.row = 0;

	if (argc < 2)
	{
		printf("��� �����Է��ϼ���.\n");
		exit(1);
	}

	read_matrix(&A, argv[1]);
	for (int i = 2; i < argc; i++)
	{
		T1 = GetTickCount();
		read_matrix(&B, argv[i]);

		memset(C.element, 0, sizeof(int)*MAX_ROW*MAX_COL); //A*B�ϱ� ���� C���� �ʱ�ȭ��Ű��
		matrix_multiplication(&A, &B, &C); //C = A*B
	
		printf("\n(%4dx%-4d) X (%4dx%-4d) = (%4dx%-4d)", A.row, A.col, B.row, B.col, C.row, C.col);

		//C������ A�� �����ϱ�
		memset(A.element, 0, sizeof(int)*MAX_ROW*MAX_COL);
		A.row = C.row;
		A.col = C.col;
		for (int j = 0; j < A.row; j++)
		{
			for (int k = 0; k < A.col; k++)
			{
				A.element[j][k] = C.element[j][k];
			}
		}
	}
	printf("\n");
	fp = fopen("result.txt", "w");
	printf("\n");

	for (int i = 0; i < C.row; i++) {
		for (int j = 0; j < C.col; j++) {
			printf("%10d", C.element[i][j]);
			fprintf(fp, "%10d", C.element[i][j]);
		}
		printf("\n");
		fprintf(fp, "\n");
	}

	fclose(fp);
	T2 = GetTickCount();
	printf("\nProcessing Time :  %10.3f sec \n\n", (T2 - T1) / 1000);

	return 0;
}

void read_matrix(matrix* M, char *filename)
{
	char line[MAX_LINE], *tok;
	FILE *fp;

	if (!(fp = fopen(filename, "r"))) { printf("ERROR: file open\n"); exit(0); }
	memset(M->element, 0, sizeof(int)*MAX_ROW*MAX_COL);
	M->row = 0;
	while (fgets(line, MAX_LINE, fp))
	{
		tok = strtok(line, DELIM);
		M->col = 0;
		do
		{
			M->element[M->row][M->col++] = atoi(tok);

		} while (tok = strtok(NULL, DELIM));
		M->row++;
	}
	fclose(fp);
}

void matrix_multiplication(matrix* A, matrix* B, matrix* C)
{
	int row = C->row = A->row;
	int col = C->col = B->col;

	HANDLE **hThreadArray;
	arg **argument;

	//thread�� C�迭�� ũ�⸸ŭ �����Ҵ�
	hThreadArray = (HANDLE **)malloc(sizeof(HANDLE) * C->row);
	argument = (arg**)malloc(sizeof(arg)*C->row);
	for (int i = 0; i < row; i++) {
		hThreadArray[i] = (HANDLE*)malloc(sizeof(HANDLE) * C->col);
		argument[i] = (arg*)malloc(sizeof(arg)* C->col);
	}

	//thread�� 16000�� �̻�Ÿ� thread����µ��� �����ϴµ�.
	//16000���� ó���ұ� ����������, �׳� �װŸ��� thread����µ� ������ �� �� �����Ƿ�, null��ȯ���� ���缭 ���� thread�� ��� ó���� �Ŀ� �ٽ� �� thread�� ���� �����ϴ� �ɷ� �ߴ�.
	int i = 0;
	int j = 0;
	int a = 0;
	for (; i < row; i++)
	{
		for (j=0; j < col; j++)
		{
			argument[i][j].A = A;
			argument[i][j].B = B;
			argument[i][j].C = C;
			argument[i][j].element_index.row = i;
			argument[i][j].element_index.col = j;

			hThreadArray[i][j] = CreateThread(NULL, 0, ThreadProc, &argument[i][j], 0, NULL);

			if (hThreadArray[i][j] == NULL)
			{
				//������ ����� �����ϸ� ������ �ߴ� ���� �� ��ġ���. �׷��� j�� �ϳ� ��������� �ٽ� �� ������ ����� ��.
				//printf("%d %d fail to create thread\n", i, j);
				for (a=0; a < i; a++)
				{
					if (j > 64) //j = col
					{
						int repeatwait = j / 64;
						int remainwait = j % 64;
						int temp1, temp2;
						for (temp1 = 0, temp2 = 0; temp1 < repeatwait; temp1++, temp2 += 64)
							WaitForMultipleObjects(64, &hThreadArray[a][temp2], TRUE, INFINITE);
					}
					else
						WaitForMultipleObjects(col, hThreadArray[a], TRUE, INFINITE);
				}
				j--;
			}
		}
	}
	//thread �� ���� �� ������ ��ٸ�
	for (a=0; a < i; a++)
	{
		if (j > 64) //j = col
		{
			int repeatwait = j / 64;
			int remainwait = j % 64;
			int temp1, temp2;
			for (temp1 = 0, temp2 = 0; temp1 < repeatwait; temp1++, temp2 += 64)
			{
				//DWORD dwRet = -1;
				//dwRet = WaitForMultipleObjects(64, &hThreadArray[a][temp2], TRUE, INFINITE);
				//checkMultiObjects(dwRet);
				WaitForMultipleObjects(64, &hThreadArray[a][temp2], TRUE, INFINITE);
			}
			/*for(int j=0;j<col;j++)
			WaitForMultipleObjects(64, &hThreadArray[i][j], TRUE, INFINITE);*/
		}
		else
			WaitForMultipleObjects(col, hThreadArray[a], TRUE, INFINITE);
	}
	//thread �� ���� �� ������ ��ٸ�-�����Ѱ͵�
	/*for (int i = 0; i < row; i++)
	{
	printf("i : %d\n", i);
	WaitForMultipleObjects(col, hThreadArray[i], TRUE, INFINITE);
	}*/
	//WaitForMultipleObjects(row, *hThreadArray, TRUE, INFINITE);
	/*for (int i = 0; i < row; i++)
	{
	for (int j = 0; j < col; j++)
	WaitForMultipleObjects(1, hThreadArray[i][j], TRUE, INFINITE);
	}*/
	//WaitForMultipleObjects(col*row, *hThreadArray, TRUE, INFINITE);
	//Sleep(8000);
	//waitformultipleobjects�Լ��� �ѹ��� ��ٸ��� ���� 64����...
	//�׷��� ũ�Ⱑ 80�� �Ѿ�� �׳� �ȵ� �ſ���.����������
	//
	/*DWORD ret;
	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			while(TRUE)
			{
				ret = WaitForSingleObject(&hThreadArray[i][j], INFINITE);

				if (ret == WAIT_FAILED)
					return 0;
				else if (ret == WAIT_ABANDONED)
				{
					ResetEvent(&hThreadArray[i][j]);
					continue;
				}
				else if (ret == WAIT_TIMEOUT)
					continue;
				else
					ResetEvent(&hThreadArray[i][j]);
			}
		}
	}
	*/

	//thread��� ����
	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			CloseHandle(hThreadArray[i][j]);
		}
	}

	//thread�����Ҵ��Ѱ� ��ȯ��
	for (int i = 0; i < row; i++)
		free(hThreadArray[i]);
	free(hThreadArray);
}

DWORD WINAPI ThreadProc(void* Arg)
{
	Sleep(1000);
	DWORD dwResult = 0;

	arg* thread = (arg*)Arg;

	int row = thread->element_index.row;
	int col = thread->element_index.col;

	thread->C->element[row][col] = 0;

	int calculate_count = thread->A->col;
	//printf("%d\n", calculate_count);

	for (int i = 0; i < calculate_count; i++)
	{
		thread->C->element[row][col] += thread->A->element[row][i] * thread->B->element[i][col];
		//printf("%d ", thread->C->element[row][col]);
	}
	return (dwResult);
}

/*
<CreateThread �Լ�>
�����带 �����ϴ� �Լ�, ������ ���� �� �ݱ� ���� ����.
�߰������� CreateThread�Լ� �Ű����� ������ ����.
���� : https://qkfmxha.tistory.com/78
�� ���μ��� ������ ���ν�����(main) ������ �ٸ� �����尡
�����ǰ� �ϰ�, ���ư��� �Ѵ�. 
�� ���μ��� ������ �۾��� ������ �� �� �ְ� �����ش�.
������ '����'�� ����Ǵ� ���� �ƴ϶�. ��û���� ������ ������ ��ȯ�̶�� �۾��� �Ͼ�� ���̴�.(context switching)
�ʹ� ���� ���α׷��� ���ÿ� ���ư��� �Ѵٰ� �����ϰ� ����� ���̴�.

�Ű����� �����ؼ� �츮�� ù��°, �ι�° �Ű������� �Ű�Ƚᵵ ��. ���� NULL, 0�� �ѱ�� �ȴ�.
����° ���ڴ� ������ �Լ��� �ּ�(pointer)�Ѱ��ش�.
(*�Լ� �ּ� = �Լ��� �̸�)
�Լ��� �⺻ ������ �ƹ����Գ� �ϴ°� �ƴ϶�, ����������
��, DWORD WINAPI ThreadProc(LPVOID lpparameter);
�̷��� ������ �ִ�. 
LPVOID�� Long Pointer void(��, void*)

���� : https://3dmpengines.tistory.com/482

�����Ϸ��� �Լ� �Ķ���Ͱ� �������� ����, ����ü�� �Ѱ��ش�. �׹�° �Ű������� �� �Ѱ��ִ� �Ű�������.
����: https://miffyzzang.tistory.com/425

*/
/*
WaitForMultipleObjects�Լ�

64���̻��� thread�� ��� �߻��ϴ� ���� �ذ�
https://jurang5.tistory.com/entry/ThreadWaitForMultipleObjects%EB%A5%BC-%EC%93%B8-%EB%95%8C%EB%8A%94-beginthread

waitforsingle
https://m.blog.naver.com/PostView.nhn?blogId=kkan22&logNo=80054180744&proxyReferer=https%3A%2F%2Fwww.google.com%2F
*/