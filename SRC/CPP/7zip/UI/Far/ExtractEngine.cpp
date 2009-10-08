// ExtractEngine.h

#include "StdAfx.h"

#include <stdio.h>

#include "ExtractEngine.h"

#include "Common/Wildcard.h"
#include "Common/StringConvert.h"

#include "Windows/Defs.h"

#include "FarUtils.h"
#include "Messages.h"
#include "OverwriteDialog.h"

using namespace NWindows;
using namespace NFar;

extern void PrintMessage(const char *message);

CExtractCallBackImp::~CExtractCallBackImp()
{
}

void CExtractCallBackImp::Init(
    UINT codePage,
    CProgressBox *progressBox,
    bool passwordIsDefined,
    const UString &password)
{
  m_PasswordIsDefined = passwordIsDefined;
  m_Password = password;
  m_CodePage = codePage;
  m_ProgressBox = progressBox;
}

STDMETHODIMP CExtractCallBackImp::SetTotal(UInt64 size)
{
  _total = size;
  _totalIsDefined = true;
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::SetCompleted(const UInt64 *completeValue)
{
  if (WasEscPressed())
    return E_ABORT;
  _processedIsDefined = (completeValue != NULL);
  if (_processedIsDefined)
    _processed = *completeValue;
  if (m_ProgressBox != 0)
    m_ProgressBox->Progress(
      _totalIsDefined ? &_total: NULL,
      _processedIsDefined ? &_processed: NULL, m_message);
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::AskOverwrite(
    const wchar_t *existName, const FILETIME *existTime, const UInt64 *existSize,
    const wchar_t *newName, const FILETIME *newTime, const UInt64 *newSize,
    INT32 *answer)
{
  NOverwriteDialog::CFileInfo oldFileInfo, newFileInfo;
  oldFileInfo.Time = *existTime;
  oldFileInfo.SizeIsDefined = (existSize != NULL);
  if (oldFileInfo.SizeIsDefined)
    oldFileInfo.Size = *existSize;
  oldFileInfo.Name = GetSystemString(existName, m_CodePage);

  newFileInfo.TimeIsDefined = (newTime != 0);
  if (newFileInfo.TimeIsDefined)
    newFileInfo.Time = *newTime;
  
  newFileInfo.SizeIsDefined = (newSize != NULL);
  if (newFileInfo.SizeIsDefined)
    newFileInfo.Size = *newSize;
  newFileInfo.Name = GetSystemString(newName, m_CodePage);
  
  NOverwriteDialog::NResult::EEnum result =
    NOverwriteDialog::Execute(oldFileInfo, newFileInfo);
  
  switch(result)
  {
  case NOverwriteDialog::NResult::kCancel:
    // *answer = NOverwriteAnswer::kCancel;
    // break;
    return E_ABORT;
  case NOverwriteDialog::NResult::kNo:
    *answer = NOverwriteAnswer::kNo;
    break;
  case NOverwriteDialog::NResult::kNoToAll:
    *answer = NOverwriteAnswer::kNoToAll;
    break;
  case NOverwriteDialog::NResult::kYesToAll:
    *answer = NOverwriteAnswer::kYesToAll;
    break;
  case NOverwriteDialog::NResult::kYes:
    *answer = NOverwriteAnswer::kYes;
    break;
  case NOverwriteDialog::NResult::kAutoRename:
    *answer = NOverwriteAnswer::kAutoRename;
    break;
  default:
    return E_FAIL;
  }
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::PrepareOperation(const wchar_t *name, bool /* isFolder */, INT32 /* askExtractMode */, const UInt64 * /* position */)
{
  if (WasEscPressed())
    return E_ABORT;
  m_CurrentFilePath = name;
  m_message = GetSystemString(name, CP_OEMCP);
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::MessageError(const wchar_t *message)
{
  if (g_StartupInfo.ShowMessage(GetSystemString(message, CP_OEMCP)) == -1)
    return E_ABORT;
  return S_OK;
}

STDMETHODIMP CExtractCallBackImp::SetOperationResult(INT32 operationResult, bool encrypted)
{
  switch(operationResult)
  {
    case NArchive::NExtract::NOperationResult::kOK:
      break;
    default:
    {
      UINT idMessage;
      switch(operationResult)
      {
        case NArchive::NExtract::NOperationResult::kUnSupportedMethod:
          idMessage = NMessageID::kExtractUnsupportedMethod;
          break;
        case NArchive::NExtract::NOperationResult::kCRCError:
          idMessage = encrypted ?
            NMessageID::kExtractCRCFailedEncrypted :
            NMessageID::kExtractCRCFailed;
          break;
        case NArchive::NExtract::NOperationResult::kDataError:
          idMessage = encrypted ?
            NMessageID::kExtractDataErrorEncrypted :
            NMessageID::kExtractDataError;
          break;
        default:
          return E_FAIL;
      }
      farChar buffer[512];
      const CSysString s = GetSystemString(m_CurrentFilePath, m_CodePage);
      g_StartupInfo.m_FSF.sprintf(buffer, g_StartupInfo.GetMsgString(idMessage), (const farChar *)s);
      if (g_StartupInfo.ShowMessage(buffer) == -1)
        return E_ABORT;
    }
  }
  return S_OK;
}

extern HRESULT GetPassword(UString &password);

STDMETHODIMP CExtractCallBackImp::CryptoGetTextPassword(BSTR *password)
{
  if (!m_PasswordIsDefined)
  {
    RINOK(GetPassword(m_Password));
    m_PasswordIsDefined = true;
  }
  return StringToBstr(m_Password, password);
}