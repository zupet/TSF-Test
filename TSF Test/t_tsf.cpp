//
//

#include "stdafx.h"
#include "t_tsf.h"

#include <ctffunc.h>

static CComPtr<ITfUIElementMgr>			g_uiElementMgr;
static CComPtr<ITfCategoryMgr>			g_categoryMgr;
static CComPtr<ITfDisplayAttributeMgr>	g_dispAttrMgr;

/*---------------------------------------------------------------------------*/ 
/*                                                                           */ 
/*---------------------------------------------------------------------------*/ 
TTsfTextStore::TTsfTextStore()
:m_ref(1)
,m_pendingLockUpgrade(FALSE)
,m_lockType(0)
,m_selectBegin(0)
,m_selectEnd(0)
,m_interimChar(FALSE)
,m_layoutChanged(FALSE)
,m_hWnd(NULL)
,m_viewCookie(GetTickCount())
{
}

/*---------------------------------------------------------------------------*/ 
TTsfTextStore::~TTsfTextStore()
{
	assert(m_context == NULL);
}

/*---------------------------------------------------------------------------*/ 
bool TTsfTextStore::InitTsfTextStore(ITfContext* context, TfEditCookie editCookie, HWND hWnd)
{
	m_context = context;
	m_editCookie = editCookie;
	m_hWnd = hWnd;

	CComPtr<ITfSource> source;
	if(SUCCEEDED(g_uiElementMgr->QueryInterface(IID_ITfSource, (LPVOID*)&source)))
	{
		DWORD uiElementSinkCookie;
		source->AdviseSink(IID_ITfUIElementSink, (ITfUIElementSink*)this, &uiElementSinkCookie);
	}

	m_context->GetProperty(GUID_PROP_ATTRIBUTE, &m_property);

	return true;
}

/*---------------------------------------------------------------------------*/ 
void TTsfTextStore::ClearTsfTextStore()
{
	m_context				= NULL;
	m_textStoreACPSink		= NULL;
	m_textStoreACPSinkOwner = NULL;
}

/*---------------------------------------------------------------------------*/ 
void TTsfTextStore::AddText(WCHAR* pchText, LONG cch)
{
	TS_TEXTCHANGE textChange;

	InsertSelection(pchText, cch, &textChange);

	m_selectBegin = m_selectBegin + cch;
	m_selectEnd = m_selectBegin;

	if(m_textStoreACPSink)
	{
		m_textStoreACPSink->OnTextChange(0, &textChange);
	}
}

/*---------------------------------------------------------------------------*/ 
void TTsfTextStore::DeleteText()
{
	if(m_selectBegin == m_selectEnd && m_selectBegin > 0)
	{
		m_selectEnd = m_selectBegin;
		m_selectBegin = m_selectBegin - 1;
	}

	TS_TEXTCHANGE textChange;

	DeleteSelection(&textChange);

	if(m_textStoreACPSink)
	{
		m_textStoreACPSink->OnTextChange(0, &textChange);
	}
}

/*---------------------------------------------------------------------------*/ 
void TTsfTextStore::ClearText()
{
	m_selectBegin = 0;
	m_selectEnd = (LONG)m_input.size();

	DeleteText();
}

