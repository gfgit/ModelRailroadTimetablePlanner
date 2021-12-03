; Setup ModelRailroadTimetablePlanner and register variables for .ttt file associations

;--------------------------------
;Include Modern UI and FileFunc and LogicLib

 !include "MUI2.nsh"
  
  
!include "FileFunc.nsh"
!insertmacro RefreshShellIcons
!insertmacro un.RefreshShellIcons

!include LogicLib.nsh
  
;--------------------------------
;Definitions

!include "constants.nsh"

;--------------------------------
;General

;Name and file
Name "${COMPANY_NAME} - ${APP_NAME}"
OutFile "${APP_PRODUCT}-${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}-setup.exe"
Unicode True
SetCompressor /SOLID /FINAL lzma
  
;Default installation folder
InstallDir "$PROGRAMFILES64\${COMPANY_NAME}\${APP_PRODUCT}" ; x86_64 64-bit

;Get installation folder from registry if available
InstallDirRegKey HKCU "Software\${COMPANY_NAME} ${APP_PRODUCT}" ""

;Request application privileges for Windows Vista
RequestExecutionLevel admin ;Require admin rights on NT6+ (When UAC is turned on)

; Prevent blurry text on High DPI screen due to Windows stretching the window
ManifestDPIAware True

;--------------------------------
;Interface Settings

!define MUI_ABORTWARNING
  
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\orange-uninstall.ico"

;--------------------------------
;Pages

#!define MUI_WELCOMEPAGE_TITLE "$(welcome_title)"
#!define MUI_WELCOMEPAGE_TEXT "$(welcome_text)"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${MR_TIMETABLE_PLANNER_LICENSE}"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_LINK "$(visit_site)"
!define MUI_FINISHPAGE_LINK_LOCATION ${ABOUTURL}

!define MUI_FINISHPAGE_RUN "$INSTDIR\${MR_TIMETABLE_PLANNER_EXE}"
!define MUI_FINISHPAGE_NOREBOOTSUPPORT

!define MUI_FINISHPAGE_SHOWREADME "${MR_TIMETABLE_PLANNER_README}"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "$(show_readme_label)"

!insertmacro MUI_PAGE_FINISH

;Uninstaller Pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English" ; The first language is the default language
  !insertmacro MUI_LANGUAGE "Italian"
  
;--------------------------------
;Localized messages

LangString welcome_title ${LANG_ENGLISH} "Welcome to the ${APP_NAME} ${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD} Setup Wizard"
LangString welcome_title ${LANG_ITALIAN} "Benvenuto nel Wizard di installazione di ${APP_NAME} ${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"

LangString welcome_text ${LANG_ENGLISH} "This wizard will guide you through the installation of ${APP_NAME}"
LangString welcome_text ${LANG_ITALIAN} "Questa procedura ti guiderà nell'installazione di ${APP_NAME}"

LangString visit_site ${LANG_ENGLISH} "Visit the ${APP_NAME} site for the latest news, FAQs and support"
LangString visit_site ${LANG_ITALIAN} "Visita il sito di ${APP_NAME} per le ultime notizie, domande frequanti e ricevere supporto"

LangString show_readme_label ${LANG_ENGLISH} "Show release notes"
LangString show_readme_label ${LANG_ITALIAN} "Mostra note di rilascio"

LangString DESC_MainProgram ${LANG_ENGLISH} "Main application and settings files"
LangString DESC_MainProgram ${LANG_ITALIAN} "Applicazione principale e file di configurazione"

LangString DESC_SM_Shortcut ${LANG_ENGLISH} "Create shortcuts to Start Menu. This makes easier to start Model Railroad Timetable Planner"
LangString DESC_SM_Shortcut ${LANG_ITALIAN} "Crea collegamenti al Menu Start. Questo rende pi${U+00FA} facile l'avvio dell'applicazione" #${U+00FA} = ù (U accentata minuscola)

LangString DESC_FileAss ${LANG_ENGLISH} "Setup file associations to display an icon for Train Timetable Session files and be able to open the application by just double clicking on the file"
LangString DESC_FileAss ${LANG_ITALIAN} "Configura le associazioni dei file per mostrare un'icona sui file Train Timetable Session e per aprire l'applicazione semplicemente facendo doppio click sul file"

LangString keep_logs_message ${LANG_ENGLISH} "Keep logs files? This is useful if you need to send them"
LangString keep_logs_message ${LANG_ITALIAN} "Mantenere i file di log? E' utile se si intende inviarli"

