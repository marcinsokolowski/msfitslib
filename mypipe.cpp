#include "mypipe.h"
#include <errno.h>
#include "mydate.h"

CMyPipe::CMyPipe()
: pipe_write(NULL), pipe_read(NULL)
{
	pipe( fd );

	pipe_write = fdopen( fd[1], "w" );
	pipe_read  = fdopen( fd[0], "r" );

	m_Reader = fd[0];
	m_Writer = fd[1];
}


CMyPipe::~CMyPipe()
{
	close(fd[0]);
	close(fd[1]);
}

int CMyPipe::WaitForData( char& data )
{
	int ret = read( m_Reader, &data, 1 );

	// printf("\n\n!!!!!!!!  read from pipe = %c\n\n\n",data);fflush(0);

	return ret;
}

int CMyPipe::WriteData( char c )
{
	int ret = write( m_Writer, &c, 1 );
	fflush( pipe_write );
	if( ret < 1 ){
		printf("ERROR in CMyPipe::WriteData at unix_time=%d: could not write char to m_Writer pipe errno=%d!!!!\n",get_dttm(),errno);
	}


	// fflush( 0 );

	// printf("\n\n!!!!!!!!  write to pipe = %c\n\n\n",c);fflush(0);

	return ret;
}


char CPipeLock::Wait()
{
	char c;
	WaitForData( c );

	return c;
}

int CPipeLock::Release( char c )
{
	int ret = WriteData( c );
	return ret;
}

