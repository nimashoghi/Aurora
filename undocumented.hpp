#pragma once

#include <windows.h>

/*
 * Data structures
 */

typedef struct _WNF_STATE_NAME
{
    ULONG Data[2];
} WNF_STATE_NAME, *PWNF_STATE_NAME;
typedef const WNF_STATE_NAME *PCWNF_STATE_NAME;

typedef struct _WNF_TYPE_ID
{
    GUID TypeId;
} WNF_TYPE_ID, *PWNF_TYPE_ID;
typedef const WNF_TYPE_ID *PCWNF_TYPE_ID;

typedef ULONG WNF_CHANGE_STAMP, *PWNF_CHANGE_STAMP;


NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryWnfStateData(
    _In_ PCWNF_STATE_NAME StateName,
    _In_opt_ PCWNF_TYPE_ID TypeId,
    _In_opt_ const VOID* ExplicitScope,
    _Out_ PWNF_CHANGE_STAMP ChangeStamp,
    _Out_writes_bytes_to_opt_(*BufferSize, *BufferSize) PVOID Buffer,
    _Inout_ PULONG BufferSize);

/*
 * Constants
 */

// https://i.imgur.com/6xYweKo.png
inline WNF_STATE_NAME WNF_RM_GAME_MODE_ACTIVE
{{
    0xA3BC1075,
    0x41C6033F
}};
