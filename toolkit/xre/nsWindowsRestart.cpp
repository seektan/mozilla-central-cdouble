/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla XULRunner bootstrap.
 *
 * The Initial Developer of the Original Code is
 * Benjamin Smedberg <benjamin@smedbergs.us>.
 *
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Mozilla Foundation. All Rights Reserved.
 *
 * Contributor(s):
 *   Robert Strong <robert.bugzilla@gmail.com>
 *   Brian R. Bondy <netzen@gmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

// This file is not build directly. Instead, it is included in multiple
// shared objects.

#ifdef nsWindowsRestart_cpp
#error "nsWindowsRestart.cpp is not a header file, and must only be included once."
#else
#define nsWindowsRestart_cpp
#endif

#include "nsUTF8Utils.h"
#include "nsWindowsHelpers.h"

#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <wchar.h>
#include <rpc.h>
#include <aclapi.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "rpcrt4.lib")

#ifndef ERROR_ELEVATION_REQUIRED
#define ERROR_ELEVATION_REQUIRED 740L
#endif

BOOL (WINAPI *pCreateProcessWithTokenW)(HANDLE,
                                        DWORD,
                                        LPCWSTR,
                                        LPWSTR,
                                        DWORD,
                                        LPVOID,
                                        LPCWSTR,
                                        LPSTARTUPINFOW,
                                        LPPROCESS_INFORMATION);

BOOL (WINAPI *pIsUserAnAdmin)(VOID);

/**
 * Get the length that the string will take and takes into account the
 * additional length if the string needs to be quoted and if characters need to
 * be escaped.
 */
static int ArgStrLen(const PRUnichar *s)
{
  int backslashes = 0;
  int i = wcslen(s);
  BOOL hasDoubleQuote = wcschr(s, L'"') != NULL;
  // Only add doublequotes if the string contains a space or a tab
  BOOL addDoubleQuotes = wcspbrk(s, L" \t") != NULL;

  if (addDoubleQuotes) {
    i += 2; // initial and final duoblequote
  }

  if (hasDoubleQuote) {
    while (*s) {
      if (*s == '\\') {
        ++backslashes;
      } else {
        if (*s == '"') {
          // Escape the doublequote and all backslashes preceding the doublequote
          i += backslashes + 1;
        }

        backslashes = 0;
      }

      ++s;
    }
  }

  return i;
}

/**
 * Copy string "s" to string "d", quoting the argument as appropriate and
 * escaping doublequotes along with any backslashes that immediately precede
 * doublequotes.
 * The CRT parses this to retrieve the original argc/argv that we meant,
 * see STDARGV.C in the MSVC CRT sources.
 *
 * @return the end of the string
 */
static PRUnichar* ArgToString(PRUnichar *d, const PRUnichar *s)
{
  int backslashes = 0;
  BOOL hasDoubleQuote = wcschr(s, L'"') != NULL;
  // Only add doublequotes if the string contains a space or a tab
  BOOL addDoubleQuotes = wcspbrk(s, L" \t") != NULL;

  if (addDoubleQuotes) {
    *d = '"'; // initial doublequote
    ++d;
  }

  if (hasDoubleQuote) {
    int i;
    while (*s) {
      if (*s == '\\') {
        ++backslashes;
      } else {
        if (*s == '"') {
          // Escape the doublequote and all backslashes preceding the doublequote
          for (i = 0; i <= backslashes; ++i) {
            *d = '\\';
            ++d;
          }
        }

        backslashes = 0;
      }

      *d = *s;
      ++d; ++s;
    }
  } else {
    wcscpy(d, s);
    d += wcslen(s);
  }

  if (addDoubleQuotes) {
    *d = '"'; // final doublequote
    ++d;
  }

  return d;
}

/**
 * Creates a command line from a list of arguments. The returned
 * string is allocated with "malloc" and should be "free"d.
 *
 * argv is UTF8
 */
