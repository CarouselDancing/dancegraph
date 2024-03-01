#include "utility.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace dancenet
{

    std::string readTextFile(const std::string& path)
    {
        std::stringstream buffer;
        std::ifstream ifsdb(path);
        if (ifsdb.is_open())
            buffer << ifsdb.rdbuf();
        else
            std::cerr << "Error opening file for reading: " << path << std::endl;
        return buffer.str();
    }

    void writeTextFile(const std::string& path, const std::string& text)
    {
        std::ofstream myfile(path);
        if (myfile.is_open())
            myfile << text;
        else
            std::cerr << "Error opening file for writing: " << path << std::endl;
    }

    std::vector<std::string> string_split(const std::string& str, char separator, int maxSplits)
    {
        std::vector<std::string> strings;
        int startIndex = 0, endIndex = 0;
        for (int i = 0; i <= str.size(); i++) {

            // If we reached the end of the word or the end of the input.
            if (str[i] == separator || i == str.size()) {
                endIndex = i;
                std::string temp;
                temp.append(str, startIndex, endIndex - startIndex);
                strings.push_back(temp);
                startIndex = endIndex + 1;
                if (strings.size() == maxSplits)
                    i = int(str.size());
            }
        }
        return strings;
    }

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
    std::string copyfiletotemp(std::string sharedLibraryPath) {

        HANDLE hFile = INVALID_HANDLE_VALUE;
        TCHAR lpTempPathBuffer[MAX_PATH];
        TCHAR szTempFileName[MAX_PATH];

        FILE* ifp = fopen(sharedLibraryPath.c_str(), "r");
        if (!ifp) {            
            return std::string("");
        }


        uint32_t dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer);
        

        //  Generates a temporary file name. 
        uint32_t rval = GetTempFileName(lpTempPathBuffer, // directory for tmp files
            TEXT("Dan"),     // temp file name prefix 
            0,                // create unique name 
            szTempFileName);  // buffer for name 

        if (!rval) {
            std::cout << "Can't get filename\n";
            return std::string("");
        }
        std::cout << "Temp filename: " << szTempFileName << "\n";
        std::filesystem::copy(sharedLibraryPath, szTempFileName, std::filesystem::copy_options::overwrite_existing);
        return std::string(szTempFileName);
    }


#endif


}