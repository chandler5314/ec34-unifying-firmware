param(
    [string]$Vid = "046D",
    [string]$ProductId = "C52B",
    [string]$UsagePage = "FF00",
    [string]$Usage = "0001",
    [string]$ReadUsagePage = "",
    [string]$ReadUsage = "",
    [string]$Report = "10,FF,81,02,00,00,00",
    [int]$TimeoutMs = 1000
)

$ErrorActionPreference = "Stop"

$source = @"
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.InteropServices;

public static class HidppReportTool {
    [StructLayout(LayoutKind.Sequential)]
    public struct SP_DEVICE_INTERFACE_DATA {
        public int cbSize;
        public Guid InterfaceClassGuid;
        public int Flags;
        public IntPtr Reserved;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
    public struct SP_DEVICE_INTERFACE_DETAIL_DATA {
        public int cbSize;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
        public string DevicePath;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct HIDD_ATTRIBUTES {
        public int Size;
        public ushort VendorID;
        public ushort ProductID;
        public ushort VersionNumber;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct HIDP_CAPS {
        public ushort Usage;
        public ushort UsagePage;
        public ushort InputReportByteLength;
        public ushort OutputReportByteLength;
        public ushort FeatureReportByteLength;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 17)]
        public ushort[] Reserved;
        public ushort NumberLinkCollectionNodes;
        public ushort NumberInputButtonCaps;
        public ushort NumberInputValueCaps;
        public ushort NumberInputDataIndices;
        public ushort NumberOutputButtonCaps;
        public ushort NumberOutputValueCaps;
        public ushort NumberOutputDataIndices;
        public ushort NumberFeatureButtonCaps;
        public ushort NumberFeatureValueCaps;
        public ushort NumberFeatureDataIndices;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct OVERLAPPED {
        public IntPtr Internal;
        public IntPtr InternalHigh;
        public uint Offset;
        public uint OffsetHigh;
        public IntPtr hEvent;
    }

    public class Device {
        public string Path;
        public ushort InputLen;
        public ushort OutputLen;
    }

    const int DIGCF_PRESENT = 0x00000002;
    const int DIGCF_DEVICEINTERFACE = 0x00000010;
    const uint GENERIC_READ = 0x80000000;
    const uint GENERIC_WRITE = 0x40000000;
    const uint FILE_SHARE_READ = 0x00000001;
    const uint FILE_SHARE_WRITE = 0x00000002;
    const uint OPEN_EXISTING = 3;
    const uint FILE_FLAG_OVERLAPPED = 0x40000000;
    const uint WAIT_OBJECT_0 = 0x00000000;
    const uint WAIT_TIMEOUT = 0x00000102;
    const int ERROR_IO_PENDING = 997;

    [DllImport("hid.dll")]
    static extern void HidD_GetHidGuid(out Guid HidGuid);

    [DllImport("hid.dll", SetLastError = true)]
    static extern bool HidD_GetAttributes(IntPtr HidDeviceObject, ref HIDD_ATTRIBUTES Attributes);

    [DllImport("hid.dll", SetLastError = true)]
    static extern bool HidD_GetPreparsedData(IntPtr HidDeviceObject, out IntPtr PreparsedData);

    [DllImport("hid.dll", SetLastError = true)]
    static extern bool HidD_FreePreparsedData(IntPtr PreparsedData);

    [DllImport("hid.dll")]
    static extern int HidP_GetCaps(IntPtr PreparsedData, out HIDP_CAPS Capabilities);

    [DllImport("hid.dll", SetLastError = true)]
    static extern bool HidD_FlushQueue(IntPtr HidDeviceObject);

    [DllImport("setupapi.dll", SetLastError = true)]
    static extern IntPtr SetupDiGetClassDevs(ref Guid ClassGuid, IntPtr Enumerator, IntPtr hwndParent, int Flags);