PRUnichar*
MakeCommandLine(int argc, PRUnichar **argv)
{
  int i;
  int len = 0;

  // The + 1 of the last argument handles the allocation for null termination
  for (i = 0; i < argc; ++i)
    len += ArgStrLen(argv[i]) + 1;

  // Protect against callers that pass 0 arguments
  if (len == 0)
    len = 1;

  PRUnichar *s = (PRUnichar*) malloc(len * sizeof(PRUnichar));
  if (!s)
    return NULL;

  PRUnichar *c = s;
  for (i = 0; i < argc; ++i) {
    c = ArgToString(c, argv[i]);
    if (i + 1 != argc) {
      *c = ' ';
      ++c;
    }
  }

  *c = '\0';

  return s;
}

/**
 * Convert UTF8 to UTF16 without using the normal XPCOM goop, which we
 * can't link to updater.exe.
 */
static PRUnichar*
AllocConvertUTF8toUTF16(const char *arg)
{
  // UTF16 can't be longer in units than UTF8
  int len = strlen(arg);
  PRUnichar *s = new PRUnichar[(len + 1) * sizeof(PRUnichar)];
  if (!s)
    return NULL;

  ConvertUTF8toUTF16 convert(s);
  convert.write(arg, len);
  convert.write_terminator();
  return s;
}

static void
FreeAllocStrings(int argc, PRUnichar **argv)
{
  while (argc) {
    --argc;
    delete [] argv[argc];
  }

  delete [] argv;
}

BOOL 
EnsureWindowsServiceRunning() {
  // Get a handle to the SCM database.
  nsAutoServiceHandle serviceManager(OpenSCManager(NULL, NULL, 
                                                   SC_MANAGER_CONNECT | 
                                                   SC_MANAGER_ENUMERATE_SERVICE));
  if (!serviceManager)  {
    return FALSE;
  }

  // Get a handle to the service.
  nsAutoServiceHandle service(OpenServiceW(serviceManager, 
                                           L"MozillaMaintenance", 
                                           SERVICE_QUERY_STATUS | SERVICE_START));
  if (!service) { 
    return FALSE;
  }

  // Make sure the service is not stopped.
  SERVICE_STATUS_PROCESS ssp;
  DWORD bytesNeeded;
  if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp,
                            sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded)) {
    return FALSE;
  }

  if (ssp.dwCurrentState == SERVICE_STOPPED) {
    if (!StartService(service, 0, NULL)) {
      return FALSE;
    }

    // Make sure we can get into a started state without waiting too long.
    // This usually starts instantly but the extra code is just in case it
    // takes longer.
    DWORD totalWaitTime = 0;
    static const int maxWaitTime = 1000 * 5; // Never wait more than 5 seconds
    while (QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp,
                                sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded)) {
      if (ssp.dwCurrentState == SERVICE_RUNNING) {
        break;
      } else if (ssp.dwCurrentState == SERVICE_START_PENDING &&
                 totalWaitTime > maxWaitTime) {
        // We will probably eventually start, but we can't wait any longer.
        break;
      } else if (ssp.dwCurrentState != SERVICE_START_PENDING) {
        return FALSE;
      }

      Sleep(ssp.dwWaitHint);
      // Increment by at least 10 milliseconds to ensure we always make 
      // progress towards maxWaitTime in case dwWaitHint is 0.
      totalWaitTime += (ssp.dwWaitHint + 10);
    }
  }

  return ssp.dwCurrentState == SERVICE_RUNNING;
}


BOOL
PathAppendSafe(LPWSTR base, LPCWSTR extra)
{
  if (wcslen(base) + wcslen(extra) >= MAX_PATH) {
    return FALSE;
  }

  return PathAppendW(base, extra);
}

BOOL
PathGetSiblingFilePath(LPWSTR destinationBuffer, 
                       LPCWSTR siblingFilePath, 
                       LPCWSTR newFileName)
{
  if (wcslen(siblingFilePath) >= MAX_PATH) {
    return FALSE;
  }

  wcscpy(destinationBuffer, siblingFilePath);
  if (!PathRemoveFileSpecW(destinationBuffer))
    return FALSE;

  if (wcslen(destinationBuffer) + wcslen(newFileName) >= MAX_PATH) {
    return FALSE;
  }

  return PathAppendSafe(destinationBuffer, newFileName);
}

