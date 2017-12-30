///////////////////////////////////////////////////////////////////////////////////
// Main : WASAPI�Đ��T���v�� v1.00                                               //
//                                                                               //
// ���̃\�[�X�R�[�h�͎��R�ɉ��ς��Ďg�p�\�ł��B                                //
// �܂����p���p���\�ł����A���ׂĂ̊��Ő��������삷��ۏ�͂���܂���B      //
//                          http://www.charatsoft.com/                           //
///////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "CWasapi.h"
#include "CWindow.h"
#include "CWav.h"

#define DEBUGMODE				// �f�o�b�O�o�͂��Ȃ��ꍇ�̓R�����g������
#include "DEBUG.H"

// �}�N��
#define SAFE_RELEASE(x)		{ if( x ) { x->Release(); x=NULL; } }
#define SAFE_FREE(x)		{ if( x ) { free(x); x=NULL; } }


//////////////////////////////////////////////////////////////////////////////////////
// WASAPI�֘A
//////////////////////////////////////////////////////////////////////////////////////

// �w�b�_
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <FunctionDiscoveryKeys_devpkey.h>

// ���C�u����
#pragma comment(lib, "Avrt.lib")
#pragma comment(lib, "winmm.lib")

// �C���^�[�t�F�[�X��GUID�̎���(�v���W�F�N�g����C�t�@�C���ɕK��1�K�v)
const CLSID CLSID_MMDeviceEnumerator	= __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator		= __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient				= __uuidof(IAudioClient);
const IID IID_IAudioClock				= __uuidof(IAudioClock);
const IID IID_IAudioRenderClient		= __uuidof(IAudioRenderClient);

