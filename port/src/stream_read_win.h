// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#ifdef _WIN32
#include <windows.h>
#include "compat_win.h"

/* A separate overlapped handle lets independent zone reads on the same package run
 * concurrently, without moving/restoring the CRT descriptor's shared file cursor.
 * The guest API remains synchronous: the buffer is not consumed before completion. */
static inline void *stream_read_open(int fd) {
    HANDLE source = (HANDLE)_get_osfhandle(fd);
    if (source == INVALID_HANDLE_VALUE) return NULL;
    HANDLE stream = ReOpenFile(source, GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              FILE_FLAG_OVERLAPPED);
    return stream == INVALID_HANDLE_VALUE ? NULL : stream;
}

static inline ssize_t stream_read_at(void *stream, void *buffer, size_t size, int64_t offset) {
    if (offset < 0 || size > (uint64_t)INT64_MAX - (uint64_t)offset) {
        errno = EINVAL; return -1;
    }
    if (!size) return 0;
    HANDLE event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!event) {
        compat_last_io_error = GetLastError();
        errno = compat_errno_from_win32(compat_last_io_error); return -1;
    }
    size_t done = 0;
    while (done < size) {
        OVERLAPPED at = {0};
        const uint64_t position = (uint64_t)offset + done;
        at.Offset = (DWORD)position;
        at.OffsetHigh = (DWORD)(position >> 32);
        at.hEvent = event;
        ResetEvent(event);
        const DWORD chunk = size - done > 0x1000000u ? 0x1000000u : (DWORD)(size - done);
        DWORD received = 0;
        BOOL ok = ReadFile((HANDLE)stream, (char *)buffer + done, chunk, &received, &at);
        if (!ok && GetLastError() == ERROR_IO_PENDING) {
            ok = GetOverlappedResult((HANDLE)stream, &at, &received, TRUE);
        }
        if (!ok) {
            const DWORD error = GetLastError();
            CloseHandle(event);
            if (error == ERROR_HANDLE_EOF) return (ssize_t)done;
            compat_last_io_error = error;
            errno = compat_errno_from_win32(error);
            return done ? (ssize_t)done : -1;
        }
        done += received;
        if (received < chunk) break;
    }
    CloseHandle(event);
    return (ssize_t)done;
}
#endif
