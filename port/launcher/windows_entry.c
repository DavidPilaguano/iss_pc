// SPDX-License-Identifier: GPL-2.0-or-later
// Local source-checkout entry point. Game settings come from the launcher, never fixed overrides.
#ifndef ISS_ENTRY_PLAY
#define ISS_ENTRY_PLAY 0
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>

int WINAPI wWinMain(HINSTANCE self, HINSTANCE previous, PWSTR args, int show) {
    (void)self; (void)previous; (void)args; (void)show;
    wchar_t dir[32768], python[32768], command[32768], log_path[32768];
    static wchar_t old_path[32768], new_path[32768];
    DWORD size = GetModuleFileNameW(NULL, dir, 32768);
    if (!size || size >= 32768) return 1;
    wchar_t *slash = wcsrchr(dir, L'\\');
    if (!slash) return 1;
    *slash = 0;
    swprintf(python, 32768, L"%ls\\test-python\\Scripts\\python.exe", dir);
#if ISS_ENTRY_PLAY
    swprintf(command, 32768, L"\"%ls\" \"%ls\\..\\launcher\\infamous_run.py\"", python, dir);
#else
    swprintf(command, 32768, L"\"%ls\" \"%ls\\..\\launcher\\infamous_launcher_qt.py\"", python, dir);
#endif
#if ISS_ENTRY_PLAY
    swprintf(log_path, 32768, L"%ls\\Jugar-log.txt", dir);
#else
    swprintf(log_path, 32768, L"%ls\\Launcher-log.txt", dir);
#endif
    size = GetEnvironmentVariableW(L"PATH", old_path, 32768);
    if (size >= 32000) return 1;
    swprintf(new_path, 32768, L"C:\\msys64\\clang64\\bin;%ls", old_path);
    SetEnvironmentVariableW(L"PATH", new_path);
    wchar_t config[32768], user_dir[32768];
    swprintf(config, 32768, L"%ls\\entry.ini", dir);
    GetPrivateProfileStringW(L"launcher", L"user_dir", L"", user_dir, 32768, config);
    if (user_dir[0]) SetEnvironmentVariableW(L"ISS_USER_DIR", user_dir);
    SetEnvironmentVariableW(L"PYTHONIOENCODING", L"utf-8");
    SECURITY_ATTRIBUTES security = {sizeof(security), NULL, TRUE};
    HANDLE log = CreateFileW(log_path, GENERIC_WRITE, FILE_SHARE_READ, &security,
                             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE input = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (log == INVALID_HANDLE_VALUE || input == INVALID_HANDLE_VALUE) return 1;
    STARTUPINFOW startup = {0};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input;
    startup.hStdOutput = startup.hStdError = log;
    PROCESS_INFORMATION process = {0};
    BOOL started = CreateProcessW(python, command, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                                  NULL, dir, &startup, &process);
    CloseHandle(input);
    CloseHandle(log);
    if (!started) {
        MessageBoxW(NULL, L"No se pudo iniciar. Revisa Python y la instalacion en port\\out.",
                    L"inFAMOUS", MB_OK | MB_ICONERROR);
        return 1;
    }
    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hProcess);
    if (code) {
        swprintf(command, 32768, L"Se cerro con un error. Revisa el registro:\n%ls", log_path);
        MessageBoxW(NULL, command,
                    L"inFAMOUS", MB_OK | MB_ICONERROR);
    }
    return (int)code;
}