//////////////////////////////////////////////////////////////////////////////////////
// �Đ��X���b�h
//////////////////////////////////////////////////////////////////////////////////////
DWORD CALLBACK 
CWasapi::PlayThread( LPVOID param )
{
	DEBUG( "�X���b�h�J�n\n" );

	while( bThreadLoop ) {
		// ���̃o�b�t�@�擾���K�v�ɂȂ�܂őҋ@
		DWORD retval = WaitForSingleObject( hEvent,2000 );
		if( retval!=WAIT_OBJECT_0 ) {
			DEBUG( "�^�C���A�E�g\n" );
			pAudioClient->Stop();
			break;
		}

		// ����K�v�ȃt���[�������擾
		UINT32 frame_count;
		HRESULT ret = pAudioClient->GetBufferSize( &frame_count );
//		ODS( "�t���[����    : %d\n",frame_count );

		// �t���[��������o�b�t�@�T�C�Y���Z�o
		int buf_size = frame_count * iFrameSize;
//		ODS( "�o�b�t�@�T�C�Y : %dbyte\n",buf_size );


		// �o�̓o�b�t�@�̃|�C���^���擾
		LPBYTE dst;
		ret = pRenderClient->GetBuffer( frame_count,&dst );
		if( SUCCEEDED(ret) ) {
			// �\�[�X�o�b�t�@�̃|�C���^���擾
			LPBYTE src = wav.GetBuffer();

			// ���݂̃J�[�\�����玟�ɕK�v�ȃo�b�t�@�T�C�Y�����Z�����Ƃ���WAV�o�b�t�@�̃T�C�Y�𒴂��邩
			if( iAddr+buf_size>wav.GetBufferSize() ) {
				////////////////////////////////////////////////////////////////////////////////////////
				// ������ꍇ�͂܂����݂̃J�[�\������Ō�܂ł��R�s�[���A���Ɏc��������擪�����[����
				////////////////////////////////////////////////////////////////////////////////////////

				// WAV�o�b�t�@�T�C�Y���猻�݂̃J�[�\���ʒu���������������̂��Ō�܂ł̃o�b�t�@�T�C�Y
				int last_size = wav.GetBufferSize() - iAddr;

				// ���݂̃J�[�\������Ō�܂ł̃o�b�t�@���o�̓o�b�t�@�̐擪�ɃR�s�[����
				memcpy( &dst[0],&src[iAddr],last_size );

				// ����K�v�ȃT�C�Y���獡�R�s�[�����T�C�Y�����������̂��擪�����[����o�b�t�@�T�C�Y
				int begin_size = buf_size - last_size;

				// WAV�o�b�t�@�̐擪�����[�T�C�Y�����o�̓o�b�t�@�ɒǉ��R�s�[����
				memcpy( &dst[last_size],&src[0],begin_size );

				// ��[�������̃T�C�Y�����݂̃J�[�\���Ƃ���
				iAddr = begin_size;

				ODS( "LOOP OK\n" );
			} else {
				////////////////////////////////////////////////////////////////////////////////////////
				// �����Ȃ��ꍇ�͌��݂̃J�[�\������K�v�ȃo�b�t�@�����R�s�[���ăJ�[�\����i�߂�
				////////////////////////////////////////////////////////////////////////////////////////

				// WAV�o�b�t�@�̌��݂̃J�[�\������o�̓o�b�t�@�Ɏw��T�C�Y���R�s�[����
				memcpy( &dst[0],&src[iAddr],buf_size );

				// �J�[�\�����R�s�[�����������i�߂�
				iAddr += buf_size;
			}

			// �o�b�t�@���J��
			pRenderClient->ReleaseBuffer( frame_count,0 );
		}

	}

	DEBUG( "�X���b�h�I��\n" );
	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////
// WASAPI�̏�����
//////////////////////////////////////////////////////////////////////////////////////
BOOL 
CWasapi::InitWasapi( int latency )
{
	HRESULT ret;

	// �}���`���f�B�A�f�o�C�X�񋓎q
	ret = CoCreateInstance( CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pDeviceEnumerator );
	if( FAILED(ret) ) {
		DEBUG( "CLSID_MMDeviceEnumerator�G���[\n" );
		return FALSE;
	}

	// �f�t�H���g�̃f�o�C�X��I��
	ret = pDeviceEnumerator->GetDefaultAudioEndpoint( eRender,eConsole,&pDevice );
	if( FAILED(ret) ) {
		DEBUG( "GetDefaultAudioEndpoint�G���[\n" );
		return FALSE;
	}

	// �I�[�f�B�I�N���C�A���g
	ret = pDevice->Activate( IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient );
	if( FAILED(ret) ) {
		DEBUG( "�I�[�f�B�I�N���C�A���g�擾���s\n" );
		return FALSE;
	}

	// �t�H�[�}�b�g�̍\�z
	WAVEFORMATEXTENSIBLE wf;
	ZeroMemory( &wf,sizeof(wf) );
	wf.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE);
	wf.Format.wFormatTag			= WAVE_FORMAT_EXTENSIBLE;
	wf.Format.nChannels				= 2;
	wf.Format.nSamplesPerSec		= 44100;
	wf.Format.wBitsPerSample		= 16;
	wf.Format.nBlockAlign			= wf.Format.nChannels * wf.Format.wBitsPerSample / 8;
	wf.Format.nAvgBytesPerSec		= wf.Format.nSamplesPerSec * wf.Format.nBlockAlign;
	wf.Samples.wValidBitsPerSample	= wf.Format.wBitsPerSample;
	wf.dwChannelMask				= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
	wf.SubFormat					= KSDATAFORMAT_SUBTYPE_PCM;

	// 1�T���v���̃T�C�Y��ۑ�(16bit�X�e���I�Ȃ�4byte)
	iFrameSize = wf.Format.nBlockAlign;

	// �t�H�[�}�b�g�̃T�|�[�g�`�F�b�N
	ret = pAudioClient->IsFormatSupported( AUDCLNT_SHAREMODE_EXCLUSIVE,(WAVEFORMATEX*)&wf,NULL );
	if( FAILED(ret) ) {
		DEBUG( "���T�|�[�g�̃t�H�[�}�b�g\n" );
		return FALSE;
	}

	// ���C�e���V�ݒ�
	REFERENCE_TIME default_device_period = 0;
	REFERENCE_TIME minimum_device_period = 0;

	if( latency!=0 ) {
		default_device_period = (REFERENCE_TIME)latency * 10000LL;		// �f�t�H���g�f�o�C�X�s���I�h�Ƃ��ăZ�b�g
		DEBUG( "���C�e���V�w��             : %I64d (%f�~���b)\n",default_device_period,default_device_period/10000.0 );
	} else {
		ret = pAudioClient->GetDevicePeriod( &default_device_period,&minimum_device_period );
		DEBUG( "�f�t�H���g�f�o�C�X�s���I�h : %I64d (%f�~���b)\n",default_device_period,default_device_period/10000.0 );
		DEBUG( "�ŏ��f�o�C�X�s���I�h       : %I64d (%f�~���b)\n",minimum_device_period,minimum_device_period/10000.0 );
	}

	// ������
	UINT32 frame = 0;
	ret = pAudioClient->Initialize( AUDCLNT_SHAREMODE_EXCLUSIVE,
									AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
									default_device_period,				// �f�t�H���g�f�o�C�X�s���I�h�l���Z�b�g
									default_device_period,				// �f�t�H���g�f�o�C�X�s���I�h�l���Z�b�g
									(WAVEFORMATEX*)&wf,
									NULL ); 
	if( FAILED(ret) ) {
		if( ret==AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED ) {
			DEBUG( "�o�b�t�@�T�C�Y�A���C�����g�G���[�̂��ߏC������\n" );

			// �C����̃t���[�������擾
			ret = pAudioClient->GetBufferSize( &frame );
			DEBUG( "�C����̃t���[����         : %d\n",frame );
			default_device_period = (REFERENCE_TIME)( 10000.0 *						// (REFERENCE_TIME(100ns) / ms) *
													  1000 *						// (ms / s) *
													  frame /						// frames /
													  wf.Format.nSamplesPerSec +	// (frames / s)
													  0.5);							// �l�̌ܓ��H
			DEBUG( "�C����̃��C�e���V         : %I64d (%f�~���b)\n",default_device_period,default_device_period/10000.0  );

			// ��x�j�����ăI�[�f�B�I�N���C�A���g���Đ���
			SAFE_RELEASE( pAudioClient );
			ret = pDevice->Activate( IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient );
			if( FAILED(ret) ) {
				DEBUG( "�I�[�f�B�I�N���C�A���g�Ď擾���s\n" );
				return FALSE;
			}

			// �Ē���
			ret = pAudioClient->Initialize( AUDCLNT_SHAREMODE_EXCLUSIVE,
											AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
											default_device_period,
											default_device_period,
											(WAVEFORMATEX*)&wf,
											NULL );
		}

		if( FAILED(ret) ) {
			DEBUG( "���������s : 0x%08X\n",ret );
			switch( ret )
			{
			case AUDCLNT_E_NOT_INITIALIZED:					DEBUG( "AUDCLNT_E_NOT_INITIALIZED\n" );					break;
			case AUDCLNT_E_ALREADY_INITIALIZED:				DEBUG( "AUDCLNT_E_ALREADY_INITIALIZED\n" );				break;
			case AUDCLNT_E_WRONG_ENDPOINT_TYPE:				DEBUG( "AUDCLNT_E_WRONG_ENDPOINT_TYPE\n" );				break;
			case AUDCLNT_E_DEVICE_INVALIDATED:				DEBUG( "AUDCLNT_E_DEVICE_INVALIDATED\n" );				break;
			case AUDCLNT_E_NOT_STOPPED:						DEBUG( "AUDCLNT_E_NOT_STOPPED\n" );						break;
			case AUDCLNT_E_BUFFER_TOO_LARGE:				DEBUG( "AUDCLNT_E_BUFFER_TOO_LARGE\n" );				break;
			case AUDCLNT_E_OUT_OF_ORDER:					DEBUG( "AUDCLNT_E_OUT_OF_ORDER\n" );					break;
			case AUDCLNT_E_UNSUPPORTED_FORMAT:				DEBUG( "AUDCLNT_E_UNSUPPORTED_FORMAT\n" );				break;
			case AUDCLNT_E_INVALID_SIZE:					DEBUG( "AUDCLNT_E_INVALID_SIZE\n" );					break;
			case AUDCLNT_E_DEVICE_IN_USE:					DEBUG( "AUDCLNT_E_DEVICE_IN_USE\n" );					break;
			case AUDCLNT_E_BUFFER_OPERATION_PENDING:		DEBUG( "AUDCLNT_E_BUFFER_OPERATION_PENDING\n" );		break;
			case AUDCLNT_E_THREAD_NOT_REGISTERED:			DEBUG( "AUDCLNT_E_THREAD_NOT_REGISTERED\n" );			break;
//			case AUDCLNT_E_NO_SINGLE_PROCESS:				DEBUG( "AUDCLNT_E_NO_SINGLE_PROCESS\n" );				break;	// VC2010�ł͖���`�H
			case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED:		DEBUG( "AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED\n" );		break;
			case AUDCLNT_E_ENDPOINT_CREATE_FAILED:			DEBUG( "AUDCLNT_E_ENDPOINT_CREATE_FAILED\n" );			break;
			case AUDCLNT_E_SERVICE_NOT_RUNNING:				DEBUG( "AUDCLNT_E_SERVICE_NOT_RUNNING\n" );				break;
			case AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED:		DEBUG( "AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED\n" );		break;
			case AUDCLNT_E_EXCLUSIVE_MODE_ONLY:				DEBUG( "AUDCLNT_E_EXCLUSIVE_MODE_ONLY\n" );				break;
			case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL:	DEBUG( "AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL\n" );	break;
			case AUDCLNT_E_EVENTHANDLE_NOT_SET:				DEBUG( "AUDCLNT_E_EVENTHANDLE_NOT_SET\n" );				break;
			case AUDCLNT_E_INCORRECT_BUFFER_SIZE:			DEBUG( "AUDCLNT_E_INCORRECT_BUFFER_SIZE\n" );			break;
			case AUDCLNT_E_BUFFER_SIZE_ERROR:				DEBUG( "AUDCLNT_E_BUFFER_SIZE_ERROR\n" );				break;
			case AUDCLNT_E_CPUUSAGE_EXCEEDED:				DEBUG( "AUDCLNT_E_CPUUSAGE_EXCEEDED\n" );				break;
			default:										DEBUG( "UNKNOWN\n" );									break;
			}
			return FALSE;
		}
	}

	// �C�x���g����
	hEvent = CreateEvent( NULL,FALSE,FALSE,NULL );
	if( !hEvent ) {
		DEBUG( "�C�x���g�I�u�W�F�N�g�쐬���s\n" );
		return FALSE;
	}

	// �C�x���g�̃Z�b�g
	ret = pAudioClient->SetEventHandle( hEvent );
	if( FAILED(ret) ) {
		DEBUG( "�C�x���g�I�u�W�F�N�g�ݒ莸�s\n" );
		return FALSE;
	}

	// �����_���[�̎擾
	ret = pAudioClient->GetService( IID_IAudioRenderClient,(void**)&pRenderClient );
	if( FAILED(ret) ) {
		DEBUG( "�����_���[�擾�G���[\n" );
		return FALSE;
	}

	// WASAPI���擾
	ret = pAudioClient->GetBufferSize( &frame );
	DEBUG( "�ݒ肳�ꂽ�t���[����       : %d\n",frame );

	UINT32 size = frame * iFrameSize;
	DEBUG( "�ݒ肳�ꂽ�o�b�t�@�T�C�Y   : %dbyte\n",size );
	DEBUG( "1�T���v���̎���            : %f�b\n",(float)size/wf.Format.nSamplesPerSec );

	// �[���N���A�����ăC�x���g�����Z�b�g
	LPBYTE pData;
	ret = pRenderClient->GetBuffer( frame,&pData );
	if( SUCCEEDED(ret) ) {
		ZeroMemory( pData,size );
		pRenderClient->ReleaseBuffer( frame,0 );
	}

	// �X���b�h���[�v�t���O�𗧂Ă�
	bThreadLoop = TRUE;

	// �Đ��X���b�h�N��
	DWORD dwThread;
	hThread = CreateThread( NULL,0, CWasapi::PlayThread, NULL,0,&dwThread );
	if( !hThread ) {
		// ���s
		return FALSE;
	}

	// �Đ��J�n
	pAudioClient->Start();

	DEBUG( "WASAPI����������\n" );
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// WASAPI�̏I��
//////////////////////////////////////////////////////////////////////////////////////
void 
CWasapi::ExitWasapi()
{
	// �X���b�h���[�v�t���O�𗎂Ƃ�
	bThreadLoop = FALSE;

	// �X���b�h�I������
	if( hThread ) {
		// �X���b�h�����S�ɏI������܂őҋ@
		WaitForSingleObject( hThread,INFINITE );
		CloseHandle( hThread );
		hThread = NULL;
	}

	// �C�x���g���J������
	if( hEvent ) {
		CloseHandle( hEvent );
		hEvent = NULL;
	}

	// �C���^�[�t�F�[�X�J��
	SAFE_RELEASE( pRenderClient );
	SAFE_RELEASE( pAudioClient );
	SAFE_RELEASE( pDevice );
	SAFE_RELEASE( pDeviceEnumerator );

	DEBUG( "WASAPI�I��\n" );
}

#if(0)
//////////////////////////////////////////////////////////////////////////////////////
// ���C�����[�`��
//////////////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain( HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpszCmdParam,int nCmdShow )
{
	// �f�o�b�O��`
	INITDEBUG( "DEBUG.TXT" );
	CLEARDEBUG;

	// �E�B���h�E����
	if( !win.Create(hInstance,L"TestWasapi") ) {
		DEBUG( "�E�B���h�E�����G���[\n" );
		return -1;
	}

	// ���{����̖͂�����
	ImmAssociateContext( win.hWnd, NULL );

	// WAV�����[�h
	if( !wav.Load("loop.wav") ) {
		win.Delete();
		return -1;
	}

	// COM�̏�����
	CoInitialize( NULL );

	// WASAPI������
	if( !InitWasapi(0) ) {			// 0�Ȃ�f�t�H���g�f�o�C�X�s���I�h���g�p
		// �G���[�Ȃ�I������
		MessageBoxA( win.hWnd,"WASAPI���������s","�G���[",MB_OK|MB_ICONHAND );
		ExitWasapi();
		CoUninitialize();
		win.Delete();
		return -1;
	}

	// �E�B���h�E���[�v
	while(1) {
		MSG msg;
		int ret = GetMessage( &msg,NULL,0,0 );		// ���b�Z�[�W���o�^�����܂Ńu���b�N
		if( ret==0 || ret==-1 ) {
			// �I���R�[�h�Ȃ甲����
			break;
		}
		// ���b�Z�[�W����������
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	// WASAPI�I��
	ExitWasapi();

	// COM�̏I��
	CoUninitialize();

	// �E�B���h�E�폜
	win.Delete();

	return 0;
}
#endif