#pragma once

bool Hook(char* src, char* dst, int len);

char* TrampHook(char* src, char* dst, unsigned int len);

bool WriteMem(void* pDst, char* pBytes, size_t size);