BOOL
GetUpdateDirectoryPath(WCHAR *path) 
{
  HRESULT hr = SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 
    SHGFP_TYPE_CURRENT, path);
  if (FAILED(hr)) {
    return FALSE;
  }
  if (!PathAppendSafe(path, L"Mozilla")) {
    return FALSE;
  }
  // The directory should already be created from the installer, but
  // just to be safe in case someone deletes.
  CreateDirectoryW(path, NULL);

  if (!PathAppendSafe(path, L"updates")) {
    return FALSE;
  }
  CreateDirectoryW(path, NULL);
  return TRUE;
}

/**
 * Launch a service initiated action with the specified arguments.
 *
 * @param  exePath The path of the executable to run
 * @param  argc    The total number of arguments in argv
 * @param  argv
 *         An array of null terminated strings to pass to the exePath, 
 *         argv[0] is ignored
 * @return TRUE if successful
 */
BOOL
WinLaunchServiceCommand(const PRUnichar *exePath, int argc, PRUnichar **argv)
{
  // Ensure the service is running, if not we should try to start it, if it is
  // not in a running state we cannot execute a service command.
  if (!EnsureWindowsServiceRunning()) {
    return FALSE;
  }

  WCHAR updateData[MAX_PATH + 1];
  if (!GetUpdateDirectoryPath(updateData)) {
    return FALSE;
  }

  // Get a unique filename
  WCHAR tempFilePath[MAX_PATH + 1];
  const int USE_SYSTEM_TIME = 0;
  if (!GetTempFileNameW(updateData, L"moz", USE_SYSTEM_TIME, tempFilePath)) {
    return FALSE;
  }
  
  const int FILE_SHARE_NONE = 0;
  nsAutoHandle updateMetaFile(CreateFileW(tempFilePath, GENERIC_WRITE, 
                                          FILE_SHARE_NONE, NULL, CREATE_ALWAYS, 
                                          0, NULL));
  if (updateMetaFile == INVALID_HANDLE_VALUE) {
    return FALSE;
  }

  // Write out the command line arguments that are passed to updater.exe
  PRUnichar *commandLineBuffer = MakeCommandLine(argc, argv);
  DWORD sessionID, sessionIDWrote;
  ProcessIdToSessionId(GetCurrentProcessId(), &sessionID);
  BOOL result = WriteFile(updateMetaFile, &sessionID, 
                          sizeof(DWORD), 
                          &sessionIDWrote, NULL);

  WCHAR appBuffer[MAX_PATH + 1];
  ZeroMemory(appBuffer, sizeof(appBuffer));
  wcscpy(appBuffer, exePath);
  DWORD appBufferWrote;
  result |= WriteFile(updateMetaFile, appBuffer, 
                      MAX_PATH * sizeof(WCHAR), 
                      &appBufferWrote, NULL);

  WCHAR workingDirectory[MAX_PATH + 1];
  ZeroMemory(workingDirectory, sizeof(appBuffer));
  GetCurrentDirectoryW(sizeof(workingDirectory) / sizeof(workingDirectory[0]), 
                       workingDirectory);
  DWORD workingDirectoryWrote;
  result |= WriteFile(updateMetaFile, workingDirectory, 
                      MAX_PATH * sizeof(WCHAR), 
                      &workingDirectoryWrote, NULL);

  DWORD commandLineLength = wcslen(commandLineBuffer) * sizeof(WCHAR);
  DWORD commandLineWrote;
  result |= WriteFile(updateMetaFile, commandLineBuffer, 
                      commandLineLength, 
                      &commandLineWrote, NULL);
  free(commandLineBuffer);
  if (!result ||
      sessionIDWrote != sizeof(DWORD) ||
      appBufferWrote != MAX_PATH * sizeof(WCHAR) ||
      workingDirectoryWrote != MAX_PATH * sizeof(WCHAR) ||
      commandLineWrote != commandLineLength) {
    updateMetaFile.reset();
    DeleteFileW(tempFilePath);
    return FALSE;
  }

  // Note we construct the 'service work' meta object with a .tmp extension,
  // When we want the service to start processing it we simply rename it to
  // have a .mz extension.  This ensures that the service will never try to
  // process a partial update work meta file. 
  updateMetaFile.reset();
  WCHAR completedMetaFilePath[MAX_PATH + 1];
  wcscpy(completedMetaFilePath, tempFilePath);

  // Change the file extension of the temp file path from .tmp to .mz
  LPWSTR extensionPart = 
    &(completedMetaFilePath[wcslen(completedMetaFilePath) - 3]);
  wcscpy(extensionPart, L"mz");
  return MoveFileExW(tempFilePath, completedMetaFilePath, 
                     MOVEFILE_REPLACE_EXISTING);
}

