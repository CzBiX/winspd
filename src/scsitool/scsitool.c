/**
 * @file scsitool/scsitool.c
 *
 * @copyright 2018 Bill Zissimopoulos
 */
/*
 * This file is part of WinSpd.
 *
 * You can redistribute it and/or modify it under the terms of the GNU
 * General Public License version 3 as published by the Free Software
 * Foundation.
 *
 * Licensees holding a valid commercial license may use this software
 * in accordance with the commercial license agreement provided in
 * conjunction with the software.  The terms and conditions of any such
 * commercial license agreement shall govern, supersede, and render
 * ineffective any application of the GPLv3 license to this software,
 * notwithstanding of any reference thereto in the software or
 * associated repository.
 */

#include <winspd/winspd.h>
#include <shared/minimal.h>

#define PROGNAME                        "scsitool"

#define GLOBAL                          L"\\\\?\\"
#define GLOBALROOT                      L"\\\\?\\GLOBALROOT"

#define info(format, ...)               printlog(GetStdHandle(STD_OUTPUT_HANDLE), format, __VA_ARGS__)
#define warn(format, ...)               printlog(GetStdHandle(STD_ERROR_HANDLE), format, __VA_ARGS__)
#define fatal(ExitCode, format, ...)    (warn(format, __VA_ARGS__), ExitProcess(ExitCode))

static void vprintlog(HANDLE h, const char *format, va_list ap)
{
    char buf[1024];
        /* wvsprintf is only safe with a 1024 byte buffer */
    size_t len;
    DWORD BytesTransferred;

    wvsprintfA(buf, format, ap);
    buf[sizeof buf - 1] = '\0';

    len = lstrlenA(buf);
    buf[len++] = '\n';

    WriteFile(h, buf, (DWORD)len, &BytesTransferred, 0);
}

static void printlog(HANDLE h, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vprintlog(h, format, ap);
    va_end(ap);
}

static void usage(void)
{
    fatal(ERROR_INVALID_PARAMETER,
        "usage: %s COMMAND ARGS\n"
        "\n"
        "commands:\n"
        "    devpath device-name\n"
        "    inquiry device-name\n"
        "    report-luns device-name\n",
        PROGNAME);
}

static DWORD ScsiControlByName(PWSTR DeviceName,
    DWORD Ptl, PCDB Cdb, UCHAR DataDirection, PVOID DataBuffer, PDWORD PDataLength,
    PUCHAR PScsiStatus, UCHAR SenseInfoBuffer[32])
{
    HANDLE DeviceHandle = INVALID_HANDLE_VALUE;
    DWORD Error;

    Error = SpdOpenDevice(DeviceName, &DeviceHandle);
    if (ERROR_SUCCESS != Error)
        goto exit;

    Error = SpdScsiControl(DeviceHandle, Ptl, Cdb,
        DataDirection, DataBuffer, PDataLength, PScsiStatus, SenseInfoBuffer);

exit:
    if (INVALID_HANDLE_VALUE != DeviceHandle)
        CloseHandle(DeviceHandle);

    return Error;
}

static int devpath(int argc, wchar_t **argv)
{
    if (2 != argc)
        usage();

    WCHAR PathBuf[1024];
    DWORD Error;

    Error = SpdGetDevicePath(argv[1], PathBuf, sizeof PathBuf);
    if (ERROR_SUCCESS != Error)
        goto exit;

    info("%S", PathBuf);

exit:
    return Error;
}

static int inquiry(int argc, wchar_t **argv)
{
    if (2 != argc)
        usage();

    CDB Cdb;
    __declspec(align(16)) UCHAR DataBuffer[VPD_MAX_BUFFER_SIZE];
    DWORD DataLength = sizeof DataBuffer;
    UCHAR ScsiStatus;
    UCHAR SenseInfoBuffer[32];
    DWORD Error;

    memset(&Cdb, 0, sizeof Cdb);
    Cdb.CDB6INQUIRY3.OperationCode = SCSIOP_INQUIRY;
    Cdb.CDB6INQUIRY3.AllocationLength = VPD_MAX_BUFFER_SIZE;

    Error = ScsiControlByName(argv[1], 0, &Cdb, 1/*SCSI_IOCTL_DATA_IN*/,
        DataBuffer, &DataLength, &ScsiStatus, SenseInfoBuffer);

    return Error;
}

static int report_luns(int argc, wchar_t **argv)
{
    if (2 != argc)
        usage();

    return 0;
}

int wmain(int argc, wchar_t **argv)
{
    argc--;
    argv++;

    if (0 == argc)
        usage();

    if (0 == invariant_wcscmp(L"devpath", argv[0]))
        return devpath(argc, argv);
    else
    if (0 == invariant_wcscmp(L"inquiry", argv[0]))
        return inquiry(argc, argv);
    else
    if (0 == invariant_wcscmp(L"report-luns", argv[0]))
        return report_luns(argc, argv);
    else
        usage();

    return 0;
}

void wmainCRTStartup(void)
{
    DWORD Argc;
    PWSTR *Argv;

    Argv = CommandLineToArgvW(GetCommandLineW(), &Argc);
    if (0 == Argv)
        ExitProcess(GetLastError());

    ExitProcess(wmain(Argc, Argv));
}
