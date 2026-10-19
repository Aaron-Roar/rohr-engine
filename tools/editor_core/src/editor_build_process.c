/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#else
#include <windows.h>
#endif
#include "editor_build_process.h"
#include <SDL3/SDL.h>
#include <string.h>
#include <stdlib.h>
#define EDITOR_BUILD_PUMP_BYTES 65536u
#define EDITOR_BUILD_STOP_MS 2000u
struct EditorBuildProcess {
    bool active, interrupting, exited, output_done;
    int exit_code;
    Uint64 deadline;
#ifdef _WIN32
    HANDLE process, job, output;
    DWORD pid;
#else
    pid_t pid;
    int output;
#endif
};
static EditorResult process_error(const char *detail) {
#ifdef _WIN32
    return editor_result_error(EDITOR_ERROR_FILE_IO, "%s (Windows error %lu)", detail, (unsigned long)GetLastError());
#else
    return editor_result_error(EDITOR_ERROR_FILE_IO, "%s: %s", detail, strerror(errno));
#endif
}
EditorBuildProcess *editor_build_process_create(void) {
    EditorBuildProcess *process = SDL_calloc(1, sizeof(*process));
#ifndef _WIN32
    if(process) process->output = -1;
#endif
    return process;
}
static void handles_close(EditorBuildProcess *process) {
#ifdef _WIN32
    if(process->output) CloseHandle(process->output);
    if(process->process) CloseHandle(process->process);
    if(process->job) CloseHandle(process->job);
#else
    if(process->output >= 0) close(process->output);
#endif
    *process = (EditorBuildProcess){0};
#ifndef _WIN32
    process->output = -1;
#endif
}
#ifdef _WIN32
static wchar_t *wide_text(const char *text) {
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, NULL, 0);
    if(count <= 0) return NULL;
    wchar_t *wide = SDL_malloc((size_t)count * sizeof(*wide));
    if(wide && !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, wide, count)) { SDL_free(wide); wide = NULL; }
    return wide;
}
/* Windows CRT argv quoting: double backslashes before quotes and at the closing
 * delimiter. Every argument is quoted, including empty arguments. */
