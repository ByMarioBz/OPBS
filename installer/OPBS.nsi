Unicode true
RequestExecutionLevel user
SetCompressor /SOLID lzma

!ifndef OPBS_VERSION
  !error "OPBS_VERSION no fue definido."
!endif
!ifndef OPBS_FILE_VERSION
  !error "OPBS_FILE_VERSION no fue definido."
!endif
!ifndef PAYLOAD_DIR
  !error "PAYLOAD_DIR no fue definido."
!endif
!ifndef OUTPUT_FILE
  !error "OUTPUT_FILE no fue definido."
!endif

Name "OPBS ${OPBS_VERSION}"
OutFile "${OUTPUT_FILE}"
InstallDir "$LOCALAPPDATA\Programs\OPBS"
InstallDirRegKey HKCU "Software\OPBS" "InstallLocation"
Icon "${__FILEDIR__}\..\frontend\cmake\windows\obs-studio.ico"
UninstallIcon "${__FILEDIR__}\..\frontend\cmake\windows\obs-studio.ico"
VIProductVersion "${OPBS_FILE_VERSION}"
VIAddVersionKey /LANG=1034 "ProductName" "OPBS"
VIAddVersionKey /LANG=1034 "ProductVersion" "${OPBS_VERSION}"
VIAddVersionKey /LANG=1034 "FileVersion" "${OPBS_VERSION}"
VIAddVersionKey /LANG=1034 "FileDescription" "OPBS - Presentador integrado basado en OBS Studio"
VIAddVersionKey /LANG=1034 "LegalCopyright" "GPL-2.0"

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "OPBS" SEC_OPBS
  SetOutPath "$INSTDIR"
  File /r "${PAYLOAD_DIR}\*.*"
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  WriteRegStr HKCU "Software\OPBS" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OPBS" "DisplayName" "OPBS"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OPBS" "DisplayVersion" "${OPBS_VERSION}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OPBS" "Publisher" "OPBS"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OPBS" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OPBS" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OPBS" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OPBS" "NoRepair" 1

  CreateDirectory "$SMPROGRAMS\OPBS"
  CreateShortcut "$SMPROGRAMS\OPBS\OPBS.lnk" "$INSTDIR\bin\64bit\OPBS.exe" "--portable --disable-updater"
  CreateShortcut "$SMPROGRAMS\OPBS\Desinstalar OPBS.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortcut "$DESKTOP\OPBS.lnk" "$INSTDIR\bin\64bit\OPBS.exe" "--portable --disable-updater"
SectionEnd

Section "Uninstall"
  Delete "$DESKTOP\OPBS.lnk"
  Delete "$SMPROGRAMS\OPBS\OPBS.lnk"
  Delete "$SMPROGRAMS\OPBS\Desinstalar OPBS.lnk"
  RMDir "$SMPROGRAMS\OPBS"

  IfSilent keep_config
  MessageBox MB_YESNO|MB_DEFBUTTON2 \
    "¿También quieres eliminar la biblioteca, biblias, presentaciones y preferencias guardadas por OPBS?" \
    IDNO keep_config
  RMDir /r "$INSTDIR\config"

keep_config:
  RMDir /r "$INSTDIR\bin"
  RMDir /r "$INSTDIR\data"
  RMDir /r "$INSTDIR\obs-plugins"
  Delete "$INSTDIR\INICIAR_OPBS.bat"
  Delete "$INSTDIR\LEEME.txt"
  Delete "$INSTDIR\portable_mode.txt"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"

  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\OPBS"
  DeleteRegKey HKCU "Software\OPBS"
SectionEnd
