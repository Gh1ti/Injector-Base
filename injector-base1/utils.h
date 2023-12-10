#pragma once
#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <string>

// Function to get the process ID by its name
DWORD GetProcessIdByName(const std::string& processName) {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(snapshot, &processEntry)) {
            do {
                if (processName.compare(processEntry.szExeFile) == 0) {
                    pid = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &processEntry));
        }

        CloseHandle(snapshot);
    }

    return pid;
}

bool InjectDLL(DWORD processId, const char* dllPath) {
    // Open the target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL) {
        std::cout << "Failed to open process. Error code: " << GetLastError() << std::endl;
        return false;
    }

    // Allocate memory in the target process to store the DLL path
    LPVOID pRemoteBuf = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (pRemoteBuf == NULL) {
        std::cout << "Failed to allocate memory in the remote process. Error code: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    // Write the DLL path to the allocated memory in the target process
    if (!WriteProcessMemory(hProcess, pRemoteBuf, dllPath, strlen(dllPath) + 1, NULL)) {
        std::cout << "Failed to write DLL path in the remote process. Error code: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Get the address of the LoadLibraryA function from kernel32.dll
    LPVOID pLoadLibrary = (LPVOID)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
    if (pLoadLibrary == NULL) {
        std::cout << "Failed to get address of LoadLibraryA. Error code: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Create a remote thread in the target process to execute LoadLibraryA with the DLL path as a parameter
    HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pRemoteBuf, 0, NULL);
    if (hRemoteThread == NULL) {
        std::cout << "Failed to create remote thread. Error code: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Wait for the remote thread to finish
    WaitForSingleObject(hRemoteThread, INFINITE);

    // Clean up
    CloseHandle(hRemoteThread);
    VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return true;
}