static wchar_t *command_line(const char *const *args) {
    size_t capacity = 1;
    for(size_t i = 0; args[i]; ++i) {
        size_t length = strlen(args[i]);
        if(length > (32767 - 3) / 2 || capacity > 32767 - (length * 2 + 3)) return NULL;
        capacity += length * 2 + 3;
    }
    char *text = SDL_malloc(capacity); if(!text) return NULL;
    size_t used = 0;
    for(size_t i = 0; args[i]; ++i) {
        if(i) text[used++] = ' ';
        text[used++] = '"';
        const char *at = args[i];
        while(*at) {
            size_t slashes = 0;
            while(*at == '\\') { ++slashes; ++at; }
            size_t write = (*at == '"' || !*at) ? slashes * 2 : slashes;
            while(write--) text[used++] = '\\';
            if(*at == '"') text[used++] = '\\';
            if(*at) text[used++] = *at++;
        }
        text[used++] = '"';
    }
    text[used] = 0; wchar_t *wide = wide_text(text); SDL_free(text); return wide;
}
#endif
EditorResult editor_build_process_start(EditorBuildProcess *process, const char *directory,
        const char *const *arguments) {
    if(!process || process->active || !directory || !arguments || !arguments[0] || !arguments[0][0])
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Build needs an idle process, directory, and command");
    handles_close(process);
#ifdef _WIN32
    SECURITY_ATTRIBUTES security = {sizeof(security), NULL, TRUE};
    HANDLE write_pipe = NULL, null_input = INVALID_HANDLE_VALUE;
    wchar_t *command = command_line(arguments), *cwd = wide_text(directory);
    STARTUPINFOW startup = {.cb = sizeof(startup), .dwFlags = STARTF_USESTDHANDLES};
    PROCESS_INFORMATION information = {0};
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {0};
    if(!command || !cwd || !(process->job = CreateJobObjectW(NULL, NULL)) ||
            !CreatePipe(&process->output, &write_pipe, &security, 0) ||
            !SetHandleInformation(process->output, HANDLE_FLAG_INHERIT, 0)) goto failed;
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if(!SetInformationJobObject(process->job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) goto failed;
    null_input = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security, OPEN_EXISTING, 0, NULL);
    if(null_input == INVALID_HANDLE_VALUE) goto failed;
    startup.hStdInput = null_input; startup.hStdOutput = startup.hStdError = write_pipe;
    if(!CreateProcessW(NULL, command, NULL, NULL, TRUE,
            CREATE_SUSPENDED | CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW, NULL, cwd, &startup, &information)) goto failed;
    if(!AssignProcessToJobObject(process->job, information.hProcess)) {
        TerminateProcess(information.hProcess, 1); WaitForSingleObject(information.hProcess, INFINITE);
        CloseHandle(information.hProcess); CloseHandle(information.hThread); goto failed;
    }
    process->process = information.hProcess; process->pid = information.dwProcessId;
    if(ResumeThread(information.hThread) == (DWORD)-1) {
        CloseHandle(information.hThread); TerminateJobObject(process->job, 1);
        WaitForSingleObject(process->process, INFINITE); goto failed;
    }
    CloseHandle(information.hThread); CloseHandle(write_pipe); CloseHandle(null_input);
    SDL_free(command); SDL_free(cwd);
    process->active = true; return editor_result_value(true);
failed: {
    EditorResult result = process_error("Could not launch build command");
    if(write_pipe) CloseHandle(write_pipe);
    if(null_input != INVALID_HANDLE_VALUE) CloseHandle(null_input);
    SDL_free(command); SDL_free(cwd); handles_close(process); return result;
}
#else
    int pipes[2];
    if(pipe(pipes) != 0) return process_error("Could not create build output pipe");
    if(fcntl(pipes[0], F_SETFL, O_NONBLOCK) < 0 ||
            fcntl(pipes[0], F_SETFD, FD_CLOEXEC) < 0 || fcntl(pipes[1], F_SETFD, FD_CLOEXEC) < 0) {
        EditorResult result = process_error("Could not configure build output pipe"); close(pipes[0]); close(pipes[1]); return result;
    }
    pid_t child = fork();
    if(child == 0) {
        /* Only async-signal-safe operations between fork and exec. */
        if(setpgid(0, 0) != 0) _exit(126);
        signal(SIGINT, SIG_DFL); signal(SIGTERM, SIG_DFL); signal(SIGPIPE, SIG_DFL);
        close(pipes[0]);
        int input = open("/dev/null", O_RDONLY);
        if(input < 0 || dup2(input, STDIN_FILENO) < 0 || dup2(pipes[1], STDOUT_FILENO) < 0 ||
                dup2(pipes[1], STDERR_FILENO) < 0 || chdir(directory) != 0) _exit(126);
        if(input > STDERR_FILENO) close(input);
        if(pipes[1] > STDERR_FILENO) close(pipes[1]);
        execvp(arguments[0], (char *const *)arguments);
        static const char failure[] = "Could not execute build command\n";
        ssize_t written = write(STDERR_FILENO, failure, sizeof(failure) - 1);
        (void)written; _exit(127);
    }
    if(child < 0) {
        EditorResult result = process_error("Could not create build process");
        close(pipes[0]); close(pipes[1]); return result;
    }
    close(pipes[1]);
    (void)setpgid(child, child);
    process->pid = child; process->output = pipes[0]; process->active = true;
    return editor_result_value(true);
#endif
}
void editor_build_process_interrupt(EditorBuildProcess *process) {
    if(!process || !process->active || process->interrupting) return;
    process->interrupting = true; process->deadline = SDL_GetTicks() + EDITOR_BUILD_STOP_MS;
#ifdef _WIN32
    /* GUI processes have no console. Console builds receive CTRL_BREAK when
     * attached; the job guarantees whole-tree termination in either case. */
    (void)GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, process->pid);
    (void)TerminateJobObject(process->job, 130);
