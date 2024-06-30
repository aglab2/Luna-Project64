#include "Rsp.h"

#define MaxBPoints			0x30

typedef struct {
   unsigned int Location;
} BPOINT;

extern BPOINT BPoint[MaxBPoints];
extern int	NoOfBpoints;

void CALL Add_BPoint ( void );
void CALL CreateBPPanel ( void * hDlg, rectangle rcBox );
void CALL HideBPPanel ( void );
void CALL PaintBPPanel ( window_paint ps );
void CALL ShowBPPanel ( void );
void CALL RefreshBpoints ( void * hList );
void CALL RemoveBpoint ( void * hList, int index );
void CALL RemoveAllBpoint ( void );

int  CALL AddRSP_BPoint ( DWORD Location, int Confirm );
int  CALL CheckForRSPBPoint ( DWORD Location );
void CALL RemoveRSPBreakPoint (DWORD Location);