/**
 * Sets update.status to pending so that the next startup will not use
 * the service and instead will attempt an update the with a UAC prompt.
 *
 * @param  updateDirPath The path of the update directory
 * @return TRUE if successful
 */
BOOL
WriteStatusPending(LPCWSTR updateDirPath)
{
  WCHAR updateStatusFilePath[MAX_PATH + 1];
  wcscpy(updateStatusFilePath, updateDirPath);
  if (!PathAppendSafe(updateStatusFilePath, L"update.status")) {
    return FALSE;
  }

  const char pending[] = "pending";
  nsAutoHandle statusFile(CreateFileW(updateStatusFilePath, GENERIC_WRITE, 0, 
                                      NULL, CREATE_ALWAYS, 0, NULL));
  if (statusFile == INVALID_HANDLE_VALUE) {
    return FALSE;
  }

  DWORD wrote;
  BOOL ok = WriteFile(statusFile, pending, 
                      sizeof(pending) - 1, &wrote, NULL); 
  return ok && (wrote == sizeof(pending) - 1);
}

/**
 * Sets update.status to a specific failure code
 *
 * @param  updateDirPath The path of the update directory
 * @return TRUE if successful
 */
BOOL
WriteStatusFailure(LPCWSTR updateDirPath, int errorCode) 
{
  WCHAR updateStatusFilePath[MAX_PATH + 1];
  wcscpy(updateStatusFilePath, updateDirPath);
  if (!PathAppendSafe(updateStatusFilePath, L"update.status")) {
    return FALSE;
  }

  nsAutoHandle statusFile(CreateFileW(updateStatusFilePath, GENERIC_WRITE, 0, 
                                      NULL, CREATE_ALWAYS, 0, NULL));
  if (statusFile == INVALID_HANDLE_VALUE) {
    return FALSE;
  }
  char failure[32];
  sprintf(failure, "failed: %d", errorCode);

  DWORD toWrite = strlen(failure);
  DWORD wrote;
  BOOL ok = WriteFile(statusFile, failure, 
                      toWrite, &wrote, NULL); 
  return ok && wrote == toWrite;
}

/**
 * Launch a service initiated action with the specified arguments.
 *
 * @param  exePath The path of the executable to run
 * @param  argc    The total number of arguments in argv
 * @param  argv    An array of null terminated strings to pass to the exePath, 
 *         argv[0] is ignored
 * @return TRUE if successful
 */
BOOL
WinLaunchServiceCommand(const PRUnichar *exePath, int argc, char **argv)
{
  PRUnichar** argvConverted = new PRUnichar*[argc];
  if (!argvConverted)
    return FALSE;

  for (int i = 0; i < argc; ++i) {
    argvConverted[i] = AllocConvertUTF8toUTF16(argv[i]);
    if (!argvConverted[i]) {
      FreeAllocStrings(i, argvConverted);
      return FALSE;
    }
  }

  BOOL ok = WinLaunchServiceCommand(exePath, argc, argvConverted);
  FreeAllocStrings(argc, argvConverted);
  return ok;
}



