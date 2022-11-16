#include <iostream>
#include <Windows.h>
#include <vector>
#include <fstream>
#include <chrono>
#include <process.h>
#include <stdlib.h>

using namespace std;

HANDLE mat_mutex;
LPCWSTR mutexName = L"Mutex";
vector<vector<int>> mat1;
vector<vector<int>> mat2;
vector<vector<int>> res;

struct Data {
    int i1, k1, j1, k2, p1, k3;
    Data(int _i1, int _k1, int _j1, int _k2, int _p1, int _k3) {
        i1 = _i1;
        k1 = _k1;
        j1 = _j1;
        k2 = _k2;
        p1 = _p1;
        k3 = _k3;
    }
};

ostream& operator<<(ostream& os, vector<vector<int>> temp) {
    for (int i = 0; i < temp.size(); i++) {
        for (int j = 0; j < temp[0].size(); j++)
            os << temp[i][j] << ' ';
        os << '\n';
    }
    return os;
}
istream& operator>>(istream& in, vector<vector<int>>& temp) {
    int n, m;
    in >> n >> m;
    temp.resize(n, vector<int>(m));
    for (int i = 0; i < temp.size(); i++) {
        for (int j = 0; j < temp[0].size(); j++)
            in >> temp[i][j];
    }
    return in;
}
void calcBlock(void* pParam) {
    HANDLE mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, mutexName);
    WaitForSingleObject(mutex, INFINITE);
    Data* d = (Data*)(pParam);
    for (int i = d->i1; i < d->k1; i++)
        for (int j = d->j1; j < d->k2; j++)
            for (int p = d->p1; p < d->k3; p++) {
                res[i][j] += mat1[i][p] * mat2[p][j];
            }
    ReleaseMutex(mutex);
    CloseHandle(mutex);
}

void MatricesByBlocks(int k) {
    
    const int n = res.size();
    const int m = mat2.size();
    int t = m / k + (m % k != 0);
    t = t * t * t;
    HANDLE* threads = (HANDLE*)malloc(sizeof(HANDLE)*t);
    int count = 0;
    int i = 0, j, p;
    while (i + k <= n) {
        j = 0;
        while (j + k <= n) {
            p = 0;
            while (p + k <= m) {
                threads[count] = (HANDLE) _beginthread(calcBlock,0, new Data(i, i + k, j, j + k, p, p + k));
                count++;
                p += k;
            }
            if (m % k != 0) {
                threads[count] = (HANDLE)_beginthread(calcBlock, 0, new Data(i, i + k, j, j + k, p, p + m % k));
                count++;
            }
            j += k;
        }
        if (n % k != 0) {
            p = 0;
            while (p + k <= m) {
                threads[count] = (HANDLE)_beginthread(calcBlock, 0, new Data(i, i + k, j, j + n % k, p, p + k));
                count++;
                p += k;
            }
            if (m % k != 0) {
                threads[count] = (HANDLE) _beginthread(calcBlock,0, new Data(i, i + k, j, j + n % k, p, p + m % k));
                count++;
            }
        }
        i += k;
    }
    if (n % k != 0) {
        j = 0;
        while (j + k <= n) {
            p = 0;
            while (p + k <= m) {
                threads[count] = (HANDLE) _beginthread(calcBlock,0, new Data(i, i + n % k, j, j + k, p, p + k));
                count++;
                p += k;
            }
            if (m % k != 0) {
                threads[count] = (HANDLE) _beginthread(calcBlock,0,
                    new Data(i, i + n % k, j, j + k, p, p + m % k));
                count++;
            }
            j += k;
        }
        if (n % k != 0) {
            p = 0;
            while (p + k < m) {
                threads[count] = (HANDLE) _beginthread(calcBlock,0, new Data(i, i + n % k, j, j + n % k, p, p + k));
                count++;
                p += k;
            }
            if (m % k != 0) {
                threads[count] = (HANDLE) _beginthread(calcBlock,0, new Data(i, i + n % k, j, j + n % k, p, p + m % k));
                count++;
            };
        }
    }

    WaitForMultipleObjects(count, threads, TRUE, INFINITE);

}

int main() {
    ifstream fin("./dat.txt");
    mat_mutex = CreateMutex(NULL, FALSE, mutexName);
    fin >> mat1 >> mat2;
    int n = mat1.size();
    res = vector<vector<int>>(mat1.size(), vector<int>(mat1.size(), 0));
    for (int i = 1; i <= n; i++) {
        cout << "Block size: " << i;
        res = vector<vector<int>>(mat1.size(), vector<int>(mat1.size(), 0));
        auto start = chrono::high_resolution_clock::now();
        MatricesByBlocks(i);
        auto end = chrono::high_resolution_clock::now();
        cout << "   Time: " << chrono::duration<double>(end - start).count() << endl;
    }
    ReleaseMutex(mat_mutex);
    CloseHandle(mat_mutex);
    cout << res << endl;
    return 0;
}
