#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <sddl.h>
#include <Aclapi.h>

std::wstring FileTimeToString(const FILETIME& ft)
{
    SYSTEMTIME stUTC, stLocal;
    FileTimeToSystemTime(&ft, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

    wchar_t buffer[100];
    swprintf_s(buffer, L"%02d.%02d.%04d %02d:%02d:%02d",
        stLocal.wDay, stLocal.wMonth, stLocal.wYear,
        stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
    return buffer;
}

void PrintFileAttributes(DWORD attr)
{
    std::wcout << L"  Атрибути файлу:\n";
    if (attr & FILE_ATTRIBUTE_READONLY)   std::wcout << L"    • Тільки для читання\n";
    if (attr & FILE_ATTRIBUTE_HIDDEN)     std::wcout << L"    • Прихований\n";
    if (attr & FILE_ATTRIBUTE_SYSTEM)     std::wcout << L"    • Системний\n";
    if (attr & FILE_ATTRIBUTE_DIRECTORY)  std::wcout << L"    • Каталог\n";
    if (attr & FILE_ATTRIBUTE_ARCHIVE)    std::wcout << L"    • Архівний\n";
    if (attr & FILE_ATTRIBUTE_TEMPORARY)  std::wcout << L"    • Тимчасовий\n";
    if (attr & FILE_ATTRIBUTE_COMPRESSED) std::wcout << L"    • Стиснутий\n";
}

void PrintOwnerInfo(const std::wstring& path)
{
    PSID pOwner = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD res = GetNamedSecurityInfoW(
        path.c_str(),
        SE_FILE_OBJECT,
        OWNER_SECURITY_INFORMATION,
        &pOwner,
        NULL,
        NULL,
        NULL,
        &pSD
    );

    if (res == ERROR_SUCCESS)
    {
        WCHAR name[256], domain[256];
        DWORD nameSize = 256, domainSize = 256;
        SID_NAME_USE sidType;
        if (LookupAccountSidW(NULL, pOwner, name, &nameSize, domain, &domainSize, &sidType))
        {
            std::wcout << L"  Власник: " << domain << L"\\" << name << L"\n";
        }
        else
        {
            std::wcout << L"  Не вдалося визначити ім'я власника.\n";
        }
        LocalFree(pSD);
    }
    else
    {
        std::wcout << L"  Помилка при отриманні інформації про власника (код " << res << L")\n";
    }
}

int wmain(int argc, wchar_t* argv[])
{
    if (argc != 2)
    {
        std::wcout << L"Використання: " << argv[0] << L" <шлях_до_файлу>\n";
        return 0;
    }

    std::wstring path = argv[1];
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;

    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fileInfo))
    {
        std::wcerr << L"Не вдалося отримати інформацію про файл. Код помилки: " << GetLastError() << L"\n";
        return 1;
    }

    std::wcout << L"Інформація про файл: " << path << L"\n";
    std::wcout << L"--------------------------------------------------\n";


    PrintFileAttributes(fileInfo.dwFileAttributes);

    LARGE_INTEGER fileSize;
    fileSize.HighPart = fileInfo.nFileSizeHigh;
    fileSize.LowPart = fileInfo.nFileSizeLow;
    std::wcout << L"  Розмір файлу: " << fileSize.QuadPart << L" байт\n";

    std::wcout << L"  Час створення: " << FileTimeToString(fileInfo.ftCreationTime) << L"\n";
    std::wcout << L"  Останній доступ: " << FileTimeToString(fileInfo.ftLastAccessTime) << L"\n";
    std::wcout << L"  Остання модифікація: " << FileTimeToString(fileInfo.ftLastWriteTime) << L"\n";

    PrintOwnerInfo(path);

    std::wcout << L"--------------------------------------------------\n";
    return 0;
}