LangString unist_previous_msg ${LANG_ENGLISH} "Uninstall previous version?"
LangString unist_previous_msg ${LANG_ITALIAN} "Disinstallare la versione precedente?"

LangString unist_failed_msg ${LANG_ENGLISH} "Failed to uninstall, continue anyway?"
LangString unist_failed_msg ${LANG_ITALIAN} "Impossibile disinstallare la versione precedente, procedere comunque?"

;--------------------------------
;Version info

VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${APP_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "Visit ${ABOUTURL}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${COMPANY_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" "${APP_NAME} is a trademark of ${COMPANY_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "© ${COMPANY_NAME}"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${APP_NAME} Installer"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"

VIAddVersionKey /LANG=${LANG_ITALIAN} "ProductName" "${APP_NAME}"
VIAddVersionKey /LANG=${LANG_ITALIAN} "Comments" "Visita ${ABOUTURL}"
VIAddVersionKey /LANG=${LANG_ITALIAN} "CompanyName" "${COMPANY_NAME}"
VIAddVersionKey /LANG=${LANG_ITALIAN} "LegalTrademarks" "${APP_NAME} è un marchio di ${COMPANY_NAME}"
VIAddVersionKey /LANG=${LANG_ITALIAN} "LegalCopyright" "© ${COMPANY_NAME}"
VIAddVersionKey /LANG=${LANG_ITALIAN} "FileDescription" "${APP_NAME} Installer"
VIAddVersionKey /LANG=${LANG_ITALIAN} "FileVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"

VIFileVersion ${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}.0
VIProductVersion "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}.0"

;--------------------------------
;Reserve Files
  
  ;If you are using solid compression, files that are required before
  ;the actual installation should be stored first in the data block,
  ;because this will make your installer start faster.
  
  !insertmacro MUI_RESERVEFILE_LANGDLL

;--------------------------------
;Installer Sections

Section "Application" main_program
	SetShellVarContext current
  
	SetOutPath $INSTDIR

        File ${MR_TIMETABLE_PLANNER_PATH}\${MR_TIMETABLE_PLANNER_EXE}
        File ${MR_TIMETABLE_PLANNER_EXTRA}\icons\icon.ico

        File ${MR_TIMETABLE_PLANNER_PATH}\*.dll

	SetOutPath $INSTDIR\platforms
        File ${MR_TIMETABLE_PLANNER_PATH}\platforms\*.dll

	SetOutPath $INSTDIR\printsupport
        File ${MR_TIMETABLE_PLANNER_PATH}\printsupport\*.dll

	SetOutPath $INSTDIR\styles
        File ${MR_TIMETABLE_PLANNER_PATH}\styles\*.dll

	SetOutPath $INSTDIR\imageformats
        File ${MR_TIMETABLE_PLANNER_PATH}\imageformats\*.dll
  
	SetOutPath $INSTDIR\icons
        File ${MR_TIMETABLE_PLANNER_EXTRA}\icons\lightning\lightning.svg
  
	SetOutPath $INSTDIR\translations
        File ${MR_TIMETABLE_PLANNER_PATH}\translations\*.qm
  
        SetOutPath "$LOCALAPPDATA\${COMPANY_NAME}\${APP_PRODUCT}"
	; Create empty settings file
        FileOpen $0 $OUTDIR\${MR_TIMETABLE_PLANNER_SETTINGS} w
	FileClose $0
	
	; Set application language to the language chosen int the installer (default English)
	; Convert LANG_ID to local language code
	StrCpy $0 "en_US"
	${Switch} $LANGUAGE
		${Case} ${LANG_ITALIAN}
			StrCpy $0 "it_IT"
			${Break}
		${Default}
			${Break}
	${EndSwitch}
        WriteINIStr $OUTDIR\${MR_TIMETABLE_PLANNER_SETTINGS} "General" "language" $0

	# Uninstaller - See function un.onInit and section "uninstall" for configuration
	WriteUninstaller "$INSTDIR\uninstall.exe"

	# Registry information for add/remove programs
        WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "DisplayName" "${COMPANY_NAME} - ${APP_PRODUCT} - ${DESCRIPTION}"
        WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
        WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
        WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "InstallLocation" "$\"$INSTDIR$\""
        WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "DisplayIcon" "$\"$INSTDIR\icon.ico$\""
        WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "Publisher" "$\"${COMPANY_NAME}$\""
        WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "HelpLink" "$\"${HELPURL}$\""
        WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "URLUpdateInfo" "$\"${UPDATEURL}$\""
        WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "URLInfoAbout" "$\"${ABOUTURL}$\""
        WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "DisplayVersion" "$\"${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}$\""
        WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "VersionMajor" ${VERSIONMAJOR}
        WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "VersionMinor" ${VERSIONMINOR}
	# There is no option for modifying or repairing the install
        WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "NoModify" 1
        WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "NoRepair" 1
	# Set the INSTALLSIZE constant (!defined at the top of this script) so Add/Remove Programs can accurately report the size
        WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "EstimatedSize" ${INSTALLSIZE}
	
        WriteRegStr HKCU "Software\${COMPANY_NAME} ${APP_PRODUCT}" "" $INSTDIR
        WriteRegDWORD HKCU "Software\${COMPANY_NAME} ${APP_PRODUCT}" "VersionMajor" "${VERSIONMAJOR}"
        WriteRegDWORD HKCU "Software\${COMPANY_NAME} ${APP_PRODUCT}" "VersionMinor" "${VERSIONMINOR}"
        WriteRegDWORD HKCU "Software\${COMPANY_NAME} ${APP_PRODUCT}" "VersionRevision" "77"
        WriteRegDWORD HKCU "Software\${COMPANY_NAME} ${APP_PRODUCT}" "VersionBuild" "${VERSIONBUILD}"

SectionEnd

; Open a section to create shortcuts to start menu
Section "Start Menu Shortcuts" sm_shorcuts
	# Start Menu
	SetShellVarContext current
        CreateDirectory "$SMPROGRAMS\${COMPANY_NAME}"
        CreateShortCut "$SMPROGRAMS\${COMPANY_NAME}\${APP_PRODUCT}.lnk" "$INSTDIR\${MR_TIMETABLE_PLANNER_EXE}" "" "$INSTDIR\icon.ico"
SectionEnd

; Open a section to register file type
Section "File associations" file_ass
	SetShellVarContext current
	SetOutPath $INSTDIR
  
        WriteRegStr HKCU "Software\Classes\.ttt" "" "MR_TIMETABLE_PLANNER.session"
        WriteRegStr HKCU "Software\Classes\.ttt" "PerceivedType" "document"
        WriteRegStr HKCU "Software\Classes\MR_TIMETABLE_PLANNER.session" "" "MRTPlanner Timetable Session File"
        WriteRegStr HKCU "Software\Classes\MR_TIMETABLE_PLANNER.session\DefaultIcon" "" "$INSTDIR\${MR_TIMETABLE_PLANNER_EXE},0"
        WriteRegStr HKCU "Software\Classes\MR_TIMETABLE_PLANNER.session\shell\open\command" "" '$INSTDIR\${MR_TIMETABLE_PLANNER_EXE} "%1"'
  
	DetailPrint $INSTDIR
  
	;Refresh icon cache
	${RefreshShellIcons}
SectionEnd

;--------------------------------
;Installer Functions

!macro UninstallExisting exitcode uninstcommand
Push `${uninstcommand}`
Call UninstallExisting
Pop ${exitcode}
!macroend
Function UninstallExisting
Exch $1 ; uninstcommand
Push $2 ; Uninstaller
Push $3 ; Len
StrCpy $3 ""
StrCpy $2 $1 1
StrCmp $2 '"' qloop sloop
sloop:
	StrCpy $2 $1 1 $3
	IntOp $3 $3 + 1
	StrCmp $2 "" +2
	StrCmp $2 ' ' 0 sloop
	IntOp $3 $3 - 1
	Goto run
qloop:
	StrCmp $3 "" 0 +2
	StrCpy $1 $1 "" 1 ; Remove initial quote
	IntOp $3 $3 + 1
	StrCpy $2 $1 1 $3
	StrCmp $2 "" +2
	StrCmp $2 '"' 0 qloop
run:
	StrCpy $2 $1 $3 ; Path to uninstaller
	StrCpy $1 161 ; ERROR_BAD_PATHNAME
	GetFullPathName $3 "$2\.." ; $InstDir
	IfFileExists "$2" 0 +4
	ExecWait '"$2" /S _?=$3' $1 ; This assumes the existing uninstaller is a NSIS uninstaller, other uninstallers don't support /S nor _?=
	IntCmp $1 0 "" +2 +2 ; Don't delete the installer if it was aborted
	Delete "$2" ; Delete the uninstaller
	RMDir "$3" ; Try to delete $InstDir
	RMDir "$3\.." ; (Optional) Try to delete the parent of $InstDir
Pop $3
Pop $2
Exch $1 ; exitcode
FunctionEnd
 
Function .onInit

	!insertmacro MUI_LANGDLL_DISPLAY

        ReadRegStr $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}" "UninstallString"
	${If} $0 != ""
	${AndIf} ${Cmd} `MessageBox MB_YESNO|MB_ICONQUESTION "$(unist_previous_msg)" /SD IDYES IDYES`
		!insertmacro UninstallExisting $0 $0
		${If} $0 <> 0
			MessageBox MB_YESNO|MB_ICONSTOP "$(unist_failed_msg)" /SD IDYES IDYES +2
				Abort
		${EndIf}
	${EndIf}
