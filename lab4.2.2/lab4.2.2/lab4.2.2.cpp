#include <windows.h>
#include <stdio.h>

int main(void) {
    LPCSTR input_file = "input_large.dat";
    LPCSTR output_file = "output_winapi.dat";

    HANDLE hIn = CreateFileA(input_file,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    HANDLE hOut = CreateFileA(output_file,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if (hIn == INVALID_HANDLE_VALUE || hOut == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "CreateFile failed, err=%lu\n", GetLastError());
        return 1;
    }

    const DWORD BUF_SIZE = 4 * 1024 * 1024; 
    char* buffer = (char*)malloc(BUF_SIZE);
    if (!buffer) {
        fprintf(stderr, "Memory alloc error\n");
        return 1;
    }

    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    DWORD bytesRead, bytesWritten;
    while (ReadFile(hIn, buffer, BUF_SIZE, &bytesRead, NULL) && bytesRead > 0) {
        if (!WriteFile(hOut, buffer, bytesRead, &bytesWritten, NULL) || bytesWritten != bytesRead) {
            fprintf(stderr, "WriteFile failed, err=%lu\n", GetLastError());
            break;
        }
    }

    QueryPerformanceCounter(&end);
    double elapsed = (double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
    printf("WinAPI time: %.3f seconds\n", elapsed);

    free(buffer);
    CloseHandle(hIn);
    CloseHandle(hOut);

    return 0;
}