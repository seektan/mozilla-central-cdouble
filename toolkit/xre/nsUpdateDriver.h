/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Darin Fisher <darin@meer.net>
 *  Ben Turner <mozilla@songbirdnest.com>
 *  Robert Strong <robert.bugzilla@gmail.com>
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

#ifndef nsUpdateDriver_h__
#define nsUpdateDriver_h__

#include "nscore.h"
#ifdef MOZ_UPDATER
#include "nsIUpdateService.h"
#include "nsIThread.h"
#include "nsCOMPtr.h"
#endif

class nsIFile;

#if defined(XP_WIN)
  typedef HANDLE     ProcessType;
#elif defined(XP_MACOSX)
  typedef pid_t      ProcessType;
#else
#include "prproces.h"
  typedef PRProcess* ProcessType;
#endif

/**
 * This function processes any available updates.  As part of that process, it
 * may exit the current process and relaunch it at a later time.
 *
 * Two directories are passed to this function: greDir (where the actual
 * binary resides) and appDir (which contains application.ini for XULRunner
 * apps). If this is not a XULRunner app then appDir is identical to greDir.
 * 
 * The argc and argv passed to this function should be what is needed to
 * relaunch the current process.
 *
 * The appVersion param passed to this function is the current application's
 * version and is used to determine if an update's version is older than the
 * current application version.
 *
 * If you want the update to be processed without restarting, set the restart
 * parameter to false.
 *
 * This function does not modify appDir.
 */
NS_HIDDEN_(nsresult) ProcessUpdates(nsIFile *greDir, nsIFile *appDir,
                                    nsIFile *updRootDir,
                                    int argc, char **argv,
                                    const char *appVersion,
                                    bool restart = true,
                                    ProcessType *pid = nsnull);

#ifdef MOZ_UPDATER
// The implementation of the update processor handles the task of loading the
// updater application in the background for applying an update.
// XXX ehsan this is living in this file in order to make use of the existing
// stuff here, we might want to move it elsewhere in the future.
class nsUpdateProcessor : public nsIUpdateProcessor
{
public:
  nsUpdateProcessor();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIUPDATEPROCESSOR

private:
  void WaitForProcess();
  void UpdateDone();

private:
  ProcessType mUpdaterPID;
  nsCOMPtr<nsIThread> mProcessWatcher;
  nsCOMPtr<nsIUpdate> mUpdate;
};
#endif

#endif  // nsUpdateDriver_h__
