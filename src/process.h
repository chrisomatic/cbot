
void start_process(const char* binary, const char* args)
{

    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};

    si.cb = sizeof(si);

    int result = CreateProcess(
            binary
            ,args
            ,NULL
            ,NULL
            ,FALSE
            ,0
            ,NULL
            ,NULL
            ,&si
            ,&pi
        );

    if(!result)
    {
        printf( "CreateProcess failed (%d).\n",GetLastError());
        return;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle( pi.hProcess);
    CloseHandle( pi.hThread);
}