#else
    (void)kill(-process->pid, SIGINT);
#endif
}
EditorResult editor_build_process_poll(EditorBuildProcess *process,
        EditorBuildOutput output, void *context, bool *finished, int *exit_code) {
    if(!process || !finished || !exit_code) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Invalid build poll arguments");
    *finished = !process->active; *exit_code = 0;
    if(!process->active) return editor_result_value(true);
    char buffer[4096]; size_t budget = EDITOR_BUILD_PUMP_BYTES;
    while(budget && !process->output_done) {
#ifdef _WIN32
        DWORD available = 0, count = 0;
        if(!PeekNamedPipe(process->output, NULL, 0, NULL, &available, NULL)) {
            if(GetLastError() == ERROR_BROKEN_PIPE) { process->output_done = true; break; }
            return process_error("Could not poll build output");
        }
        if(!available) break;
        if(!ReadFile(process->output, buffer, (DWORD)SDL_min(SDL_min(budget, sizeof(buffer)), available), &count, NULL))
            return process_error("Could not read build output");
#else
        ssize_t count = read(process->output, buffer, SDL_min(budget, sizeof(buffer)));
        if(count < 0) {
            if(errno == EINTR) continue;
            if(errno == EAGAIN || errno == EWOULDBLOCK) break;
            return process_error("Could not read build output");
        }
        if(!count) { process->output_done = true; break; }
#endif
        if(output) output(context, buffer, (size_t)count);
        budget -= (size_t)count;
    }
    if(process->interrupting && SDL_GetTicks() >= process->deadline) {
#ifdef _WIN32
        (void)TerminateJobObject(process->job, 130);
#else
        (void)kill(-process->pid, SIGKILL);
#endif
    }
    if(!process->exited) {
#ifdef _WIN32
        DWORD code;
        DWORD state = WaitForSingleObject(process->process, 0);
        if(state == WAIT_FAILED) return process_error("Could not poll build process");
        if(state == WAIT_OBJECT_0) {
            if(!GetExitCodeProcess(process->process, &code)) return process_error("Could not read build exit status");
            process->exited = true; process->exit_code = (int)code;
        }
#else
        int status; pid_t waited = waitpid(process->pid, &status, WNOHANG);
        if(waited < 0 && errno != EINTR) return process_error("Could not reap build process");
        if(waited > 0) {
            process->exited = true;
            process->exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
        }
#endif
    }
    if(process->exited && process->output_done) {
#ifndef _WIN32
        /* A custom command can leave background descendants with redirected
         * output. Their lifetime still belongs to this operation. */
        (void)kill(-process->pid, SIGKILL);
#endif
        *finished = true; *exit_code = process->exit_code; handles_close(process);
    }
    return editor_result_value(true);
}
bool editor_build_process_active_check(const EditorBuildProcess *process) { return process && process->active; }
void editor_build_process_destroy(EditorBuildProcess *process) {
    if(!process) return;
    editor_build_process_interrupt(process);
    Uint64 deadline = SDL_GetTicks() + EDITOR_BUILD_STOP_MS + 100;
    while(process->active && SDL_GetTicks() < deadline) {
        bool finished; int code;
        (void)editor_build_process_poll(process, NULL, NULL, &finished, &code);
        if(process->active) SDL_Delay(1);
    }
    if(process->active) {
#ifdef _WIN32
        (void)TerminateJobObject(process->job, 130);
        (void)WaitForSingleObject(process->process, INFINITE);
#else
        (void)kill(-process->pid, SIGKILL);
        if(!process->exited) while(waitpid(process->pid, NULL, 0) < 0 && errno == EINTR) {}
#endif
        handles_close(process);
    }
    SDL_free(process);
}