FunctionEnd

;--------------------------------
;Descriptions

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${main_program} $(DESC_MainProgram)
	!insertmacro MUI_DESCRIPTION_TEXT ${sm_shorcuts} $(DESC_SM_Shortcut)
	!insertmacro MUI_DESCRIPTION_TEXT ${file_ass} $(DESC_FileAss)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END
  
;--------------------------------
;Uninstaller Section

Section un.main_program

	# Remove Start Menu launcher
        Delete "$SMPROGRAMS\${COMPANY_NAME}\${APP_PRODUCT}.lnk"
	# Try to remove the Start Menu folder - this will only happen if it is empty
        RMDir "$SMPROGRAMS\${COMPANY_NAME}"
 
	# Remove files
        Delete $INSTDIR\${MR_TIMETABLE_PLANNER_EXE}
	Delete $INSTDIR\icon.ico
	
	Delete $INSTDIR\icons\lightning.svg
	RMDir $INSTDIR\icons
	
	Delete $INSTDIR\translations\*.qm
	RMDir $INSTDIR\translations
	
	# Ask user if they want to delete or keep log files. If they choose to keep them AppData folder is not removed
	MessageBox MB_YESNO "$(keep_logs_message)" IDYES delete_settings
                RMDir /r "$LOCALAPPDATA\${COMPANY_NAME}\${APP_PRODUCT}\logs"
	