/**
 * Launch a child process with the specified arguments.
 * @note argv[0] is ignored
 * @note The form of this function that takes char **argv expects UTF-8
 */

BOOL
WinLaunchChild(const PRUnichar *exePath, int argc, PRUnichar **argv);

BOOL
WinLaunchChild(const PRUnichar *exePath, int argc, char **argv)
{
  PRUnichar** argvConverted = new PRUnichar*[argc];
  if (!argvConverted)
    return FALSE;

  for (int i = 0; i < argc; ++i) {
    argvConverted[i] = AllocConvertUTF8toUTF16(argv[i]);
    if (!argvConverted[i]) {
      FreeAllocStrings(i, argvConverted);
      return FALSE;
    }
  }

  BOOL ok = WinLaunchChild(exePath, argc, argvConverted);
  FreeAllocStrings(argc, argvConverted);
  return ok;
}

BOOL
WinLaunchChild(const PRUnichar *exePath, int argc, PRUnichar **argv)
{
  PRUnichar *cl;
  BOOL ok;

  cl = MakeCommandLine(argc, argv);
  if (!cl)
    return FALSE;

  STARTUPINFOW si = {sizeof(si), 0};
  PROCESS_INFORMATION pi = {0};

  ok = CreateProcessW(exePath,
                      cl,
                      NULL,  // no special security attributes
                      NULL,  // no special thread attributes
                      FALSE, // don't inherit filehandles
                      0,     // No special process creation flags
                      NULL,  // inherit my environment
                      NULL,  // use my current directory
                      &si,
                      &pi);

  if (ok) {
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  } else {
    LPVOID lpMsgBuf = NULL;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMsgBuf,
                  0,
                  NULL);
    wprintf(L"Error restarting: %s\n", lpMsgBuf ? lpMsgBuf : L"(null)");
    if (lpMsgBuf)
      LocalFree(lpMsgBuf);
  }

  free(cl);

  return ok;
}

void
LaunchWinPostProcess(const WCHAR *appExe, HANDLE userToken = NULL)
{
  WCHAR workingDirectory[MAX_PATH + 1];
  wcscpy(workingDirectory, appExe);
  if (!PathRemoveFileSpecW(workingDirectory))
    return;

  // Launch helper.exe to perform post processing (e.g. registry and log file
  // modifications) for the update.
  WCHAR inifile[MAX_PATH + 1];
  if (!PathGetSiblingFilePath(inifile, appExe, L"updater.ini")) {
    return;
  }

  WCHAR exefile[MAX_PATH + 1];
  WCHAR exearg[MAX_PATH + 1];
  WCHAR exeasync[10];
  bool async = true;
  if (!GetPrivateProfileStringW(L"PostUpdateWin", L"ExeRelPath", NULL, exefile,
                                MAX_PATH + 1, inifile)) {
    return;
  }

  if (!GetPrivateProfileStringW(L"PostUpdateWin", L"ExeArg", NULL, exearg,
                                MAX_PATH + 1, inifile))
    return;

  if (!GetPrivateProfileStringW(L"PostUpdateWin", L"ExeAsync", L"TRUE", 
                                exeasync,
                                sizeof(exeasync)/sizeof(exeasync[0]), inifile))
    return;

  WCHAR exefullpath[MAX_PATH + 1];
  if (!PathGetSiblingFilePath(exefullpath, appExe, exefile)) {
    return;
  }

  WCHAR dlogFile[MAX_PATH + 1];
  if (!PathGetSiblingFilePath(dlogFile, exefullpath, L"uninstall.update")) {
    return;
  }

  WCHAR slogFile[MAX_PATH + 1];
  if (!PathGetSiblingFilePath(slogFile, appExe, L"update.log")) {
    return;
  }

  WCHAR dummyArg[14];
  wcscpy(dummyArg, L"argv0ignored ");

  size_t len = wcslen(exearg) + wcslen(dummyArg);
  WCHAR *cmdline = (WCHAR *) malloc((len + 1) * sizeof(WCHAR));
  if (!cmdline)
    return;

  wcscpy(cmdline, dummyArg);
  wcscat(cmdline, exearg);

  if (!_wcsnicmp(exeasync, L"false", 6) || 
      !_wcsnicmp(exeasync, L"0", 2))
    async = false;

  // We want to launch the post update helper app to update the Windows
  // registry even if there is a failure with removing the uninstall.update
  // file or copying the update.log file.
  CopyFileW(slogFile, dlogFile, false);

  STARTUPINFOW si = {sizeof(si), 0};
  si.lpDesktop = L"";
  PROCESS_INFORMATION pi = {0};

  bool ok;
  if (userToken) {
    ok = CreateProcessAsUserW(userToken,
                              exefullpath,
                              cmdline,
                              NULL,  // no special security attributes
                              NULL,  // no special thread attributes
                              false, // don't inherit filehandles
                              0,     // No special process creation flags
                              NULL,  // inherit my environment
                              workingDirectory,
                              &si,
                              &pi);
  } else {
    ok = CreateProcessW(exefullpath,
                        cmdline,
                        NULL,  // no special security attributes
                        NULL,  // no special thread attributes
                        false, // don't inherit filehandles
                        0,     // No special process creation flags
                        NULL,  // inherit my environment
                        workingDirectory,
                        &si,
                        &pi);
  }
  free(cmdline);
  if (ok) {
    if (!async)
      WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }
}

