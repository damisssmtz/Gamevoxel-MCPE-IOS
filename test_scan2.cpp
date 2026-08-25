#include <iostream>
#include <string>
#include <windows.h>
#include <vector>

bool isBedrockPack(const std::string& dirPath) {
    std::string skinsJsonPath = dirPath + "/skins.json";
    HANDLE hFile = CreateFileA(skinsJsonPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        return true;
    }
    return false;
}

int main() {
    WIN32_FIND_DATAA findDirData;
    HANDLE hFindDir = FindFirstFileA("Resources\\\\images\\\\skins\\\\*", &findDirData);
    if (hFindDir != INVALID_HANDLE_VALUE) {
        do {
            if (findDirData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                std::string dirName = findDirData.cFileName;
                if (dirName != "." && dirName != ".." && dirName != "Personalizados") {
                    std::cout << "Found dir: " << dirName;
                    std::string fullDirPath = "Resources/images/skins/" + dirName;
                    if (isBedrockPack(fullDirPath)) {
                        std::cout << " (Bedrock)" << std::endl;
                    } else {
                        std::cout << " (Classic)" << std::endl;
                    }
                }
            }
        } while (FindNextFileA(hFindDir, &findDirData) != 0);
        FindClose(hFindDir);
    } else {
        std::cout << "Failed to find resources" << std::endl;
    }
    return 0;
}