/*---------------------------------------------------------------------------*/ 
void TTsfTextStore::ShowText(HDC hdc)
{
	/* Show Input */ 
	LPCWSTR text = m_input.c_str();
	LONG cch = (LONG)m_input.size();

	int x = 0;
	int y = 0;
	SIZE size;

	SetBkColor(hdc, 0xFFFFFF);
	SetTextColor(hdc, 0x000000);
	TextOut(hdc, x, y, text, m_selectBegin);
	GetTextExtentPoint32(hdc, text, m_selectBegin, &size);
	x += size.cx;

	/* Show Selection */ 
	SetBkColor(hdc, 0xFF8080);
	SetTextColor(hdc, 0xFFFFFF);
	TextOut(hdc, x, y, text+m_selectBegin, m_selectEnd-m_selectBegin);
	GetTextExtentPoint32(hdc, text+m_selectBegin, m_selectEnd-m_selectBegin, &size);
	x += size.cx;

	SetBkColor(hdc, 0xFFFFFF);
	SetTextColor(hdc, 0x000000);
	TextOut(hdc, x, y, text+m_selectEnd, cch-m_selectEnd);
	GetTextExtentPoint32(hdc, text+m_selectEnd, cch-m_selectEnd, &size);
	x += size.cx;

	if(m_selectBegin == m_selectEnd)
	{
		/* Show Cursor */ 
		SetBkMode(hdc, TRANSPARENT);
		SetBkColor(hdc, 0xFFFFFF);
		SetTextColor(hdc, 0x000000);

		GetTextExtentPoint32(hdc, text, m_selectBegin, &size);

		TextOut(hdc, size.cx-2, y, L"|", 1);

		SetBkMode(hdc, OPAQUE);
	}

	/* Show Reading */ 
	y += 20;

	SetBkColor(hdc, 0xFFFFFF);
	SetTextColor(hdc, 0x80FF80);
	TextOut(hdc, x, y, m_reading.c_str(), (LONG)m_reading.size());

	/* Show Candidate */ 
	y += 20;

	LONG i=0;
	for(candidate_list_iter iter=m_candidateList.begin(); iter!=m_candidateList.end(); ++iter, ++i)
	{
		if(i == m_candidateSelection)
		{
			SetBkColor(hdc, 0x0000FF);
			SetTextColor(hdc, 0xFFFFFF);
		}
		else
		{
			SetBkColor(hdc, 0x8080FF);
			SetTextColor(hdc, 0xFFFFFF);
		}

		LPCWSTR text = iter->c_str();
		LONG cch = (LONG)iter->size();

		TextOut(hdc, 0, y, text, cch);
		y += 15;		
	}

	/* Draw Underlines While Composition */ 
	for(underline_list_iter iter=m_underLineList.begin(); iter!=m_underLineList.end(); ++iter)
	{
		RECT rect;
		GetTextExtentPoint32(hdc, m_input.c_str(), iter->start, (LPSIZE)&rect.left);
		GetTextExtentPoint32(hdc, m_input.c_str(), iter->end, (LPSIZE)&rect.right);

		MoveToEx(hdc, rect.left, rect.bottom+2, NULL);

		HPEN pen, old;
		int width = iter->boldLine ? 3 : 1;

		COLORREF color = (iter->attr==TF_ATTR_TARGET_CONVERTED || iter->attr==TF_ATTR_TARGET_NOTCONVERTED) ? 0x0000FF : 0x000000;

		if(color)
		{
			pen = CreatePen(PS_SOLID, width, color);
			old = (HPEN)SelectObject(hdc, (HGDIOBJ)pen);
			LineTo(hdc, rect.right, rect.bottom+2);
			SelectObject(hdc, (HGDIOBJ)old);
			DeleteObject(pen);
		}
		//switch(iter->lineStyle)
		//{
		//case TF_LS_NONE:
		//	pen = CreatePen(PS_SOLID, width, color);
		//	old = (HPEN)SelectObject(hdc, (HGDIOBJ)pen);
		//	LineTo(hdc, rect.right, rect.bottom+2);
		//	SelectObject(hdc, (HGDIOBJ)old);
		//	DeleteObject(pen);
		//	break;
		//case TF_LS_SOLID:
		//	pen = CreatePen(PS_SOLID, width, color);
		//	old = (HPEN)SelectObject(hdc, (HGDIOBJ)pen);
		//	LineTo(hdc, rect.right, rect.bottom+2);
		//	SelectObject(hdc, (HGDIOBJ)old);
		//	DeleteObject(pen);
		//	break;
		//case TF_LS_DOT:
		//	pen = CreatePen(PS_DOT, width, color);
		//	old = (HPEN)SelectObject(hdc, (HGDIOBJ)pen);
		//	//LineTo(hdc, rect.right, rect.bottom+2);
		//	SelectObject(hdc, (HGDIOBJ)old);
		//	DeleteObject(pen);
		//	break;
		//case TF_LS_DASH:
		//	pen = CreatePen(PS_DASH, width, color);
		//	old = (HPEN)SelectObject(hdc, (HGDIOBJ)pen);
		//	//LineTo(hdc, rect.right, rect.bottom+2);
		//	SelectObject(hdc, (HGDIOBJ)old);
		//	DeleteObject(pen);
		//	break;
		//case TF_LS_SQUIGGLE:
		//	pen = CreatePen(PS_DASHDOT, width, color);
		//	old = (HPEN)SelectObject(hdc, (HGDIOBJ)pen);
		//	//LineTo(hdc, rect.right, rect.bottom+2);
		//	SelectObject(hdc, (HGDIOBJ)old);
		//	DeleteObject(pen);
		//	break;
		//default:
		//	break;
		//}

		GetTextExtentPoint32(hdc, m_input.c_str(), m_selectBegin, (LPSIZE)&rect.left);
		GetTextExtentPoint32(hdc, m_input.c_str(), m_selectEnd, (LPSIZE)&rect.right);

		Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
	}
}