HANDLE
OpenUpdaterSignalEvent(const wchar_t *destinationPath, bool create)
{
  wchar_t sanitizedPath[MAX_PATH];
  wchar_t eventName[MAX_PATH];
  wcsncpy(sanitizedPath, destinationPath, MAX_PATH);
  // Replace \ with /
  for (wchar_t *p = sanitizedPath; p;) {
    wchar_t *backSlash = wcschr(p, L'\\');
    if (backSlash)
      *backSlash = L'/';
    p = backSlash;
  }
  // Convert to lower case
  _wcslwr(sanitizedPath);
  // Note that the path name might be trimmed to MAX_PATH, but it's OK.
  _snwprintf(eventName, MAX_PATH,
             L"Global\\{8cffe800-cf4c-48b6-a6ba-40ba46d06149}%s",
             sanitizedPath);

  HANDLE result;
  if (create) {
    // Set a DACL on the event so that its handle can be opened by everyone.
    PSID pSidEveryone;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld =
      SECURITY_WORLD_SID_AUTHORITY;
    if (!AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0, &pSidEveryone))
      return NULL;
    EXPLICIT_ACCESS ea = {
      EVENT_MODIFY_STATE,            // grfAccessPermissions 
      SET_ACCESS,                    // grfAccessMode
      NO_INHERITANCE,                // grfInheritance
      {                              // Trustee
        NULL,                        // pMultipleTrustee
        NO_MULTIPLE_TRUSTEE,         // MultipleTrusteeOperation
        TRUSTEE_IS_SID,              // TrusteeForm
        TRUSTEE_IS_WELL_KNOWN_GROUP, // TrusteeType
        (LPTSTR) pSidEveryone        // ptstrName
      }
    };
    PACL pAcl;
    if (ERROR_SUCCESS != SetEntriesInAcl(1, &ea, NULL, &pAcl))
      return NULL;
    SECURITY_DESCRIPTOR sd;
    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
      return NULL;
    if (!SetSecurityDescriptorDacl(&sd, true, pAcl, false))
      return NULL;
    SECURITY_ATTRIBUTES sa = {
      sizeof(SECURITY_ATTRIBUTES), // nLength
      &sd,                         // lpSecurityDescriptor
      false                        // bInheritHandle
    };
    result = CreateEventW(&sa, false, false, eventName);
  } else {
    result = OpenEventW(EVENT_MODIFY_STATE, false, eventName);
  }

  return result;
}
