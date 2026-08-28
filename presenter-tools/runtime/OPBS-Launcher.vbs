Option Explicit

Dim fileSystem, shell, scriptDirectory, powershellPath, launcherPath, command
Set fileSystem = CreateObject("Scripting.FileSystemObject")
Set shell = CreateObject("WScript.Shell")

scriptDirectory = fileSystem.GetParentFolderName(WScript.ScriptFullName)
powershellPath = shell.ExpandEnvironmentStrings("%SystemRoot%") & "\System32\WindowsPowerShell\v1.0\powershell.exe"
launcherPath = fileSystem.BuildPath(scriptDirectory, "OPBS-Launcher.ps1")
command = Chr(34) & powershellPath & Chr(34) & _
    " -NoLogo -NoProfile -NonInteractive -WindowStyle Hidden -ExecutionPolicy Bypass -File " & _
    Chr(34) & launcherPath & Chr(34)

' WScript ejecuta el comprobador sin crear una consola visible y devuelve el control de inmediato.
shell.Run command, 0, False
