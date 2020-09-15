#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <Windows.h> //thread 생성위함
#include <synchapi.h>
#include <stdlib.h>
#include <string.h>
#include <WinBase.h>

#define MAX_ROW 100
#define MAX_COL 100
#define DELIM		" \r\t"
#define MAX_LINE	1000

typedef struct _matrix {
	int row; //행의 개수
	int col; //열의 개수 
	int element[MAX_ROW][MAX_COL];
}matrix;

typedef struct _C_element_index {
	int row; //행의 인덱스
	int col; //열의 인덱스
}matrix_index;

//thread에게 넘겨줄 매개변수 구조체
typedef struct _arg{
	matrix_index element_index;
	matrix* A; //읽기 위함
	matrix* B; //읽기 위함
	matrix* C; //저장하기 위함
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
		printf("행렬 파일입력하세요.\n");
		exit(1);
	}

	read_matrix(&A, argv[1]);
	for (int i = 2; i < argc; i++)
	{
		T1 = GetTickCount();
		read_matrix(&B, argv[i]);

		memset(C.element, 0, sizeof(int)*MAX_ROW*MAX_COL); //A*B하기 전에 C내용 초기화시키기
		matrix_multiplication(&A, &B, &C); //C = A*B
	
		printf("\n(%4dx%-4d) X (%4dx%-4d) = (%4dx%-4d)", A.row, A.col, B.row, B.col, C.row, C.col);

		//C내용을 A에 복사하기
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

	//thread를 C배열의 크기만큼 동적할당
	hThreadArray = (HANDLE **)malloc(sizeof(HANDLE) * C->row);
	argument = (arg**)malloc(sizeof(arg)*C->row);
	for (int i = 0; i < row; i++) {
		hThreadArray[i] = (HANDLE*)malloc(sizeof(HANDLE) * C->col);
		argument[i] = (arg*)malloc(sizeof(arg)* C->col);
	}

	//thread가 16000개 이상돼면 thread만드는데에 실패하는듯.
	//16000개씩 처리할까 생각했지만, 그냥 그거말고도 thread만드는데 실패할 수 도 있으므로, null반환마다 멈춰서 전의 thread들 모두 처리한 후에 다시 그 thread를 만들어서 수행하는 걸로 했다.
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
				//쓰레드 만들기 실패하면 전까지 했던 것을 다 해치운다. 그러고 j를 하나 낮춘다음에 다시 그 쓰레드 만들게 함.
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
	//thread 다 실행 될 때까지 기다림
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
	//thread 다 실행 될 때까지 기다림-실패한것들
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
	//waitformultipleobjects함수는 한번에 기다리는 수가 64개임...
	//그래서 크기가 80이 넘어가면 그냥 안된 거였음.ㅋㅋㅋㅋㅋ
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

	//thread모두 종료
	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < col; j++)
		{
			CloseHandle(hThreadArray[i][j]);
		}
	}

	//thread동적할당한거 반환함
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
<CreateThread 함수>
스레드를 수행하는 함수, 스레드 생성 및 닫기 정보 있음.
추가적으로 CreateThread함수 매개변수 정보도 있음.
참고 : https://qkfmxha.tistory.com/78
한 프로세스 내에서 메인스레드(main) 제외한 다른 스레드가
생성되게 하고, 돌아가게 한다. 
한 프로세스 내에서 작업을 나눠서 할 수 있게 도와준다.
하지만 '동시'에 실행되는 것이 아니라. 엄청나게 빠르게 스레드 전환이라는 작업이 일어나는 것이다.(context switching)
너무 빨라서 프로그램이 동시에 돌아가게 한다고 착각하게 만드는 것이다.

매개변수 관련해서 우리가 첫번째, 두번째 매개변수는 신경안써도 됨. 각각 NULL, 0을 넘기면 된다.
세번째 인자는 스레드 함수의 주소(pointer)넘겨준다.
(*함수 주소 = 함수의 이름)
함수의 기본 모형은 아무렇게나 하는게 아니라, 정해져있음
즉, DWORD WINAPI ThreadProc(LPVOID lpparameter);
이렇게 정해져 있다. 
LPVOID는 Long Pointer void(즉, void*)

참고 : https://3dmpengines.tistory.com/482

생성하려는 함수 파라미터가 여러개일 때에, 구조체로 넘겨준다. 네번째 매개변수가 그 넘겨주는 매개변수임.
참고: https://miffyzzang.tistory.com/425

*/
/*
WaitForMultipleObjects함수

64개이상의 thread일 경우 발생하는 문제 해결
https://jurang5.tistory.com/entry/ThreadWaitForMultipleObjects%EB%A5%BC-%EC%93%B8-%EB%95%8C%EB%8A%94-beginthread

waitforsingle
https://m.blog.naver.com/PostView.nhn?blogId=kkan22&logNo=80054180744&proxyReferer=https%3A%2F%2Fwww.google.com%2F
*/