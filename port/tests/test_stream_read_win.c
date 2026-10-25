// SPDX-License-Identifier: GPL-2.0-or-later
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/runtime_file.c"

static int32_t guest_errno;
int32_t *runtime_errno(void) { return &guest_errno; }
int32_t runtime_guest_errno(int e) { return e; }
/* Freeze report wait marks (runtime_thread.c, not linked here). */
void runtime_wait_begin(const char *kind, const void *object, const char *name, const void *const *owner, uintptr_t frame) {
    (void)kind; (void)object; (void)name; (void)owner; (void)frame;
}
void runtime_wait_end(void) {}
/* Region trace hook (GPU library, not linked here). */
void tlggpu_note_open(const char *guest_path) { (void)guest_path; }
// Karma save hooks (runtime_karma.c, libtlggpu); file tests never open the karma save.
void runtime_karma_observe_save_read(void) {}
void iss_karma_save_open(int fd, int writing, uint64_t length) { (void)fd; (void)writing; (void)length; }
void iss_karma_save_part(int fd, const void *bytes, int64_t offset, int64_t size, int writing) { (void)fd; (void)bytes; (void)offset; (void)size; (void)writing; }
void iss_karma_save_close(int fd) { (void)fd; }
uintptr_t runtime_lookup(const RuntimeExport *table,size_t n,const char *name) {
    for (size_t i=0;i<n;++i) if (!strcmp(table[i].name,name)) return (uintptr_t)table[i].function;
    return 0;
}

enum { FILE_BYTES = 2 * 1024 * 1024, WORKERS = 6 };
static void *stream;
static int guest_fd = -1;
static unsigned char expected(size_t offset) { return (unsigned char)((offset * 17 + offset / 257) & 255); }

static void *read_worker(void *arg) {
    const size_t worker = (size_t)arg;
    unsigned char buffer[8193];
    for (size_t i = 0; i < 200; ++i) {
        const size_t offset = (i * 7919 + worker * 9871) % (FILE_BYTES - sizeof(buffer));
        const ssize_t got = guest_fd >= 0 ? do_pread(guest_fd, buffer, sizeof(buffer), (int64_t)offset)
                                         : stream_read_at(stream, buffer, sizeof(buffer), (int64_t)offset);
        assert(got == (ssize_t)sizeof(buffer));
        for (size_t j = 0; j < sizeof(buffer); ++j) assert(buffer[j] == expected(offset + j));
    }
    return NULL;
}

