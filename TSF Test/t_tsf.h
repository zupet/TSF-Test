//
//

#if !defined(_T_TSF_H_)
#define _T_TSF_H_

#include <string>
#include <vector>
#include <msctf.h>

/*---------------------------------------------------------------------------*/ 
class TTsfTextStore : public ITextStoreACP, public ITfContextOwnerCompositionSink, public ITfUIElementSink 
{
public:
	TTsfTextStore();
	~TTsfTextStore();

	bool						InitTsfTextStore(ITfContext* context, TfEditCookie editCookie, HWND hWnd);
	void						ClearTsfTextStore();
	void						AddText(WCHAR* pchText, LONG cch);
	void						DeleteText();
	void						ClearText();
	void						ShowText(HDC hdc);
	void						MoveLeft(bool shift);
	void						MoveRight(bool shift);

	/* IUnknown Interfaces */ 
    HRESULT	STDMETHODCALLTYPE	QueryInterface(REFIID, LPVOID*);
    DWORD	STDMETHODCALLTYPE	AddRef();
    DWORD	STDMETHODCALLTYPE	Release();
   
	/* ITextStoreACP Interfaces */ 
    HRESULT	STDMETHODCALLTYPE 	AdviseSink(REFIID riid, IUnknown* punk, DWORD dwMask);
    HRESULT	STDMETHODCALLTYPE 	UnadviseSink(IUnknown* punk);
    HRESULT	STDMETHODCALLTYPE 	RequestLock(DWORD dwLockFlags, HRESULT* phrSession);
    HRESULT	STDMETHODCALLTYPE 	GetStatus(TS_STATUS* pdcs);
    HRESULT	STDMETHODCALLTYPE 	QueryInsert(LONG acpTestStart, LONG acpTestEnd, ULONG cch, LONG* pacpResultStart, LONG* pacpResultEnd);
    HRESULT	STDMETHODCALLTYPE 	GetSelection(ULONG ulIndex, ULONG ulCount, TS_SELECTION_ACP* pSelection, ULONG* pcFetched);
    HRESULT	STDMETHODCALLTYPE 	SetSelection(ULONG ulCount, const TS_SELECTION_ACP* pSelection);
    HRESULT	STDMETHODCALLTYPE 	GetText(LONG acpStart, LONG acpEnd, WCHAR* pchPlain, ULONG cchPlainReq, ULONG* pcchPlainOut, TS_RUNINFO* prgRunInfo, ULONG ulRunInfoReq, ULONG* pulRunInfoOut, LONG* pacpNext);
    HRESULT	STDMETHODCALLTYPE 	SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd, const WCHAR* pchText, ULONG cch, TS_TEXTCHANGE* pChange);
    HRESULT	STDMETHODCALLTYPE 	GetFormattedText(LONG acpStart, LONG acpEnd, IDataObject* *ppDataObject);
    HRESULT	STDMETHODCALLTYPE 	GetEmbedded(LONG acpPos, REFGUID rguidService, REFIID riid, IUnknown* *ppunk);
    HRESULT	STDMETHODCALLTYPE 	QueryInsertEmbedded(const GUID* pguidService, const FORMATETC* pFormatEtc, BOOL* pfInsertable);
    HRESULT	STDMETHODCALLTYPE 	InsertEmbedded(DWORD dwFlags, LONG acpStart, LONG acpEnd, IDataObject* pDataObject, TS_TEXTCHANGE* pChange);
    HRESULT	STDMETHODCALLTYPE 	RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs);
    HRESULT	STDMETHODCALLTYPE 	RequestAttrsAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags);
    HRESULT	STDMETHODCALLTYPE 	RequestAttrsTransitioningAtPosition(LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags);
    HRESULT	STDMETHODCALLTYPE 	FindNextAttrTransition(LONG acpStart, LONG acpHalt, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs, DWORD dwFlags, LONG* pacpNext, BOOL* pfFound, LONG* plFoundOffset);
    HRESULT	STDMETHODCALLTYPE 	RetrieveRequestedAttrs(ULONG ulCount, TS_ATTRVAL* paAttrVals, ULONG* pcFetched);
    HRESULT	STDMETHODCALLTYPE 	GetEndACP(LONG* pacp);
    HRESULT	STDMETHODCALLTYPE 	GetActiveView(TsViewCookie* pvcView);
    HRESULT	STDMETHODCALLTYPE 	GetACPFromPoint(TsViewCookie vcView, const POINT* pt, DWORD dwFlags, LONG* pacp);
    HRESULT	STDMETHODCALLTYPE 	GetTextExt(TsViewCookie vcView, LONG acpStart, LONG acpEnd, RECT* prc, BOOL* pfClipped);
    HRESULT	STDMETHODCALLTYPE 	GetScreenExt(TsViewCookie vcView, RECT* prc);
    HRESULT	STDMETHODCALLTYPE 	GetWnd(TsViewCookie vcView, HWND* phwnd);
    HRESULT	STDMETHODCALLTYPE 	InsertTextAtSelection(DWORD dwFlags, const WCHAR* pchText, ULONG cch, LONG* pacpStart, LONG* pacpEnd, TS_TEXTCHANGE* pChange);
    HRESULT	STDMETHODCALLTYPE 	InsertEmbeddedAtSelection(DWORD dwFlags, IDataObject* pDataObject, LONG* pacpStart, LONG* pacpEnd, TS_TEXTCHANGE* pChange);

	/* ITfContextOwnerCompositionSink */ 
	HRESULT STDMETHODCALLTYPE	OnStartComposition(ITfCompositionView *pComposition,BOOL *pfOk);
	HRESULT STDMETHODCALLTYPE	OnUpdateComposition(ITfCompositionView *pComposition, ITfRange *pRangeNew);
	HRESULT STDMETHODCALLTYPE	OnEndComposition(ITfCompositionView *pComposition);

	/* ITfUIElementSink */ 
	HRESULT	STDMETHODCALLTYPE	BeginUIElement(DWORD dwUIElementId, BOOL *pbShow);
	HRESULT	STDMETHODCALLTYPE	UpdateUIElement(DWORD dwUIElementId);
	HRESULT	STDMETHODCALLTYPE	EndUIElement(DWORD dwUIElementId);

