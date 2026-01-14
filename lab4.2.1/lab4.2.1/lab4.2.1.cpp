#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <chrono>

int main() {
    const char* src = "testfile.bin";
    const char* dst = "copy_buffered.bin";

    FILE* fin = nullptr;
    FILE* fout = nullptr;

    if (fopen_s(&fin, src, "rb") != 0 || fopen_s(&fout, dst, "wb") != 0) {
        std::cerr << "Error to open file!\n";
        return 1;
    }

    const size_t BUF_SIZE = 1024 * 1024; // 1 MB
    char* buffer = new char[BUF_SIZE];

    auto start = std::chrono::high_resolution_clock::now();

    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, BUF_SIZE, fin)) > 0) {
        fwrite(buffer, 1, bytesRead, fout);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double timeSec = std::chrono::duration<double>(end - start).count();

    std::cout << "time (fread/fwrite): " << timeSec << " c\n";

    fclose(fin);
    fclose(fout);
    delete[] buffer;

    return 0;
}