int main(void) {
    char folder[MAX_PATH], path[MAX_PATH];
    assert(GetTempPathA(MAX_PATH, folder));
    assert(GetTempFileNameA(folder, "iss", 0, path));
    FILE *file = fopen(path, "wb");
    assert(file);
    unsigned char *data = malloc(FILE_BYTES);
    assert(data);
    for (size_t i = 0; i < FILE_BYTES; ++i) data[i] = expected(i);
    assert(fwrite(data, 1, FILE_BYTES, file) == FILE_BYTES);
    assert(fclose(file) == 0);
    free(data);
    int fd = _open(path, _O_RDONLY | _O_BINARY);
    assert(fd >= 0);
    assert(_lseeki64(fd, 123, SEEK_SET) == 123);
    stream = stream_read_open(fd);
    assert(stream);
    pthread_t workers[WORKERS];
    for (size_t i = 0; i < WORKERS; ++i) assert(!pthread_create(&workers[i], NULL, read_worker, (void *)i));
    for (size_t i = 0; i < WORKERS; ++i) assert(!pthread_join(workers[i], NULL));
    assert(_lseeki64(fd, 0, SEEK_CUR) == 123);
    unsigned char tail[64];
    assert(stream_read_at(stream, tail, sizeof(tail), FILE_BYTES - 7) == 7);
    for (size_t i = 0; i < 7; ++i) assert(tail[i] == expected(FILE_BYTES - 7 + i));
    assert(stream_read_at(stream, tail, sizeof(tail), FILE_BYTES) == 0);
    assert(stream_read_at(stream, tail, 0, 0) == 0);
    assert(stream_read_at(stream, tail, sizeof(tail), -1) == -1 && errno == EINVAL);
    assert(stream_read_at(stream, tail, sizeof(tail), INT64_MAX) == -1 && errno == EINVAL);
    DWORD handles_before = 0, handles_after = 0;
    assert(GetProcessHandleCount(GetCurrentProcess(), &handles_before));
    for (unsigned i = 0; i < 1000; ++i) assert(stream_read_at(stream, tail, sizeof(tail), i) == (ssize_t)sizeof(tail));
    assert(GetProcessHandleCount(GetCurrentProcess(), &handles_after));
    assert(handles_after == handles_before);
    /* Exercise the real guest APIs too: bounced reads, positioned/ordinary reads,
     * seek, writable saves and the synchronous fallback. */
    char user[MAX_PATH + 16], guest[MAX_PATH + 16];
    snprintf(user, sizeof(user), "%s.user", path);
    snprintf(guest, sizeof(guest), "/app0/%s", strrchr(path, '\\') + 1);
    runtime_file_configure(folder, user);
    guest_fd = (int)do_open(guest, 0, 0);
    assert(guest_fd >= 3 && files[guest_fd].stream);
    GuestStat info;
    assert(do_fstat(guest_fd, &info) == 0 && info.size == FILE_BYTES);
    assert(do_read(guest_fd, tail, 13) == 13);
    for (size_t i = 0; i < 13; ++i) assert(tail[i] == expected(i));
    for (size_t i = 0; i < WORKERS; ++i) assert(!pthread_create(&workers[i], NULL, read_worker, (void *)i));
    for (size_t i = 0; i < WORKERS; ++i) assert(!pthread_join(workers[i], NULL));
    assert(do_lseek(guest_fd, 0, SEEK_CUR) == 13);
    assert(do_read(guest_fd, tail, sizeof(tail)) == (ssize_t)sizeof(tail));
    for (size_t i = 0; i < sizeof(tail); ++i) assert(tail[i] == expected(13 + i));
    assert(do_lseek(guest_fd, -7, SEEK_END) == FILE_BYTES - 7);
    assert(do_read(guest_fd, tail, sizeof(tail)) == 7);
    for (size_t i = 0; i < 7; ++i) assert(tail[i] == expected(FILE_BYTES - 7 + i));
    assert(!do_close(guest_fd));
    guest_fd = -1;
    assert(do_open(guest, 2, 0) == -EROFS);
    assert(!_putenv_s("TLG_STREAM_READ", "0"));
    int fallback = (int)do_open(guest, 0, 0);
    assert(fallback >= 3 && !files[fallback].stream);
    assert(do_read(fallback, tail, sizeof(tail)) == (ssize_t)sizeof(tail));
    assert(do_pread(fallback, tail, sizeof(tail), 99) == (ssize_t)sizeof(tail));
    for (size_t i = 0; i < sizeof(tail); ++i) assert(tail[i] == expected(99 + i));
    assert(do_lseek(fallback, 0, SEEK_CUR) == (ssize_t)sizeof(tail));
    assert(!do_close(fallback));
    assert(!_putenv_s("TLG_STREAM_READ", ""));
    int save = (int)do_open("/data/stream-save", 0x202, 0644);
    assert(save >= 3 && !files[save].stream);
    assert(do_write(save, "test", 4) == 4 && !do_close(save));
    assert(!path_op("/data/stream-save", 2, 0));
    const char *subdirs[] = {"temp0", "download0", "data"};
    for (size_t i = 0; i < 3; ++i) {
        char subdir[MAX_PATH + 32];
        snprintf(subdir, sizeof(subdir), "%s/%s", user, subdirs[i]);
        assert(!rmdir(subdir));
    }
    assert(!rmdir(user));
    CloseHandle(stream);
    assert(_close(fd) == 0);
    assert(DeleteFileA(path));
    puts("PASS: package and guest streaming, 6 readers, exact bytes, EOF, seek, fallback, saves, no handle leak");
    return 0;
}
