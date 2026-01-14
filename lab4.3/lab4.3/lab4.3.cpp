#include <windows.h>
#include <vector>
#include <string>
#include <iostream>

constexpr DWORD BUFFER_SIZE = 1 << 20; 

struct AsyncFileContext {
    HANDLE hIn{};
    HANDLE hOut{};
    OVERLAPPED ov{};
    HANDLE hEvent{};
    std::vector<char> buffer;
    LARGE_INTEGER offset{};
    bool eof{ false };
};

HANDLE open_file_read(const std::wstring& path) {
    return CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr
    );
}

HANDLE open_file_write(const std::wstring& path) {
    return CreateFileW(
        path.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_FLAG_OVERLAPPED | FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr
    );
}

void start_async_read(AsyncFileContext& ctx) {
    ctx.ov.Offset = ctx.offset.LowPart;
    ctx.ov.OffsetHigh = ctx.offset.HighPart;
    ResetEvent(ctx.hEvent);

    DWORD bytesRead = 0;
    BOOL ok = ReadFile(
        ctx.hIn,
        ctx.buffer.data(),
        BUFFER_SIZE,
        &bytesRead,
        &ctx.ov
    );

    if (!ok && GetLastError() != ERROR_IO_PENDING) {
        ctx.eof = true;
    }
}

void complete_async_write(AsyncFileContext& ctx, DWORD bytes) {
    OVERLAPPED ovWrite{};
    ovWrite.Offset = ctx.offset.LowPart;
    ovWrite.OffsetHigh = ctx.offset.HighPart;

    DWORD written = 0;
    WriteFile(
        ctx.hOut,
        ctx.buffer.data(),
        bytes,
        &written,
        &ovWrite
    );

    GetOverlappedResult(ctx.hOut, &ovWrite, &written, TRUE);
    ctx.offset.QuadPart += written;
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 3 || argc % 2 == 0) {
        std::wcerr << L"Usage: async_multi_file_io in1 out1 [in2 out2 ...]\n";
        return 1;
    }

    std::vector<AsyncFileContext> contexts;
    std::vector<HANDLE> events;

    for (int i = 1; i < argc; i += 2) {
        AsyncFileContext ctx;
        ctx.hIn = open_file_read(argv[i]);
        ctx.hOut = open_file_write(argv[i + 1]);

        if (ctx.hIn == INVALID_HANDLE_VALUE || ctx.hOut == INVALID_HANDLE_VALUE) {
            std::wcerr << L"Failed to open files\n";
            return 1;
        }

        ctx.buffer.resize(BUFFER_SIZE);
        ctx.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        ctx.ov.hEvent = ctx.hEvent;

        contexts.push_back(ctx);
        events.push_back(ctx.hEvent);

        start_async_read(contexts.back());
    }

    size_t active = contexts.size();

    while (active > 0) {
        DWORD index = WaitForMultipleObjects(
            static_cast<DWORD>(events.size()),
            events.data(),
            FALSE,
            INFINITE
        );

        size_t i = index - WAIT_OBJECT_0;
        auto& ctx = contexts[i];

        DWORD bytes = 0;
        if (!GetOverlappedResult(ctx.hIn, &ctx.ov, &bytes, FALSE) || bytes == 0) {
            ctx.eof = true;
            CloseHandle(ctx.hEvent);
            active--;
            continue;
        }

        complete_async_write(ctx, bytes);
        start_async_read(ctx);
    }

    for (auto& ctx : contexts) {
        CloseHandle(ctx.hIn);
        CloseHandle(ctx.hOut);
    }

    std::wcout << L"Async processing completed.\n";
    return 0;
}