delete_settings:	
        Delete "$LOCALAPPDATA\${COMPANY_NAME}\${APP_PRODUCT}\${MR_TIMETABLE_PLANNER_SETTINGS}"
        RMDir "$LOCALAPPDATA\${COMPANY_NAME}\${APP_PRODUCT}"
        RMDir "$LOCALAPPDATA\${COMPANY_NAME}"

	Delete $INSTDIR\*.dll

	RMDir /r $INSTDIR\logs

	Delete $INSTDIR\platforms\*.dll
	RMDir /r $INSTDIR\platforms

	Delete $INSTDIR\printsupport\*.dll
	RMDir /r $INSTDIR\printsupport

	Delete $INSTDIR\styles\*.dll
	RMDir /r $INSTDIR\styles

	Delete $INSTDIR\imageformats\*.dll
	RMDir /r $INSTDIR\imageformats
 
	# Remove uninstaller information from the registry
        DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANY_NAME} ${APP_PRODUCT}"
        DeleteRegKey HKCU "Software\${COMPANY_NAME} ${APP_PRODUCT}"
	
	; Unregister file associations in uninstall.exe
	!macro AssocDeleteFileExtAndProgId _hkey _dotext _pid
	ReadRegStr $R0 ${_hkey} "Software\Classes\${_dotext}" ""
		StrCmp $R0 "${_pid}" 0 +2
    DeleteRegKey ${_hkey} "Software\Classes\${_dotext}"

	DeleteRegKey ${_hkey} "Software\Classes\${_pid}"
	!macroend

        !insertmacro AssocDeleteFileExtAndProgId HKCU ".ttt" "MR_TIMETABLE_PLANNER.session"
	
	DetailPrint $INSTDIR
	DetailPrint $OUTDIR
	
	;Refresh icon cache
	${un.RefreshShellIcons}
	
	# Always delete uninstaller as the last action
	Delete $INSTDIR\uninstall.exe
 
	# Try to remove the install directory - this will only happen if it is empty
	RMDir $INSTDIR
SectionEnd
