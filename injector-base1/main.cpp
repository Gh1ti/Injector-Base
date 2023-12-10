#include "utils.h"

int main(int argc, char** argv) {
    const char* dll_path = "C:\Path\\To\\Your\\dll.dll";
    char process[256];

    printf("Porcess Name : ");
    scanf_s("%s", &process);
   
    DWORD targetProcessId = GetProcessIdByName(process);
    if (InjectDLL(targetProcessId, dll_path)) {
        printf("Successfully loaded dll");
    }
    else {
        printf("Fail to load dll");
    }

    return 0;
}
