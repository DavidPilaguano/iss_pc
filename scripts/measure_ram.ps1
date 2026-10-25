# SPDX-License-Identifier: GPL-2.0-or-later
<##
Sample an already running game without changing its settings. Working set includes
shared/mapped pages; private commit is a different metric, not private resident RAM.
GPU counters are per-process Windows counters and can be unavailable on some drivers.
##>
param(
    [Parameter(Mandatory = $true)][int]$ProcessId,
    [Parameter(Mandatory = $true)][string]$OutputFile,
    [ValidateRange(1, 3600)][int]$Seconds = 180
)
$ErrorActionPreference = 'Stop'
$samples = [System.Collections.Generic.List[object]]::new()
$timer = [System.Diagnostics.Stopwatch]::StartNew()
while ($timer.Elapsed.TotalSeconds -lt $Seconds) {
    $gameProcess = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
    if ($null -eq $gameProcess) { break }
    $dedicatedBytes = $null
    $sharedBytes = $null
    try {
        $gpuCounters = @(Get-CimInstance Win32_PerfFormattedData_GPUPerformanceCounters_GPUProcessMemory -ErrorAction Stop |
            Where-Object { $_.Name -like "pid_${ProcessId}_*" })
        if ($gpuCounters.Count -gt 0) {
            $dedicatedBytes = ($gpuCounters | Measure-Object -Property DedicatedUsage -Sum).Sum
            $sharedBytes = ($gpuCounters | Measure-Object -Property SharedUsage -Sum).Sum
        }
    } catch { } # A missing driver counter is unknown, not zero usage.
    $samples.Add([pscustomobject]@{
        ElapsedSeconds = [math]::Round($timer.Elapsed.TotalSeconds, 2)
        WorkingSetBytes = $gameProcess.WorkingSet64
        PrivateCommitBytes = $gameProcess.PrivateMemorySize64
        GpuDedicatedBytes = $dedicatedBytes
        GpuSharedBytes = $sharedBytes
    })
    Start-Sleep -Seconds 1
}
if ($samples.Count -eq 0) { throw "No samples: process $ProcessId is not running." }
$samples | Export-Csv -LiteralPath $OutputFile -NoTypeInformation -Encoding utf8
[pscustomobject]@{
    Samples = $samples.Count
    PeakWorkingSetMiB = [math]::Round(($samples | Measure-Object WorkingSetBytes -Maximum).Maximum / 1MB, 1)
    PeakPrivateCommitMiB = [math]::Round(($samples | Measure-Object PrivateCommitBytes -Maximum).Maximum / 1MB, 1)
    Csv = [System.IO.Path]::GetFullPath($OutputFile)
}
