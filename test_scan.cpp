#include <iostream>
#include <string>
#include <windows.h>
#include <vector>

int main() {
    WIN32_FIND_DATAA findDirData;
    HANDLE hFindDir = FindFirstFileA("Resources\\\\images\\\\skins\\\\*", &findDirData);
    if (hFindDir != INVALID_HANDLE_VALUE) {
        do {
            if (findDirData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                std::string dirName = findDirData.cFileName;
                if (dirName != "." && dirName != ".." && dirName != "Personalizados") {
                    std::cout << "Found dir: " << dirName << std::endl;
                }
            }
        } while (FindNextFileA(hFindDir, &findDirData) != 0);
        FindClose(hFindDir);
    } else {
        std::cout << "Failed to find resources" << std::endl;
    }
    return 0;
}