    [DllImport("setupapi.dll", SetLastError = true)]
    static extern bool SetupDiEnumDeviceInterfaces(IntPtr DeviceInfoSet, IntPtr DeviceInfoData, ref Guid InterfaceClassGuid, int MemberIndex, ref SP_DEVICE_INTERFACE_DATA DeviceInterfaceData);

    [DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Auto)]
    static extern bool SetupDiGetDeviceInterfaceDetail(IntPtr DeviceInfoSet, ref SP_DEVICE_INTERFACE_DATA DeviceInterfaceData, IntPtr DeviceInterfaceDetailData, int DeviceInterfaceDetailDataSize, out int RequiredSize, IntPtr DeviceInfoData);

    [DllImport("setupapi.dll", SetLastError = true, CharSet = CharSet.Auto)]
    static extern bool SetupDiGetDeviceInterfaceDetail(IntPtr DeviceInfoSet, ref SP_DEVICE_INTERFACE_DATA DeviceInterfaceData, ref SP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData, int DeviceInterfaceDetailDataSize, out int RequiredSize, IntPtr DeviceInfoData);

    [DllImport("setupapi.dll", SetLastError = true)]
    static extern bool SetupDiDestroyDeviceInfoList(IntPtr DeviceInfoSet);

    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    static extern IntPtr CreateFile(string lpFileName, uint dwDesiredAccess, uint dwShareMode, IntPtr lpSecurityAttributes, uint dwCreationDisposition, uint dwFlagsAndAttributes, IntPtr hTemplateFile);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool CloseHandle(IntPtr hObject);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern IntPtr CreateEvent(IntPtr lpEventAttributes, bool bManualReset, bool bInitialState, string lpName);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool CancelIo(IntPtr hFile);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool ReadFile(IntPtr hFile, byte[] lpBuffer, uint nNumberOfBytesToRead, out uint lpNumberOfBytesRead, ref OVERLAPPED lpOverlapped);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool WriteFile(IntPtr hFile, byte[] lpBuffer, uint nNumberOfBytesToWrite, out uint lpNumberOfBytesWritten, ref OVERLAPPED lpOverlapped);

    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool GetOverlappedResult(IntPtr hFile, ref OVERLAPPED lpOverlapped, out uint lpNumberOfBytesTransferred, bool bWait);