/*---------------------------------------------------------------------------*/ 
void TTsfTextStore::MoveLeft(bool shift)
{
	if(m_selectBegin > 0)
	{
		m_selectBegin = m_selectBegin - 1;

		if(shift == false)
		{
			m_selectEnd = m_selectBegin;
		}
	}

	InvalidateRect(m_hWnd, NULL, TRUE);
}

/*---------------------------------------------------------------------------*/ 
void TTsfTextStore::MoveRight(bool shift)
{
	LONG cch = (LONG)m_input.size();

	if(m_selectEnd < cch)
	{
		m_selectEnd = m_selectEnd + 1;

		if(shift == false)
		{
			m_selectBegin = m_selectEnd;
		}
	}

	InvalidateRect(m_hWnd, NULL, TRUE);
}

/*---------------------------------------------------------------------------*/ /* IUnknown */ 
HRESULT TTsfTextStore::QueryInterface(REFIID riid, LPVOID* ppInterface)
{
    if(IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITextStoreACP))
    {
        AddRef();

        *ppInterface = (ITextStoreACP*)this;

		return S_OK;
	}
	else if(IsEqualIID(riid, IID_ITfUIElementSink))
	{
		AddRef();

		*ppInterface = (ITfUIElementSink*)this;
		
		return S_OK;
	}
	else if(IsEqualIID(riid, IID_ITfContextOwnerCompositionSink))
	{
		AddRef();

		*ppInterface = (ITfContextOwnerCompositionSink*)this;
		
		return S_OK;
	}
	else
	{
	    *ppInterface = NULL;

	    return E_NOINTERFACE;
	}
}

/*---------------------------------------------------------------------------*/ 
DWORD TTsfTextStore::AddRef()
{
	return ++m_ref;
}

/*---------------------------------------------------------------------------*/ 
DWORD TTsfTextStore::Release()
{
	return --m_ref;
}

