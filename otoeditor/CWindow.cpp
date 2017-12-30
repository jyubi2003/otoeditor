#pragma warning( disable : 4996 )
#include "CWindow.h"

//#define DEBUGMODE
#include "DEBUG.H"

///////////////////////////////////////////////////////////////////////////////////
// �X�^�e�B�b�N�ϐ�
///////////////////////////////////////////////////////////////////////////////////
HINSTANCE			CWindow::hInstance		= NULL;
HWND				CWindow::hWnd			= NULL;
BOOL				CWindow::bActive		= TRUE;

WCHAR				CWindow::mName[256]		= L"";
const WCHAR*		CWindow::cIconID		= IDI_APPLICATION;		// �f�t�H���g�̃A�C�R��
HMENU				CWindow::hMenu			= NULL;
DWORD				CWindow::dwStyle		= WS_POPUP|WS_SYSMENU|WS_CAPTION|WS_MINIMIZEBOX;
DWORD				CWindow::dwExStyle		= 0;

LPONMSG				CWindow::mMsg			= NULL;
int					CWindow::iMsg			= 0;


///////////////////////////////////////////////////////////////////////////////////
// �R���X�g���N�^
///////////////////////////////////////////////////////////////////////////////////
CWindow::CWindow(void)
{
}

///////////////////////////////////////////////////////////////////////////////////
// �f�X�g���N�^
///////////////////////////////////////////////////////////////////////////////////
CWindow::~CWindow()
{
}

///////////////////////////////////////////////////////////////////////////////////
// ���C���E�C���h�E�̃C�x���g�n���h��
///////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK CWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CWindow *win = NULL;

	switch( uMsg )
	{
	case WM_CREATE:
		win = (CWindow*)lParam;
		break;
	case WM_ACTIVATE:
		bActive = LOWORD(wParam)?TRUE:FALSE;				// �A�N�e�B�u��ԕύX
		break;
	case WM_DESTROY:										// ALT+F4�������ꂽ��
		PostQuitMessage( 0 );
		break;
	case WM_MOUSEMOVE:
		break;
	case WM_SYSCOMMAND:
		switch( wParam )
		{
		case SC_CLOSE:
			PostQuitMessage(0);
			return 0;
		}
		break;
	case WM_IME_NOTIFY:
		switch( wParam )
		{
		case IMN_SETOPENSTATUS:
			HIMC hImc = ImmGetContext( hWnd );
            ImmSetOpenStatus( hImc,FALSE );
			break;
		}
		break;
    }

	// ���ꃁ�b�Z�[�W����
	int i;
	for( i=0;i<iMsg;i++ ) {
		if( uMsg==mMsg[i].uiMsg ) {
			return mMsg[i].cmdProc( hWnd,wParam,lParam );	// ���ꃁ�b�Z�[�W���슮���Ȃ�
		}
	}

	return DefWindowProc( hWnd,uMsg,wParam,lParam );		// �f�t�H���g��Ԃ�
}

