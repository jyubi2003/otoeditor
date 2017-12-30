#pragma warning( disable : 4996 )

#include "stdafx.h"
#include "CWav.h"
#include <io.h>
#include <fcntl.h>

//#define DEBUGMODE
#include "DEBUG.H"

#define FREE(x)		{ if(x) free(x); x=NULL; }

CWav::CWav()
{
	fp = NULL;
	mBuf = NULL;
	iBuf = 0;
	iStreamPoint = 0;
	dwMode = WAVMODE_STATIC;
	ZeroMemory( &mHead,sizeof(mHead) );
}


CWav::~CWav()
{
	Close();
	FREE( mBuf );
}

BOOL CWav::Load( const char *file,DWORD mode )
{
	Close();

	fp = fopen( file,"rb" );
	if( !fp )
		return FALSE;

	fread( &mHead,sizeof(WAVHEADER),1,fp );
	if( strnicmp(mHead.mRiff,"RIFF",4)!=0 ) {
		DEBUG( "RIFF�t�H�[�}�b�g�G���[\n","" );
		return FALSE;
	}

	if( strnicmp(mHead.mWave,"WAVE",4)!=0 ) {
		DEBUG( "WAV�`�����N��������Ȃ�\n","" );
		return FALSE;
	}

	if( strnicmp(mHead.mFmt,"fmt ",4)!=0 ) {
		DEBUG( "�t�H�[�}�b�g�`�����N��������Ȃ�\n","" );
		return FALSE;
	}


	DEBUG( "�t�H�[�}�b�gID     = %d\n",mHead.wFormatID );
	DEBUG( "�`�����N�T�C�Y     = %d\n",mHead.dwChuncSize );
	DEBUG( "�`�����l����       = %d\n",mHead.wChannel );
	DEBUG( "�r�b�g��           = %d\n",mHead.wBitrate );
	DEBUG( "�T���v�����O���g�� = %d\n",mHead.dwSample );

	// data�`�����N��T��
	DATACHUNC data;
	while(1) {
		// EOF�Ȃ�I��
		if( feof(fp) ) {
			DEBUG( "data�`�����N��������Ȃ�����\n","" );
			fclose( fp );
			return FALSE;
		}
		fread( &data,sizeof(DATACHUNC),1,fp );

		// data�`�����N�Ȃ甲����
		if( strnicmp(data.mData,"data",4)==0 )
			break;
		// ���̃`�����N��
		if( !fseek( fp,data.dwSize,SEEK_CUR ) ) {
			DEBUG( "seek�G���[\n","" );
			fclose( fp );
			return FALSE;
		}
	}

	// �f�[�^�T�C�Y�̎擾
	iBuf = data.dwSize;
	DEBUG( "�f�[�^�T�C�Y       = %d\n",iBuf );

	// ���[�h�L��
	dwMode = mode;

	switch( dwMode )
	{
	case WAVMODE_STATIC:
		mBuf = (LPBYTE)realloc( mBuf,iBuf );
		if( !mBuf ) {
			DEBUG( "�������m�ۃG���[\n","" );
			return FALSE;
		}

		fread( mBuf,iBuf,1,fp );
		break;
	}
	return TRUE;
}

BOOL CWav::Close( void )
{
	if( fp ) {
		fclose( fp );
		fp = NULL;
		DEBUG( "CLOSE OK\n","" );
	}
	dwMode = WAVMODE_STATIC;
	iStreamPoint = 0;
	return TRUE;
}

BOOL CWav::Create( const char *file,int ch,int bit,int samp )
{
	Close();

	iBuf = 0;
	memcpy( &mHead.mRiff,"RIFF",4 );
	mHead.dwFileSize = sizeof(WAVHEADER) + sizeof(DATACHUNC);
	memcpy( &mHead.mWave,"WAVE",4 );
	memcpy( &mHead.mFmt,"fmt ",4 );
	mHead.dwChuncSize = 16;		// 16�Œ�
	mHead.wFormatID = 1;
	mHead.wChannel = ch;
	mHead.dwSample = samp;
	mHead.wBitrate = bit;
	mHead.dwBytePerSec = mHead.wChannel * mHead.dwSample * mHead.wBitrate / 8;
	mHead.wBlockSize = mHead.wChannel * mHead.wBitrate / 8;

	memcpy( &mData.mData,"data",4 );
	mData.dwSize = 0;

	strcpy( mFile,file );
	fp = fopen( mFile,"wb" );
	if( !fp )
		return FALSE;

	fwrite( &mHead,sizeof(WAVHEADER),1,fp );
	fwrite( &mData,sizeof(DATACHUNC),1,fp );

	return TRUE;
}

BOOL CWav::Write( const LPVOID buf,int size )
{
	if( dwMode!=WAVMODE_STATIC )
		return FALSE;

/*	mBuf = (LPBYTE)realloc( mBuf,iBuf+size );
	if( !mBuf )
		return FALSE;

	memcpy( &mBuf[iBuf],buf,size );/**/

	// �f�[�^�������ݐ�
//	DEBUG( "�������ݐ� %08X\n",sizeof(WAVHEADER) + sizeof(DATACHUNC)+iBuf );
	fseek( fp,sizeof(WAVHEADER) + sizeof(DATACHUNC)+iBuf,SEEK_SET );
	fwrite( buf,size,1,fp );

	// �w�b�_�̍X�V
	mHead.dwFileSize += size;
	mData.dwSize += size;
	fseek( fp,0,SEEK_SET );
	fwrite( &mHead,sizeof(WAVHEADER),1,fp );
	fwrite( &mData,sizeof(DATACHUNC),1,fp );
	
	iBuf += size;

	return TRUE;
}

BOOL CWav::Read( char *buf,int *size )
{
	if( dwMode!=WAVMODE_STREAM ) {
		DEBUG( "�X�g���[�����[�h�ł͂Ȃ�\n","" );
		return FALSE;
	}

	// �|�C���^���ړ�
	fseek( fp,sizeof(WAVHEADER) + sizeof(DATACHUNC)+iStreamPoint,SEEK_SET );

	// �ő�l�̃`�F�b�N
	int read = *size;
	if( read+iStreamPoint>iBuf )
		read = iBuf - iStreamPoint;

	*size = read;

	iStreamPoint += read;

	if( read<1 ) {
		DEBUG( "���[�h�I��\n","" );
		return FALSE;
	}

//	DEBUG( "���[�h�T�C�Y %d\n",read );

	fread( buf,read,1,fp );

	return TRUE;
}
