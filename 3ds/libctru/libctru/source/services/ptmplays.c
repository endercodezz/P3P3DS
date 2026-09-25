#include <string.h>
#include <3ds/types.h>
#include <3ds/result.h>
#include <3ds/svc.h>
#include <3ds/srv.h>
#include <3ds/synchronization.h>
#include <3ds/ipc.h>

#include <3ds/services/ptmplays.h>

static Handle ptmPlaysHandle;
static int ptmPlaysRefCount;

Result ptmPlaysInit(void)
{
	if (AtomicPostIncrement(&ptmPlaysRefCount)) return 0;
	Result res = srvGetServiceHandle(&ptmPlaysHandle, "ptm:plays");
	if (R_FAILED(res)) AtomicDecrement(&ptmPlaysRefCount);
	return res;
}

void ptmPlaysExit(void)
{
	if (AtomicDecrement(&ptmPlaysRefCount)) return;
	svcCloseHandle(ptmPlaysHandle);
}

Handle *ptmPlaysGetSessionHandle(void)
{
	return &ptmPlaysHandle;
}

Result PTMPLAYS_GetPlayHistory(s32 *outEndIndex, PtmPlayEvent *outEvents, s32 startIndex, s32 maxEvents)
{
	Result ret;
	u32 *cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x807,2,2); // 0x08070282

	cmdbuf[1] = (u32)startIndex;
	cmdbuf[2] = (u32)maxEvents;
	cmdbuf[3] = IPC_Desc_Buffer(sizeof(PtmPlayEvent) * maxEvents, IPC_BUFFER_W);
	cmdbuf[4] = (u32)outEvents;

	if(R_FAILED(ret = svcSendSyncRequest(ptmPlaysHandle)))return ret;

	*outEndIndex = (s32)cmdbuf[2];

	return (Result)cmdbuf[1];
}

Result PTMPLAYS_GetPlayHistoryStart(s32 *outStartIndex)
{
	Result ret;
	u32 *cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x808,0,0); // 0x08080000

	if(R_FAILED(ret = svcSendSyncRequest(ptmPlaysHandle)))return ret;

	*outStartIndex = (s32)cmdbuf[2];

	return (Result)cmdbuf[1];
}

Result PTMPLAYS_GetPlayHistorySize(s32 *outNumEvents)
{
	Result ret;
	u32 *cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x809,0,0); // 0x08090000

	if(R_FAILED(ret = svcSendSyncRequest(ptmPlaysHandle)))return ret;

	*outNumEvents = (s32)cmdbuf[2];

	return (Result)cmdbuf[1];
}

Result PTMPLAYS_CalcPlayHistoryStart(s32 *outIndex, s32 start, s32 numEvents)
{
	Result ret;
	u32 *cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x80A,2,0); // 0x08090000

	cmdbuf[1] = (u32)start;
	cmdbuf[2] = (u32)numEvents;

	if(R_FAILED(ret = svcSendSyncRequest(ptmPlaysHandle)))return ret;

	*outIndex = (s32)cmdbuf[2];

	return (Result)cmdbuf[1];
}
