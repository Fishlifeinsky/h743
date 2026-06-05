#include "system.h"
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include "uart.h"
#include "vfs.h"
#include "rtc.h"
#include <unistd.h>

//--------------------------------------------------------------------+
// 外部接口: 系统调用桩 (BSP_SYSCALLS_ENABLED 控制)
//
//   控制台 fd: STDIN_FILENO / STDOUT_FILENO / STDERR_FILENO → UART
//   其他 fd:   通过 fd 映射表委托 VFS (newlib FILE._file 是 short)
//--------------------------------------------------------------------+

#if BSP_SYSCALLS_ENABLED

#define MAX_OPEN_FILES 16

static vfs_handle_t *g_fd_table[MAX_OPEN_FILES];

static int fd_alloc(vfs_handle_t *handle) {
    for (int i = 3; i < MAX_OPEN_FILES; i++) {
        if (g_fd_table[i] == NULL) {
            g_fd_table[i] = handle;
            return i;
        }
    }
    return -1;
}

static vfs_handle_t *fd_lookup(int fd) {
    if (fd < 3 || fd >= MAX_OPEN_FILES) return NULL;
    return g_fd_table[fd];
}

static void fd_free(int fd) {
    if (fd >= 3 && fd < MAX_OPEN_FILES) {
        g_fd_table[fd] = NULL;
    }
}

char *__env[1] = { 0 };
char **environ = __env;

void initialise_monitor_handles(void)
{
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

void _exit(int status)
{
    _kill(status, -1);
    while (1) {}
}

int _read(int file, char *ptr, int len)
{
    if (file == STDIN_FILENO) {
        return uart_read(0, ptr, len,5000);
    }
    vfs_handle_t *handle = fd_lookup(file);
    if (!handle) { errno = EBADF; return -1; }
    return vfs_read(handle, ptr, len);
}

int _write(int file, char *ptr, int len)
{
    if (file == STDOUT_FILENO || file == STDERR_FILENO) {
        return uart_write(0, ptr, len);
    }
    vfs_handle_t *handle = fd_lookup(file);
    if (!handle) { errno = EBADF; return -1; }
    return vfs_write(handle, ptr, len);
}

int _close(int file)
{
    if (file == STDIN_FILENO || file == STDOUT_FILENO || file == STDERR_FILENO) {
        errno = EBADF;
        return -1;
    }
    vfs_handle_t *handle = fd_lookup(file);
    if (!handle) { errno = EBADF; return -1; }
    int ret = vfs_close(handle);
    fd_free(file);
    return ret;
}

int _fstat(int file, struct stat *st)
{
    if (!st) {
        errno = EFAULT;
        return -1;
    }

    if (file == STDIN_FILENO || file == STDOUT_FILENO || file == STDERR_FILENO) {
        st->st_mode = S_IFCHR;
        return 0;
    }

    vfs_handle_t *handle = fd_lookup(file);
    if (!handle) { errno = EBADF; return -1; }
    return vfs_fstat(handle, st);
}

int _isatty(int file)
{
    if (file == STDIN_FILENO || file == STDOUT_FILENO || file == STDERR_FILENO) return 1;
    vfs_handle_t *handle = fd_lookup(file);
    if (handle && handle->node && handle->node->type == VFS_TYPE_DEVICE) return 1;
    return 0;
}

int _lseek(int file, int ptr, int dir)
{
    if (file == STDIN_FILENO || file == STDOUT_FILENO || file == STDERR_FILENO) {
        errno = ESPIPE;
        return -1;
    }

    vfs_handle_t *handle = fd_lookup(file);
    if (!handle) { errno = EBADF; return -1; }
    return vfs_lseek(handle, ptr, dir);
}

int _open(char *path, int flags, ...)
{
    int vfs_flags = vfs_from_std_flags(flags);
    vfs_handle_t *handle = vfs_open(path, vfs_flags);
    if (!handle) {
        errno = ENOENT;
        return -1;
    }
    int fd = fd_alloc(handle);
    if (fd < 0) {
        vfs_close(handle);
        errno = EMFILE;
        return -1;
    }
    DEBUG_PRINT("_open: path=\"%s\" fd=%d (handle=%p)\r\n", path, fd, handle);
    return fd;
}

int _wait(int *status)
{
    (void)status;
    errno = ECHILD;
    return -1;
}

int _unlink(char *name)
{
    int ret = vfs_unlink(name);
    if (ret < 0) errno = ENOENT;
    return ret;
}

static uint32_t rtc_to_epoch(uint8_t year, uint8_t month, uint8_t day,
                              uint8_t hour, uint8_t min, uint8_t sec)
{
    static const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30,
                                            31, 31, 30, 31, 30, 31};
    uint32_t total_days = 0;
    uint16_t full_year = 2000 + year;

    for (uint16_t y = 1970; y < full_year; y++) {
        total_days += 365;
        if (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0))
            total_days++;
    }

    for (uint8_t m = 1; m < month; m++) {
        total_days += days_in_month[m - 1];
        if (m == 2 && (full_year % 4 == 0 && (full_year % 100 != 0 || full_year % 400 == 0)))
            total_days++;
    }

    total_days += day - 1;
    return total_days * 86400 + hour * 3600 + min * 60 + sec;
}

int _gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz;

    if (!tv) {
        errno = EFAULT;
        return -1;
    }

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
        return -1;
    if (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
        return -1;

    tv->tv_sec  = rtc_to_epoch(sDate.Year, sDate.Month, sDate.Date,
                                sTime.Hours, sTime.Minutes, sTime.Seconds);
    tv->tv_usec = 0;
    return 0;
}

int _times(struct tms *buf)
{
    (void)buf;
    return -1;
}

int _stat(char *file, struct stat *st)
{
    if (!file || !st) {
        errno = EFAULT;
        return -1;
    }

    int ret = vfs_stat(file, st);
    if (ret < 0) errno = ENOENT;
    return ret;
}

int _link(char *old, char *new)
{
    int ret = vfs_link(old, new);
    if (ret < 0) errno = EMLINK;
    return ret;
}

int _fork(void)
{
    errno = EAGAIN;
    return -1;
}

int _execve(char *name, char **argv, char **env)
{
    (void)name;
    (void)argv;
    (void)env;
    errno = ENOMEM;
    return -1;
}

#endif // BSP_SYSCALLS_ENABLED