///////////////////////////////////////////////////////////////////////////////////
// �E�C���h�E�𐶐�����
///////////////////////////////////////////////////////////////////////////////////
BOOL CWindow::Create( HINSTANCE hInst,const WCHAR *appName,BOOL show,DWORD w,DWORD h,HWND parent )
{
	WNDCLASS wc;
	DEVMODE dmMode;

	// ��ʉ𑜓x���`�F�b�N
	EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&dmMode);
	// 16bit�ȏ�̉𑜓x����Ȃ��ƋN���ł��Ȃ�
	if( dmMode.dmBitsPerPel<16 ) {
		MessageBoxW( GetDesktopWindow(),L"16Bit�ȏ�̉𑜓x�ɂ��Ă�������",L"�N���ł��܂���",MB_OK );
		return FALSE;
	}

	// �Z�b�g
	hInstance = hInst;
	wcscpy( mName,appName );

	// �E�C���h�E�N���X�o�^
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNCLIENT;
    wc.lpfnWndProc		= WindowProc;
    wc.cbClsExtra		= 0;
    wc.cbWndExtra		= sizeof(DWORD);
    wc.hInstance		= hInstance;
    wc.hIcon			= LoadIcon(hInstance, cIconID );
    wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName		= MAKEINTRESOURCE( hMenu );
    wc.lpszClassName	= mName;
    if ( !RegisterClass(&wc) )
        return FALSE;

	// �E�C���h�E����
    hWnd = CreateWindowExW(
		dwExStyle,
		wc.lpszClassName,		// Class
		mName,					// Title bar
		dwStyle,				// Style
		GetSystemMetrics(SM_CXSCREEN)/2-w/2,
		GetSystemMetrics(SM_CYSCREEN)/2-h/2,
		w,						// Init. x pos
		h,						// Init. y pos
		parent,					// Parent window
		NULL,					// Menu handle
		hInstance,				// Program handle
		this					// Create parms
	);
    if( !hWnd )
        return FALSE;			// �����Ɏ��s

	// �t�H���g�̐ݒ�
	HDC hdc = GetDC( hWnd );
	if( hdc ) {
		SetBkMode( hdc,TRANSPARENT );
		ReleaseDC( hWnd,hdc );
	}

	MoveClientWindowCenter( w,h );
	// �E�C���h�E��\��
	if( show )
		::ShowWindow( hWnd,SW_SHOW );

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////
// �����I�ɃE�B���h�E���폜����
///////////////////////////////////////////////////////////////////////////////////
void CWindow::Delete( void )
{
	if( hWnd ) {
		// �ʏ�̃E�B���h�E�Ȃ�
		::DestroyWindow( hWnd );
		// �o�^�����N���X��������
		UnregisterClassW( mName,hInstance );
		ZeroMemory( &mName,sizeof(mName) );
		hWnd = NULL;
		hInstance = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////////
// �J�[�\���̕\���E��\��
///////////////////////////////////////////////////////////////////////////////////
void CWindow::ShowCursor( BOOL bShow )
{
	if( bShow )
		while(::ShowCursor(TRUE)<0);
	else
		while(::ShowCursor(FALSE)>=0);
}

///////////////////////////////////////////////////////////////////////////////////
// �E�C���h�E�̕\���E��\��
///////////////////////////////////////////////////////////////////////////////////
void CWindow::ShowWindow( BOOL bShow )
{
	// �E�C���h�E�̕\��
	if( bShow )
		::ShowWindow( hWnd,SW_SHOW );
	else
		::ShowWindow( hWnd,SW_HIDE );
}

///////////////////////////////////////////////////////////////////////////////////
// �A�v���P�[�V�����̃A�C�R���̕ύX
///////////////////////////////////////////////////////////////////////////////////
void CWindow::SetIcon( const WCHAR *icon )
{
	cIconID = icon;
}


// ���ꃁ�b�Z�[�W�̒ǉ�
BOOL CWindow::AddMsgProc( UINT msg,ONCOMMAND proc )
{
	int i;
	// ���ɑ��݂��Ă��Ȃ����`�F�b�N
	for( i=0;i<iMsg;i++ ) {
		if( mMsg[i].uiMsg==msg ) {
			// ����ΐV�����A�h���X�ɍX�V
			mMsg[i].cmdProc = proc;
			return TRUE;
		}
	}

	// �ǉ�
	iMsg++;
	mMsg = (LPONMSG)realloc( mMsg,sizeof(ONMSG)*iMsg );
	ZeroMemory( &mMsg[iMsg-1],sizeof(ONMSG) );
	mMsg[iMsg-1].uiMsg = msg;
	mMsg[iMsg-1].cmdProc = proc;
	return TRUE;
}

// �E�C���h�E�X�^�C���̕ύX(���I�ɕύX���\)
BOOL CWindow::SetWindowStyle( DWORD style )
{
	dwStyle = style;
	if( hWnd ) {
		// ���łɃE�C���h�E�����݂���ꍇ�͑����f
		::SetWindowLong( hWnd,GWL_STYLE,style );
		::SetWindowPos( hWnd,0,0,0,0,0,SWP_FRAMECHANGED|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER );
	}
	return TRUE;
}

// �E�C���h�E�̈ړ�
void CWindow::Move( int x,int y )
{
	SetWindowPos( hWnd,0,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER );
}

// �E�C���h�E�̈ړ�(���ƍ����������ɕύX)
void CWindow::Move( int x,int y,int w,int h )
{
	MoveWindow( hWnd,x,y,w,h,TRUE );
}

// �w��T�C�Y���N���C�A���g�̈�ɂȂ�悤�ɃE�C���h�E��z�u
BOOL CWindow::MoveClientWindowCenter( int w,int h )
{
	RECT Win,Cli;
	GetWindowRect( hWnd,&Win );								// �E�C���h�E�̍�����擾
	GetClientRect( hWnd,&Cli );								// �E�C���h�E���̃N���C�A���g���W���擾
	int frame_w = (Win.right - Win.left) - Cli.right;		// �t���[���̕�
	int frame_h = (Win.bottom - Win.top) - Cli.bottom;		// �t���[���̍���
	int scr_w	= GetSystemMetrics( SM_CXSCREEN );			// �X�N���[���̕�
	int scr_h	= GetSystemMetrics( SM_CYSCREEN );			// �X�N���[���̍���
	SetWindowPos( hWnd,0,( scr_w - (frame_w/2+w) ) / 2,( scr_h - (frame_h/2+h) ) / 2,w+frame_w,h+frame_h,SWP_NOZORDER );

	return TRUE;
}

// ���j���[�A�C�e���̕ύX
BOOL CWindow::SetMenuItem(int menuid,BOOL check,BOOL gray )
{
	HMENU menu = GetMenu( hWnd );
	if( menu ) {
		MENUITEMINFO miinfo;
		ZeroMemory( &miinfo,sizeof(miinfo) );
		miinfo.cbSize = sizeof(miinfo);
		miinfo.fMask = MIIM_STATE;
		if( check )
			miinfo.fState |= MFS_CHECKED;
		else
			miinfo.fState |= MFS_UNCHECKED;
		if( gray )
			miinfo.fState |= MFS_GRAYED;
		else
			miinfo.fState |= MFS_ENABLED;
		return SetMenuItemInfo( menu,menuid,FALSE,&miinfo );
	}
	return TRUE;
}

BOOL CWindow::TextOutW( int x,int y,const WCHAR *str,COLORREF col )
{
	HDC hdc = GetDC( hWnd );
	if( hdc ) {
		SetTextColor( hdc,col );
		::TextOutW( hdc,x,y,str,wcslen(str) );
		ReleaseDC( hWnd,hdc );
	}
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////
// ���j���[�o�[�̕ύX(Create�̑O�ɕK�v�j
///////////////////////////////////////////////////////////////////////////////////
void CWindow::SetMenu( HMENU menu )
{
	hMenu = menu;
}