protected:
	void						InsertSelection(LPCWSTR pchText, LONG cch, TS_TEXTCHANGE* pChange);
	void						DeleteSelection(TS_TEXTCHANGE* pChange);

	/* TTsfTextStore */ 
	DWORD						m_ref;
	BOOL						m_pendingLockUpgrade;
	DWORD						m_lockType;
	LONG						m_selectBegin;
	LONG						m_selectEnd;
	BOOL						m_interimChar;
	BOOL						m_layoutChanged;

	HWND						m_hWnd;

	/* TSF related */ 
	CComPtr<ITfContext>			m_context;

	TsViewCookie				m_viewCookie;
	TfEditCookie				m_editCookie;
	
	CComPtr<ITextStoreACPSink>	m_textStoreACPSink;
	CComPtr<IUnknown>			m_textStoreACPSinkOwner;
	DWORD						m_textStoreACPSinkMask;

	CComPtr<ITfProperty>		m_property;

	/* inputs */ 
	std::wstring				m_input;

	struct UnderLine
	{
		TF_DA_LINESTYLE lineStyle;
		BOOL			boldLine;
		ULONG			start;
		ULONG			end;
		COLORREF		bkColor;
		TF_DA_ATTR_INFO attr;
	};

	typedef	std::vector<UnderLine>		underline_list;
	typedef	underline_list::iterator	underline_list_iter;

	underline_list				m_underLineList;

	typedef	std::vector<std::wstring>	candidate_list;
	typedef	candidate_list::iterator	candidate_list_iter;
	
	candidate_list				m_candidateList;
	UINT						m_candidateSelection;

	std::wstring				m_reading;
};

/*---------------------------------------------------------------------------*/ 
class TTsfMgr
{
public:
	bool			InitTsfThreadMgr();
	void			ClearTextServiceMgr();

	BOOL			GetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
	void			SetFocus();

	TTsfTextStore*	CreateTsfTextStore(HWND hWnd);
	void			DestoryTsfTextStore(TTsfTextStore* textStore);

protected:
	CComPtr<ITfThreadMgr>				m_threadMgr;
	CComPtr<ITfDocumentMgr>				m_documentMgr;
	TfClientId							m_clientId;

    CComPtr<ITfMessagePump>				m_messagePump;
	CComPtr<ITfKeystrokeMgr>			m_keystrokeMgr;
};

/*---------------------------------------------------------------------------*/ 

#endif //_T_TSF_H_