/*---------------------------------------------------------------------------*/ /* ITextStoreACP */ 
HRESULT TTsfTextStore::AdviseSink(REFIID riid, IUnknown *punk, DWORD dwMask)
{
	CComPtr<IUnknown> punkID;
	if(SUCCEEDED(punk->QueryInterface(IID_IUnknown, (LPVOID*)&punkID)))
	{
		if(m_textStoreACPSinkOwner && punkID == m_textStoreACPSinkOwner)
		{
			m_textStoreACPSinkMask = dwMask;

			return S_OK;
		}
		else if(m_textStoreACPSink == NULL)
		{
			if(IsEqualIID(riid, IID_ITextStoreACPSink))
			{
				if(SUCCEEDED(punk->QueryInterface(IID_ITextStoreACPSink, (LPVOID*)&m_textStoreACPSink)))
				{
					m_textStoreACPSinkOwner	= punkID;
					m_textStoreACPSinkMask	= dwMask;
					
					return S_OK;
				}
				else
				{
					return E_INVALIDARG;
				}
			}
			else
			{
				return E_INVALIDARG;
			}
		}
		else
		{
			return CONNECT_E_ADVISELIMIT;
		}
	}
	else
	{
		return E_INVALIDARG;
	}	
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::UnadviseSink(IUnknown *punk)
{
    CComPtr<IUnknown> punkID;
    if(SUCCEEDED(punk->QueryInterface(IID_IUnknown, (LPVOID*)&punkID)))
    {
		if(punkID == m_textStoreACPSinkOwner && m_textStoreACPSink)
		{
			m_textStoreACPSinkOwner	= NULL;
			m_textStoreACPSink		= NULL;
			m_textStoreACPSinkMask	= 0;

			return S_OK;
		}
		else
		{
			return CONNECT_E_NOCONNECTION;
		}
    }
	else
	{
		return CONNECT_E_NOCONNECTION;
	}

}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::RequestLock(DWORD dwLockFlags, HRESULT *phrSession)
{
	if(m_textStoreACPSink)
	{
		if(phrSession)
		{
			if(m_lockType == 0)
			{
				m_lockType = dwLockFlags;
			    
				*phrSession = m_textStoreACPSink->OnLockGranted(dwLockFlags);

				m_lockType = 0;
			    
				if(m_pendingLockUpgrade)
				{
					m_pendingLockUpgrade = FALSE;

					HRESULT hr;
					RequestLock(TS_LF_READWRITE, &hr);
				}

				if(m_layoutChanged)
				{
					m_layoutChanged = FALSE;
					m_textStoreACPSink->OnLayoutChange(TS_LC_CHANGE, m_viewCookie);
				}

				return S_OK;
			}
			else if(dwLockFlags & TS_LF_SYNC)
			{
				*phrSession = TS_E_SYNCHRONOUS;

				return S_OK;
			}
			else if(((m_lockType & TS_LF_READWRITE) == TS_LF_READ) &&  ((dwLockFlags & TS_LF_READWRITE) == TS_LF_READWRITE))
			{
				m_pendingLockUpgrade = true;

				*phrSession = TS_S_ASYNC;

				return S_OK;
			}
			else
			{
				*phrSession = E_FAIL;

				return E_FAIL;
			}
		}
		else
		{
			return E_INVALIDARG;
		}
	}
	else
    {
        return E_UNEXPECTED;
    }
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::GetStatus(TS_STATUS *pdcs)
{
	if(pdcs)
	{
		pdcs->dwDynamicFlags = 0;
		pdcs->dwStaticFlags = TS_SS_REGIONS;

		return S_OK;	
	}
	else
	{
		return E_INVALIDARG;
	}
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::QueryInsert(LONG acpTestStart, LONG acpTestEnd, ULONG cch, LONG *pacpResultStart, LONG *pacpResultEnd)
{
	if(pacpResultStart && pacpResultEnd)
	{
		if((m_lockType & TS_LF_READ) == TS_LF_READ)
		{
			LONG lTextLength = (LONG)m_input.size();
			if(acpTestStart >= 0 && acpTestStart <= acpTestEnd && acpTestEnd <= lTextLength)
			{
				*pacpResultStart = acpTestStart;
				*pacpResultEnd = acpTestEnd;

				return S_OK;
			}
			else
			{
				return E_INVALIDARG;
			}
		}
		else
		{
			return TS_E_NOLOCK;
		}
	}
	else
	{
		return E_INVALIDARG;
	}
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP *pSelection, ULONG *pcFetched)
{
	if(pSelection && pcFetched)
	{
		if((m_lockType & TS_LF_READ) == TS_LF_READ)
		{
			if(ulIndex == TF_DEFAULT_SELECTION || ulIndex == 0)
			{
				*pcFetched = 1;

				if(ulCount != 0)
				{
					pSelection[0].acpStart = m_selectBegin;
					pSelection[0].acpEnd = m_selectEnd;
					pSelection[0].style.fInterimChar = m_interimChar;
					if(m_interimChar)
					{
						pSelection[0].style.ase = TS_AE_NONE;
					}
					else
					{
						pSelection[0].style.ase = TS_AE_END;
					}
				}

				return S_OK;
			}
			else
			{
				return E_INVALIDARG;
			}
		}
		else
		{
			return TS_E_NOLOCK;
		}
	}
	else
	{
        return E_INVALIDARG;
    }
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::SetSelection(ULONG ulCount, const TS_SELECTION_ACP *pSelection)
{
	if(pSelection)
	{
		if((m_lockType & TS_LF_READWRITE) == TS_LF_READWRITE)
		{
			if(ulCount == 1)
			{
				m_selectBegin	= pSelection[0].acpStart;
				m_selectEnd		= pSelection[0].acpEnd;
				m_interimChar	= pSelection[0].style.fInterimChar;

				return S_OK;
			}
			else
			{
				return E_INVALIDARG;
			}
		}
		else
		{
			return TS_E_NOLOCK;
		}
	}
	else
    {
        return E_INVALIDARG;
    }
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::GetText(LONG acpStart, LONG acpEnd, WCHAR *pchPlain, ULONG cchPlainReq, ULONG *pcchPlainRet, TS_RUNINFO *prgRunInfo, ULONG cRunInfoReq, ULONG *pcRunInfoRet, LONG *pacpNext)
{
	if((m_lockType & TS_LF_READ) == TS_LF_READ)
	{
		if(pacpNext && pcchPlainRet && pcRunInfoRet)
		{
			LONG lTextLength = (LONG)m_input.size();

			if(acpEnd == -1)
			{
				acpEnd = lTextLength;
			}

			if ((acpStart >= 0) && (acpStart <= acpEnd) && (acpEnd <= lTextLength))
			{
				ULONG cchReq = min(cchPlainReq, (ULONG)(acpEnd - acpStart));

				if(cchPlainReq>0 && pchPlain)
				{
					LPCWSTR pwszText = m_input.c_str();
					memcpy(pchPlain, pwszText+acpStart, sizeof(pwszText[0]) * cchReq);

					*pcchPlainRet = cchReq;
					*pacpNext = acpStart + cchReq;
				}
				else if(cchPlainReq == 0)
				{
					*pcchPlainRet = 0;
					*pacpNext = acpStart;
				}
				else
				{
					return E_INVALIDARG;		
				}

				if(cRunInfoReq>0 && prgRunInfo)
				{
					prgRunInfo[0].type = TS_RT_PLAIN;
					prgRunInfo[0].uCount = cchReq;
					*pcRunInfoRet = 1;
				}
				else if(cRunInfoReq == 0)
				{
					*pcRunInfoRet = 0;
				}
				else
				{
					return E_INVALIDARG;
				}

				return S_OK;
			}
			else
			{
				return TF_E_INVALIDPOS;
			}
		}
		else
		{
			return E_INVALIDARG;
		}
	}
	else
    {
        return TS_E_NOLOCK;
    }
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR *pchText, ULONG cch, TS_TEXTCHANGE *pChange)
{
    TS_SELECTION_ACP    tsa;
    tsa.acpStart = acpStart;
    tsa.acpEnd = acpEnd;
    tsa.style.ase = TS_AE_END;
    tsa.style.fInterimChar = FALSE;

	HRESULT hr = SetSelection(1, &tsa);
    if(SUCCEEDED(hr))
    {
		LONG acpStart, acpEnd;
        return InsertTextAtSelection(TS_IAS_NOQUERY, pchText, cch, &acpStart, &acpEnd, pChange);
    }
	else
	{
		return hr;
	}
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::GetFormattedText(LONG acpStart, LONG acpEnd, IDataObject **ppDataObject)
{
	return E_NOTIMPL;
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::GetEmbedded(LONG acpPos, REFGUID rguidService, REFIID riid, IUnknown **ppunk)
{
    return E_NOTIMPL;
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::QueryInsertEmbedded(const GUID *pguidService, const FORMATETC *pFormatEtc, BOOL *pfInsertable)
{
	if(pfInsertable)
	{
		*pfInsertable = FALSE;

		return S_OK;
	}
	else
	{
		return E_INVALIDARG;
	}
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd, IDataObject *pDataObject, TS_TEXTCHANGE *pChange)
{
	if(pDataObject)
	{
		return TS_E_FORMAT;
	}
	else
	{
		return E_INVALIDARG;
	}
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs)
{
	return E_NOTIMPL;
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags)
{
	return E_NOTIMPL;
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags)
{
    return E_NOTIMPL;
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::FindNextAttrTransition(LONG acpStart, LONG acpHalt, ULONG cFilterAttrs, const TS_ATTRID *paFilterAttrs, DWORD dwFlags, LONG *pacpNext, BOOL *pfFound, LONG *plFoundOffset)
{
    return E_NOTIMPL;
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL *paAttrVals, ULONG *pcFetched)
{
	return E_NOTIMPL;
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::GetEndACP(LONG *pacp)
{
	if((m_lockType & TS_LF_READ) == TS_LF_READ)
	{
		if(pacp)
		{
			*pacp = m_selectEnd;
		    
			return S_OK;
		}
		else
		{
			return E_INVALIDARG;
		}
	}
	else
    {
        return TS_E_NOLOCK;
    }
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::GetActiveView(TsViewCookie *pvcView)
{
	if(pvcView)
	{
		*pvcView = m_viewCookie;

		return S_OK;
	}
	else
	{
		return E_INVALIDARG;
	}
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::GetACPFromPoint(TsViewCookie vcView, const POINT *pt, DWORD dwFlags, LONG *pacp)
{
    return E_NOTIMPL;
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT *prc, BOOL *pfClipped)
{
	if(vcView == m_viewCookie)
	{
		if((m_lockType & TS_LF_READ) == TS_LF_READ)
		{
			if(acpStart != acpEnd)
			{
				if(prc && pfClipped)
				{
					LONG lTextLength = (LONG)m_input.size();

					if(acpEnd == -1)
					{
						acpEnd = lTextLength;
					}

					if(acpStart > acpEnd)
					{
						LONG lTemp = acpStart;
						acpStart = acpEnd;
						acpEnd = lTemp;
					}
				    
					LPCWSTR pwszText = m_input.c_str();

					HDC hdc = GetDC(m_hWnd);

					SIZE start, end;
					GetTextExtentPoint32(hdc, pwszText, acpStart, &start);
					GetTextExtentPoint32(hdc, pwszText+acpStart, acpEnd, &end);

					ReleaseDC(m_hWnd, hdc);

					SetRect(prc, start.cx, 0, start.cx + end.cx, end.cy);

					*pfClipped = FALSE;

					RECT        rc;
					GetClientRect(m_hWnd, &rc);

					if( (prc->left < rc.left) ||
						(prc->top < rc.top) ||
						(prc->right > rc.right) ||
						(prc->bottom > rc.bottom))
					{
						*pfClipped = TRUE;
					}

					//convert the rectangle to screen coordinates
					MapWindowPoints(m_hWnd, NULL, (LPPOINT)prc, 2);

					return S_OK;
				}
				else
				{
					return E_INVALIDARG;
				}
			}
			else
			{
				return E_INVALIDARG;
			}
		}
		else
		{
			return TS_E_NOLOCK;
		}
	}
	else
	{
		return E_INVALIDARG;
	}
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::GetScreenExt(TsViewCookie vcView, RECT *prc)
{
	if(vcView == m_viewCookie)
	{
		if(prc)
		{
			GetClientRect(m_hWnd, prc);

			MapWindowPoints(m_hWnd, NULL, (LPPOINT)prc, 2);

			return S_OK;
		}
		else
		{
			return E_INVALIDARG;
		}
	}
	else
	{
		return E_INVALIDARG;
	}
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::GetWnd(TsViewCookie vcView, HWND *phwnd)
{
    if(vcView == m_viewCookie)
    {
		if(phwnd)
		{
			*phwnd = m_hWnd;
		}

		return S_OK;
    }
	else
	{
	    return E_INVALIDARG;
	}
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::InsertTextAtSelection(DWORD dwFlags, const WCHAR *pchText, ULONG cch, LONG *pacpStart, LONG *pacpEnd, TS_TEXTCHANGE *pChange)
{
    if((m_lockType & TS_LF_READWRITE) == TS_LF_READWRITE)
	{
		if(dwFlags == 0)
		{
			if(pacpStart && pacpEnd && pChange)
			{
				InsertSelection(pchText, cch, pChange);

				*pacpStart = m_selectBegin;
				*pacpEnd = m_selectBegin + cch;

				return S_OK;
			}
			else
			{
				return E_INVALIDARG;
			}
		}
		else if(dwFlags & TF_IAS_NOQUERY)
		{
			if(pChange)
			{
				InsertSelection(pchText, cch, pChange);

				if(pacpStart && pacpEnd)
				{
					*pacpStart = m_selectBegin;
					*pacpEnd = m_selectBegin + cch;
				}

				return S_OK;
			}
			else
			{
				return E_INVALIDARG;
			}
		}
		else if(dwFlags & TS_IAS_QUERYONLY)
		{
			if(pacpStart && pacpEnd)
			{
				*pacpStart = m_selectBegin;
				*pacpEnd = m_selectBegin + cch;

				if(pChange)
				{
					pChange->acpStart = m_selectBegin;
					pChange->acpOldEnd = m_selectEnd;
					pChange->acpNewEnd = m_selectBegin + cch;
				}

				return S_OK;
			}
			else
			{
				return E_INVALIDARG;
			}
		}
		else
		{
			return E_INVALIDARG;
		}
	}
	else
    {
        return TS_E_NOLOCK;
    }
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::InsertEmbeddedAtSelection(DWORD dwFlags, IDataObject *pDataObject, LONG *pacpStart, LONG *pacpEnd, TS_TEXTCHANGE *pChange)
{
	return E_NOTIMPL;
}

/*---------------------------------------------------------------------------*/ /* ITfContextOwnerCompositionSink */ 
HRESULT TTsfTextStore::OnStartComposition(ITfCompositionView *pComposition,BOOL *pfOk)
{
	*pfOk = TRUE;

	return S_OK;
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew)
{
	CComPtr<ITfRange> compRange = pRangeNew;
	if(compRange == NULL)
	{
		if(FAILED(pComposition->GetRange(&compRange)) || compRange == NULL)
		{
			return S_OK;
		}
	}

	m_underLineList.clear();

	CComPtr<IEnumTfRanges> enumRange;
	if(SUCCEEDED(m_property->EnumRanges(m_editCookie, &enumRange, compRange)))
	{
		CComPtr<ITfRange> range;
		while(SUCCEEDED(enumRange->Next(1, &range, NULL)) && range)
		{
			VARIANT var;
			VariantInit(&var);
			if(SUCCEEDED(m_property->GetValue(m_editCookie, range, &var)))
			{
				GUID guid;
				if(VT_I4 == var.vt && SUCCEEDED(g_categoryMgr->GetGUID((TfGuidAtom)var.lVal, &guid)))
				{
					CComPtr<ITfDisplayAttributeInfo> pDispInfo;
					if(SUCCEEDED(g_dispAttrMgr->GetDisplayAttributeInfo(guid, &pDispInfo, NULL)))
					{
						TF_DISPLAYATTRIBUTE tfDisplayAttr;
						if(SUCCEEDED(pDispInfo->GetAttributeInfo(&tfDisplayAttr)))
						{
							LONG start, length;
							ITfRangeACP* rangeACP;
							if(SUCCEEDED(range->QueryInterface(IID_ITfRangeACP, (LPVOID*)&rangeACP)))
							{
								rangeACP->GetExtent(&start, &length);
								rangeACP->Release();
							}

							UnderLine underLine;
							underLine.boldLine = tfDisplayAttr.fBoldLine;
							underLine.lineStyle = tfDisplayAttr.lsStyle;
							underLine.start = start;
							underLine.end = start+length;
							underLine.bkColor = tfDisplayAttr.crBk.cr;
							underLine.attr = tfDisplayAttr.bAttr;
							m_underLineList.push_back(underLine);
						}
					}
				}
			}
			VariantClear(&var);
			range = NULL;
		}
	}

	InvalidateRect(m_hWnd, NULL, TRUE);

	return S_OK;
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::OnEndComposition(ITfCompositionView *pComposition)
{
	m_underLineList.clear();

	InvalidateRect(m_hWnd, NULL, TRUE);

	return S_OK;
}

/*---------------------------------------------------------------------------*/ /* ITfUIElementSink */ 
HRESULT TTsfTextStore::BeginUIElement(DWORD dwUIElementId, BOOL *pbShow)
{
	if(pbShow)
	{
		*pbShow = FALSE;

		return S_OK;
	}
	else
	{
		return E_INVALIDARG;
	}
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::UpdateUIElement(DWORD dwUIElementId)
{
	CComPtr<ITfUIElement> uiElement;
	if(SUCCEEDED(g_uiElementMgr->GetUIElement(dwUIElementId, &uiElement)))
	{
		CComPtr<ITfCandidateListUIElement> candidateListUiElement;
		CComPtr<ITfReadingInformationUIElement> readingInformationUiElement;
		if(SUCCEEDED(uiElement->QueryInterface(IID_ITfCandidateListUIElement, (LPVOID*)&candidateListUiElement)))
		{
			m_candidateList.clear();
			
			UINT count;
			if(SUCCEEDED(candidateListUiElement->GetCount(&count)))
			{
				for(UINT i=0; i<count; ++i)
				{
					CComBSTR candidate;
					if(SUCCEEDED(candidateListUiElement->GetString(i, &candidate)))
					{
						LPWSTR text = candidate;

						m_candidateList.push_back(text);
					}
				}
			}

			candidateListUiElement->GetSelection(&m_candidateSelection);
		}
		else if(SUCCEEDED(uiElement->QueryInterface(IID_ITfReadingInformationUIElement, (LPVOID*)&readingInformationUiElement)))
		{
			m_reading.clear();

			CComBSTR reading;
			if(SUCCEEDED(readingInformationUiElement->GetString(&reading)))
			{
				LPWSTR text = reading;

				m_reading.assign(text);
			}
		}

		InvalidateRect(m_hWnd, NULL, TRUE);
	}

	return S_OK;
}

/*---------------------------------------------------------------------------*/ 
HRESULT TTsfTextStore::EndUIElement(DWORD dwUIElementId)
{
	CComPtr<ITfUIElement> uiElement;
	if(SUCCEEDED(g_uiElementMgr->GetUIElement(dwUIElementId, &uiElement)))
	{
		CComPtr<ITfCandidateListUIElement> candidateListUiElement;
		CComPtr<ITfReadingInformationUIElement> readingInformationUiElement;
		if(SUCCEEDED(uiElement->QueryInterface(IID_ITfCandidateListUIElement, (LPVOID*)&candidateListUiElement)))
		{
			m_candidateList.clear();
		}
		else if(SUCCEEDED(uiElement->QueryInterface(IID_ITfReadingInformationUIElement, (LPVOID*)&readingInformationUiElement)))
		{
			m_reading.clear();
		}
		
		InvalidateRect(m_hWnd, NULL, TRUE);
	}

	return S_OK;
}

/*---------------------------------------------------------------------------*/ /* protected */ 
void TTsfTextStore::InsertSelection(LPCWSTR pchText, LONG cch, TS_TEXTCHANGE* pChange)
{
	m_input.erase(m_input.begin() + m_selectBegin, m_input.begin() + m_selectEnd);
	m_input.insert(m_input.begin() + m_selectBegin, pchText, pchText + cch);

	InvalidateRect(m_hWnd, NULL, TRUE);

	pChange->acpStart = m_selectBegin;
	pChange->acpOldEnd = m_selectEnd;
	pChange->acpNewEnd = m_selectBegin + cch;

	m_selectEnd = m_selectBegin + cch;
	m_layoutChanged = TRUE;
}

/*---------------------------------------------------------------------------*/ 
void TTsfTextStore::DeleteSelection(TS_TEXTCHANGE* pChange)
{
	m_input.erase(m_input.begin() + m_selectBegin, m_input.begin() + m_selectEnd);

	InvalidateRect(m_hWnd, NULL, TRUE);

	pChange->acpStart = m_selectBegin;
	pChange->acpOldEnd = m_selectEnd;
	pChange->acpNewEnd = m_selectBegin;

	m_selectEnd = m_selectBegin;
	m_layoutChanged = TRUE;
}

/*---------------------------------------------------------------------------*/ 
/*                                                                           */ 
/*---------------------------------------------------------------------------*/ 
bool TTsfMgr::InitTsfThreadMgr()
{
	CoInitialize(NULL);

	if(SUCCEEDED(CoCreateInstance(CLSID_TF_ThreadMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr, (LPVOID*)&m_threadMgr)))
	{
		if(SUCCEEDED(m_threadMgr->QueryInterface(IID_ITfMessagePump, (void **)&m_messagePump)))
		{
			if(SUCCEEDED(m_threadMgr->QueryInterface(IID_ITfKeystrokeMgr, (LPVOID*)&m_keystrokeMgr)))
			{
				if(SUCCEEDED(m_threadMgr->CreateDocumentMgr(&m_documentMgr)))
				{
					m_threadMgr->SetFocus(m_documentMgr);

					CComPtr<ITfThreadMgrEx> g_threadMgrEx;
					if(SUCCEEDED(m_threadMgr->QueryInterface(IID_ITfThreadMgrEx, (LPVOID*)&g_threadMgrEx)))
					{
						g_threadMgrEx->ActivateEx(&m_clientId, TF_TMAE_UIELEMENTENABLEDONLY);
					}
					else
					{
						m_threadMgr->Activate(&m_clientId);
					}
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	}
	
	if(FAILED(m_threadMgr->QueryInterface(IID_ITfUIElementMgr, (LPVOID*)&g_uiElementMgr)))
	{
		return false;
	}

	if(FAILED(CoCreateInstance(CLSID_TF_CategoryMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (LPVOID*)&g_categoryMgr)))
    {
        return false;
    }

    if(FAILED(CoCreateInstance(CLSID_TF_DisplayAttributeMgr, NULL, CLSCTX_INPROC_SERVER, IID_ITfDisplayAttributeMgr, (LPVOID*)&g_dispAttrMgr)))
    {
        return false;
    }

	return true;
}

/*---------------------------------------------------------------------------*/ 
void TTsfMgr::ClearTextServiceMgr()
{
	m_keystrokeMgr = NULL;
	m_messagePump = NULL;
	m_documentMgr = NULL;

	if(m_threadMgr)
	{
		m_threadMgr->Deactivate();
		m_threadMgr = NULL;
	}

	g_uiElementMgr = NULL;
	g_categoryMgr = NULL;
	g_dispAttrMgr = NULL;

	CoUninitialize();
}

/*---------------------------------------------------------------------------*/ 
BOOL TTsfMgr::GetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
	BOOL result;
	if(SUCCEEDED(m_messagePump->GetMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, &result)) && result)
	{
		BOOL eaten;

		if(WM_KEYDOWN == lpMsg->message)
		{
			if (m_keystrokeMgr->TestKeyDown(lpMsg->wParam, lpMsg->lParam, &eaten) == S_OK && eaten &&
				m_keystrokeMgr->KeyDown(lpMsg->wParam, lpMsg->lParam, &eaten) == S_OK && eaten)
			{
				return GetMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
			}
		}
		else if(WM_KEYUP == lpMsg->message)
		{
			if (m_keystrokeMgr->TestKeyUp(lpMsg->wParam, lpMsg->lParam, &eaten) == S_OK && eaten &&
				m_keystrokeMgr->KeyUp(lpMsg->wParam, lpMsg->lParam, &eaten) == S_OK && eaten)
			{
				return GetMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
			}
		}

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*---------------------------------------------------------------------------*/ 
void TTsfMgr::SetFocus()
{
	if(m_threadMgr)
	{
		m_threadMgr->SetFocus(m_documentMgr);
	}
}

/*---------------------------------------------------------------------------*/ 
TTsfTextStore* TTsfMgr::CreateTsfTextStore(HWND hWnd)
{
	TTsfTextStore* textStore = new TTsfTextStore();

	CComPtr<ITfContext> context;
	TfEditCookie editCookie;
	if(SUCCEEDED(m_documentMgr->CreateContext(m_clientId, 0, (ITextStoreACP*)textStore,  &context, &editCookie)))
	{
		textStore->InitTsfTextStore(context, editCookie, hWnd);

		m_documentMgr->Push(context);
	}

	return textStore;	
}

/*---------------------------------------------------------------------------*/ 
void TTsfMgr::DestoryTsfTextStore(TTsfTextStore* textStore)
{
	textStore->ClearTsfTextStore();

	delete textStore;
}

/*---------------------------------------------------------------------------*/ 
