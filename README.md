# matrix_multiplication_with_thread
matrix끼리 곱하는 프로그램. 계산을 병행적으로 빨리 하기 위해 thread개념을 이용했다.


## 전체 구조 및 사용한 함수

- A * B = C 형태이고, 주어진 행렬 A,B를 이용하여 빈 행렬 C를 thread를 이용해 채운다.

### main

```
A행렬을 읽어서(argv[1]) A행렬구조체에 넣는다.

For(i=2~argc전)
{
	GetTIckCount함수사용
	argv[i]를 행렬구조체 B에 넣는다.
	marix_multiplication함수 실행함
	내용이 채워진 C행렬을 A행렬로 복사한다.
}

Result파일을 열어서 C의 행렬 정보를 기입한다.
GetTickCount함수 사용해서 여기까지의 시간을 출력한다.
```

### 사용한 구조체 리스트

1. Matrix 
    - 구성 : (모두 int) row, col. Element[max_row][max_col] : 행렬의 정보를 저장한다. 행과 열의 개수와 그안의 구성요소를 저장한 배열이 있다.
2. matrx_index
    - 구성 : (모두 int) row, col : 계산되는 C행렬을 위해서 필요하다. C행렬의 그 인덱스를 저장하기 위해 존재함// threa에 넘겨줄려고 만든거임.
3. Arg
    - 구성 : (matrix_index) element_index, (matrix*) A,B,C : A,B,는 읽기위한 행렬구조체이고, C는 저장하기 곱한 결과를 저장하기 위해 존재하는 행렬구조체이다.
             이때 저장하는 C행렬의 인덱스를 알기위해 element_index가 있다.
             CreateThread에서 void*형으로 인자를 받는데 이때, 매개변수로 arg구조체의 포인터를 주게 된다.
             
 ### 사용한 함수 리스트
 
1. void read_matrix(matrix* M, char* filename)
	- 원래 제공한 파일의 코드를 매개변수를 조금 고쳐서 사용했다. 행렬의 정보에 해당하는 부분을 matrix* M으로만 바꿔서 그 안의 내용도 그에 맞게 바꾸었다.
	행렬의 파일을 읽는 함수이다.

2.  void matrix_multiplication(matrix* A, matrix* B, matrix* C)
	- 읽어오는 행렬구조체A,B두개와 값을 계산해서 저장하는C를 매개변수로 받는다.
	행렬의 곱을 수행하는 함수이다. 이함수에서 thread를 사용한다.
	thread를 일단 기본적으로 C배열의 크기만큼 동적할당해서 CreateThread함수를 사용해서 thread를 만든다.
	이때, 매개변수도 C의 크기만큼 동적할당해서 주게되는데, 이때 매개변수는 arg구조체를 사용한다. 
	
	- 처음에 실행했을 때, 에러가 났던 부분이, CreateThread부분과 threa가 다 실행될 떄까지 기다릴 때 쓰는 함수인, WaitForMultipleObjects함수에서 났었다.
	thread개수가 일정개수가 넘어가거나 어떤 이유에서는 thread자체가 생성되지 않았다. 그래서, 실패하면 WaitForMultipleObjects함수를 써서 전의 threa들을 
	모두 다 처리하게 했고, 그 다음 다시 그 생성되지 않았던 thrread부터 다시 생성하는 식으로 했다.
	WaitForMultipleObjects에서는 처음에 몰랐던 것이 한번에 기다리는 threa수가 64개였다는 것이다. 그래서 배열인덱스 중 하나가 64개가 넘어가면 아예 기다리지 않고 넘어가
	서 그 부분에 해당하는 부분이 모두 0이 되었었다. 그래서, 기다리는 수를 64개가 돼게 고쳤다.

	- 그 후 thead들을 모두 종료 시키고, 동적할당한 것을 반환했다.

3. DWORD WINAPI ThreadProc(void* Arg)
	- thread를 만들때 주는 함수이다.  C의 배열의 요소를 계산하는 함수이다. 시작전에 1초를 쉬기 위해 Sleep(1000)을 했다.

4. CreateThread : thread를 수행하는 함수이다. 이때 넘겨주는 매개변수 중에서 신경쓴거는 함수의 정보가 들어가는 세번째, 네번째 변수부분이다. 세번째에는 함수의 이름(즉, 함수의 주소)를 주고, 네번째 매개변수는 그 함수의 매개변수에 해당하는 것을 준다. 이때 전해줘야하는 게 여러 개일 때 구조체를 써야해서 구조체 arg를 만들어 썼다.
5. waitForMultipleObjects함수 :다수의 thread를 다 실행될때까찌 기다려주는 함수이다. 최대 64개까지 기다린다.

**4,5는 라이브러리에 있는 함수를 사용한 것임. 1,2,3은 내가 만든 함수


## test description

![image](https://user-images.githubusercontent.com/52481037/93222307-cb443500-f7a9-11ea-9995-de57cf16f90c.png)

![image](https://user-images.githubusercontent.com/52481037/93222316-cd0df880-f7a9-11ea-92ce-c7d7afb37ed0.png)

![image](https://user-images.githubusercontent.com/52481037/93222337-d1d2ac80-f7a9-11ea-810f-835fd24f3822.png)

## self evaluation

- 생각보다 어려웠다. 
전반적인 구조를 짜는데는 얼마안걸렸지만, waitformultipleobjects함수에서 일차적인 문제를 겪었고, 
좀 나아졌지만 또 안돼서 봤더니, CreateThread가 다 되지않았다는 점을 알고 고쳤다. 
이 과제를 통해서 thread에 대한 개념을 직접 구현해봄으로써 더 확실하게 알게 되었다. 



