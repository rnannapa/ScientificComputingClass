#ifndef _M6378_H_
#define _M6378_H_

int    M6378_Init      ( int*, char*** );
int    M6378_Comm_size ( int* );
int    M6378_Comm_rank ( int* );
int    M6378_Abort     ( int );  /* arg is exit status. */
double M6378_Dtime     ( int* ); /* arg is resolution in nanosecs. */
int    M6378_Barrier   ();
int    M6378_Send      ( int, void*, int );
int    M6378_Recv      ( int, void*, int* ); /* &size | NULL if not needed. */
int    M6378_Finalize  ();
void  *M6378_Sendstart ( int, int );
int    M6378_Sendfinish( int );
void  *M6378_Recvstart ( int, int* );
int    M6378_Recvfinish( int );

#endif /* _M6378_H_ */
