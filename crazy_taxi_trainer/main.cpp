#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <stdio.h>

/*
	Imagebase: 0x400000
	.text:004FF6F3
	.text:004FF6F3                                                 loc_4FF6F3:                             ; CODE XREF: sub_4FF680+5Cj
	.text:004FF6F3                                                                                         ; DATA XREF: .text:off_4FF870o
	.text:004FF6F3 C7 05 A4 41 6A 01 30 2A 00 00                                   mov     g_global_tick_count, 2A30h ; jumptable 004FF6DC case 1
	.text:004FF6FD EB 43                                                           jmp     short loc_4FF742
	.text:004FF6FF                                                 ; ---------------------------------------------------------------------------
	.text:004FF6FF
	.text:004FF6FF                                                 loc_4FF6FF:                             ; CODE XREF: sub_4FF680+5Cj
	.text:004FF6FF                                                                                         ; DATA XREF: .text:off_4FF870o
	.text:004FF6FF C7 05 A4 41 6A 01 50 46 00 00                                   mov     g_global_tick_count, 4650h ; jumptable 004FF6DC case 2
	.text:004FF709 EB 37                                                           jmp     short loc_4FF742
	.text:004FF70B                                                 ; ---------------------------------------------------------------------------
	.text:004FF70B
	.text:004FF70B                                                 loc_4FF70B:                             ; CODE XREF: sub_4FF680+5Cj
	.text:004FF70B                                                                                         ; DATA XREF: .text:off_4FF870o
	.text:004FF70B C7 05 A4 41 6A 01 A0 8C 00 00                                   mov     g_global_tick_count, 8CA0h ; jumptable 004FF6DC case 3
	.text:004FF715 EB 2B                                                           jmp     short loc_4FF742


	.data:016A41A4 ?? ?? ?? ??                                     g_global_tick_count dd ?                ; DATA XREF: sub_4B3D00+73r
*/
#define OFFSET_GLOBAL_TICK_COUNT 0x12A41A4

// returns the first process found by name
BOOL get_process_by_name(CONST TCHAR *image_name, DWORD *store_pid) {
	BOOL success = FALSE;
	HANDLE snapshot = INVALID_HANDLE_VALUE;
	PROCESSENTRY32 process_entry;

	if(!store_pid) {
		_tprintf(_T("[-] invalid parameter\n"));
		goto end;
	}

	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(snapshot == INVALID_HANDLE_VALUE) {
		goto end;
	}

	process_entry.dwSize = sizeof(PROCESSENTRY32);

	if(!Process32First(snapshot, &process_entry)) {
		goto end;
	}

	do {
		if(!_tcsicmp(image_name, process_entry.szExeFile)) {
			*store_pid = process_entry.th32ProcessID;
			success = TRUE;
			break;
		}
	} while(Process32Next(snapshot, &process_entry));

end:
	if(snapshot != INVALID_HANDLE_VALUE) {
		CloseHandle(snapshot);
	}
	return success;
}

// retrieve the remote module
BOOL get_remote_module_info(DWORD pid, CONST TCHAR *module_name, BYTE **store_module_base) {
	BOOL success = FALSE;
	HANDLE snapshot = INVALID_HANDLE_VALUE;
	MODULEENTRY32 mod;

	// module base is mandatory
	if(!store_module_base) {
		goto end;
	}

	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if(snapshot == INVALID_HANDLE_VALUE) {
		goto end;
	}

	mod.dwSize = sizeof(MODULEENTRY32);

	if(!Module32First(snapshot, &mod)) {
		goto end;
	}

	do {
		if(!_tcsicmp(module_name, mod.szModule)) {
			*store_module_base = mod.modBaseAddr;
			success = TRUE;
			break;
		}
	} while(Module32Next(snapshot, &mod));

end:
	if(snapshot != INVALID_HANDLE_VALUE) {
		CloseHandle(snapshot);
	}
	return success;
}

void patch_timer(LONG new_time) {
	BOOL success;
	HANDLE target;
	BYTE *remote_mod_base;
	DWORD pid;
	TCHAR image_name[] = _T("Crazy Taxi.exe");

	success = get_process_by_name(image_name, &pid);
	if(!success) {
		_tprintf(_T("[-] error: not find process by name %d\n"), GetLastError());
		goto end;
	}

	target = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if(!target) {
		_tprintf(_T("[-] error: not open process %d\n"), GetLastError());
		goto end;
	}

	success = get_remote_module_info(pid, image_name, &remote_mod_base);
	if(!success) {
		_tprintf(_T("[-] error: not get remote module info %d\n"), GetLastError());
		goto end;
	}

	success = WriteProcessMemory(target, remote_mod_base + OFFSET_GLOBAL_TICK_COUNT, &new_time, sizeof(new_time), NULL);
	if(!success) {
		_tprintf(_T("[-] error: could not patcht the remote process%d\n"), GetLastError());
		goto end;
	}

	_tprintf(_T("[+] Adjusted the time to %d\n"), new_time);

end:
	return;
}

int main(int argc, char *argv[]) {
	_tprintf(_T("[+] Welcome to Crazy Taxi Trainer\n"
				"    Press F1 to patch the time to unlimited\n"
				"    Press F2 to patch the time to 0 to finish the round\n"));

	while(true) {
		if(GetAsyncKeyState(VK_F1)) {
			patch_timer(-1);
		}
		if(GetAsyncKeyState(VK_F2)) {
			patch_timer(0);
		}
		Sleep(1000);
	}

	return 0;
}