    public static Device Find(ushort vid, ushort pid, ushort usagePage, ushort usage) {
        Guid hidGuid;
        HidD_GetHidGuid(out hidGuid);
        IntPtr set = SetupDiGetClassDevs(ref hidGuid, IntPtr.Zero, IntPtr.Zero, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (set == IntPtr.Zero || set.ToInt64() == -1) throw new Win32Exception(Marshal.GetLastWin32Error());
        try {
            int index = 0;
            while (true) {
                var data = new SP_DEVICE_INTERFACE_DATA();
                data.cbSize = Marshal.SizeOf(typeof(SP_DEVICE_INTERFACE_DATA));
                if (!SetupDiEnumDeviceInterfaces(set, IntPtr.Zero, ref hidGuid, index, ref data)) {
                    int err = Marshal.GetLastWin32Error();
                    if (err == 259) break;
                    throw new Win32Exception(err);
                }
                int required;
                SetupDiGetDeviceInterfaceDetail(set, ref data, IntPtr.Zero, 0, out required, IntPtr.Zero);
                var detail = new SP_DEVICE_INTERFACE_DETAIL_DATA();
                detail.cbSize = IntPtr.Size == 8 ? 8 : 5;
                if (!SetupDiGetDeviceInterfaceDetail(set, ref data, ref detail, required, out required, IntPtr.Zero)) {
                    index++;
                    continue;
                }
                IntPtr handle = CreateFile(detail.DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero);
                if (handle == IntPtr.Zero || handle.ToInt64() == -1) {
                    index++;
                    continue;
                }
                try {
                    var attr = new HIDD_ATTRIBUTES();
                    attr.Size = Marshal.SizeOf(typeof(HIDD_ATTRIBUTES));
                    if (!HidD_GetAttributes(handle, ref attr) || attr.VendorID != vid || attr.ProductID != pid) {
                        index++;
                        continue;
                    }
                    IntPtr pp;
                    if (!HidD_GetPreparsedData(handle, out pp)) {
                        index++;
                        continue;
                    }
                    try {
                        HIDP_CAPS caps;
                        if (HidP_GetCaps(pp, out caps) == 0x00110000 &&
                            caps.UsagePage == usagePage && caps.Usage == usage) {
                            return new Device { Path = detail.DevicePath, InputLen = caps.InputReportByteLength, OutputLen = caps.OutputReportByteLength };
                        }
                    } finally {
                        HidD_FreePreparsedData(pp);
                    }
                } finally {
                    CloseHandle(handle);
                }
                index++;
            }
        } finally {
            SetupDiDestroyDeviceInfoList(set);
        }
        return null;
    }

    static bool IsMatchingResponse(byte[] report, byte[] response) {
        if (report.Length < 4 || response.Length < 4) return true;

        byte dev = report[1];
        byte reqHi = report[2];
        byte reqLo = report[3];

        if (response[0] != 0x10 && response[0] != 0x11) return false;
        if (response[1] != dev && response[1] != (byte)(dev ^ 0xFF)) return false;

        if (response.Length >= 5 && response[2] == 0x8F && response[3] == reqHi && response[4] == reqLo) {
            return true;
        }
        if (response.Length >= 5 && response[2] == 0xFF && response[3] == reqHi && response[4] == reqLo) {
            return true;
        }
        return response[2] == reqHi && response[3] == reqLo;
    }

    public static byte[] Transact(Device writeDev, Device readDev, byte[] report, int timeoutMs) {
        if (writeDev == null) throw new Exception("HID++ write device not found.");
        if (readDev == null) throw new Exception("HID++ read device not found.");
        if (report.Length != writeDev.OutputLen) throw new Exception("Report length does not match output report length.");

        IntPtr writeHandle = CreateFile(writeDev.Path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, IntPtr.Zero, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, IntPtr.Zero);
        if (writeHandle == IntPtr.Zero || writeHandle.ToInt64() == -1) throw new Win32Exception(Marshal.GetLastWin32Error());
        IntPtr readHandle = writeHandle;
        bool separateReadHandle = readDev.Path != writeDev.Path;
        if (separateReadHandle) {
            readHandle = CreateFile(readDev.Path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, IntPtr.Zero, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, IntPtr.Zero);
            if (readHandle == IntPtr.Zero || readHandle.ToInt64() == -1) {
                CloseHandle(writeHandle);
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }
        }
        try {
            HidD_FlushQueue(readHandle);

            var writeOv = new OVERLAPPED();
            writeOv.hEvent = CreateEvent(IntPtr.Zero, true, false, null);
            if (writeOv.hEvent == IntPtr.Zero) throw new Win32Exception(Marshal.GetLastWin32Error());
            try {
                uint writtenNow;
                bool writeOk = WriteFile(writeHandle, report, (uint)report.Length, out writtenNow, ref writeOv);
                int writeErr = Marshal.GetLastWin32Error();
                if (!writeOk && writeErr != ERROR_IO_PENDING) throw new Win32Exception(writeErr);
                if (!writeOk) {
                    uint waitWrite = WaitForSingleObject(writeOv.hEvent, (uint)timeoutMs);
                    if (waitWrite != WAIT_OBJECT_0) throw new TimeoutException("Timed out writing HID++ report.");
                    uint written;
                    if (!GetOverlappedResult(writeHandle, ref writeOv, out written, false)) throw new Win32Exception(Marshal.GetLastWin32Error());
                }
            } finally {
                CloseHandle(writeOv.hEvent);
            }

            DateTime deadline = DateTime.UtcNow.AddMilliseconds(timeoutMs);
            byte[] lastInput = null;

            while (DateTime.UtcNow < deadline) {
                int remainingMs = (int)Math.Max(1, (deadline - DateTime.UtcNow).TotalMilliseconds);
                byte[] input = new byte[readDev.InputLen];
                var readOv = new OVERLAPPED();
                readOv.hEvent = CreateEvent(IntPtr.Zero, true, false, null);
                if (readOv.hEvent == IntPtr.Zero) throw new Win32Exception(Marshal.GetLastWin32Error());
                try {
                    uint readNow;
                    bool readOk = ReadFile(readHandle, input, (uint)input.Length, out readNow, ref readOv);
                    int readErr = Marshal.GetLastWin32Error();
                    if (!readOk && readErr != ERROR_IO_PENDING) throw new Win32Exception(readErr);

                    uint waitRead = WaitForSingleObject(readOv.hEvent, (uint)remainingMs);
                    if (waitRead == WAIT_TIMEOUT) {
                        CancelIo(readHandle);
                        break;
                    }
                    if (waitRead != WAIT_OBJECT_0) throw new Win32Exception(Marshal.GetLastWin32Error());

                    uint readBytes;
                    if (!GetOverlappedResult(readHandle, ref readOv, out readBytes, false)) throw new Win32Exception(Marshal.GetLastWin32Error());
                    Array.Resize(ref input, (int)readBytes);
                    lastInput = input;
                    if (IsMatchingResponse(report, input)) {
                        return input;
                    }
                } finally {
                    CloseHandle(readOv.hEvent);
                }
            }

            string suffix = lastInput == null ? "" : " Last received: " + BitConverter.ToString(lastInput).Replace("-", " ");
            throw new TimeoutException("Timed out waiting for matching HID++ response." + suffix);
        } finally {
            if (separateReadHandle) {
                CloseHandle(readHandle);
            }
            CloseHandle(writeHandle);
        }
    }
}
"@

Add-Type -TypeDefinition $source

function Convert-HexList([string]$text) {
    $parts = $text -split '[,;:\s-]+' | Where-Object { $_ -ne "" }
    $bytes = New-Object byte[] $parts.Count
    for ($i = 0; $i -lt $parts.Count; $i++) {
        $part = $parts[$i].Trim()
        if ($part.StartsWith("0x")) {
            $part = $part.Substring(2)
        }
        $bytes[$i] = [Convert]::ToByte($part, 16)
    }
    return $bytes
}

$vidNum = [Convert]::ToUInt16($Vid, 16)
$pidNum = [Convert]::ToUInt16($ProductId, 16)
$usagePageNum = [Convert]::ToUInt16($UsagePage, 16)
$usageNum = [Convert]::ToUInt16($Usage, 16)
$readUsagePageNum = if ($ReadUsagePage -eq "") { $usagePageNum } else { [Convert]::ToUInt16($ReadUsagePage, 16) }
$readUsageNum = if ($ReadUsage -eq "") { $usageNum } else { [Convert]::ToUInt16($ReadUsage, 16) }
$bytes = Convert-HexList $Report

$device = [HidppReportTool]::Find($vidNum, $pidNum, $usagePageNum, $usageNum)
if ($null -eq $device) {
    throw "No matching HID++ collection found."
}
$readDevice = [HidppReportTool]::Find($vidNum, $pidNum, $readUsagePageNum, $readUsageNum)
if ($null -eq $readDevice) {
    throw "No matching HID++ read collection found."
}

$response = [HidppReportTool]::Transact($device, $readDevice, $bytes, $TimeoutMs)
[PSCustomObject]@{
    DevicePath = $device.Path
    ReadDevicePath = $readDevice.Path
    Sent = (($bytes | ForEach-Object { "{0:X2}" -f $_ }) -join " ")
    Received = (($response | ForEach-Object { "{0:X2}" -f $_ }) -join " ")
}
