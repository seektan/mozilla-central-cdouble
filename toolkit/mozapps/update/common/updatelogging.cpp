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
 * The Original Code is common code between maintenanceservice and updater
 *
 * The Initial Developer of the Original Code is
 * Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#if defined(XP_WIN)
#include <windows.h>
#endif


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "updatelogging.h"

UpdateLog::UpdateLog() : logFP(NULL)
{
}

void UpdateLog::Init(NS_tchar* sourcePath, NS_tchar* fileName)
{
  if (logFP)
    return;

  this->sourcePath = sourcePath;
  NS_tchar logFile[MAXPATHLEN];
  NS_tsnprintf(logFile, sizeof(logFile)/sizeof(logFile[0]),
    NS_T("%s/%s"), sourcePath, fileName);

  logFP = NS_tfopen(logFile, NS_T("w"));
}

void UpdateLog::Finish()
{
  if (!logFP)
    return;

  fclose(logFP);
  logFP = NULL;
}

void UpdateLog::Flush()
{
  if (!logFP)
    return;

  fflush(logFP);
}


void UpdateLog::Printf(const char *fmt, ... )
{
  if (!logFP)
    return;

  va_list ap;
  va_start(ap, fmt);
  vfprintf(logFP, fmt, ap);
  va_end(ap);
}
