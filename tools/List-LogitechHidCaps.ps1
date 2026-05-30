param(
    [string]$Vid = "046D",
    [string]$ProductId = ""
)

$ErrorActionPreference = "Stop"

$source = @"
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;

public static class HidCapsLister {
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

    public class Row {
        public string DevicePath;
        public ushort VendorID;
        public ushort ProductID;
        public ushort VersionNumber;
        public ushort UsagePage;
        public ushort Usage;
        public ushort InputReportByteLength;
        public ushort OutputReportByteLength;
        public ushort FeatureReportByteLength;
    }

    const int DIGCF_PRESENT = 0x00000002;
    const int DIGCF_DEVICEINTERFACE = 0x00000010;
    const uint FILE_SHARE_READ = 0x00000001;
    const uint FILE_SHARE_WRITE = 0x00000002;
    const uint OPEN_EXISTING = 3;

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

    public static List<Row> List() {
        var rows = new List<Row>();
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
                    if (!HidD_GetAttributes(handle, ref attr)) {
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
                        if (HidP_GetCaps(pp, out caps) == 0x00110000) {
                            rows.Add(new Row {
                                DevicePath = detail.DevicePath,
                                VendorID = attr.VendorID,
                                ProductID = attr.ProductID,
                                VersionNumber = attr.VersionNumber,
                                UsagePage = caps.UsagePage,
                                Usage = caps.Usage,
                                InputReportByteLength = caps.InputReportByteLength,
                                OutputReportByteLength = caps.OutputReportByteLength,
                                FeatureReportByteLength = caps.FeatureReportByteLength
                            });
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
        return rows;
    }
}
"@

Add-Type -TypeDefinition $source

$vidNum = [Convert]::ToUInt16($Vid, 16)
$rows = [HidCapsLister]::List() | Where-Object { $_.VendorID -eq $vidNum }

if ($ProductId -ne "") {
    $pidNum = [Convert]::ToUInt16($ProductId, 16)
    $rows = $rows | Where-Object { $_.ProductID -eq $pidNum }
}

$rows |
    Select-Object `
        @{Name="VID";Expression={"{0:X4}" -f $_.VendorID}},
        @{Name="PID";Expression={"{0:X4}" -f $_.ProductID}},
        @{Name="UsagePage";Expression={"0x{0:X4}" -f $_.UsagePage}},
        @{Name="Usage";Expression={"0x{0:X4}" -f $_.Usage}},
        InputReportByteLength,
        OutputReportByteLength,
        FeatureReportByteLength,
        DevicePath |
    Sort-Object PID, UsagePage, Usage, DevicePath
