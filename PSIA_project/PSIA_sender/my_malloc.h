#pragma once
void* allocate_memory(size_t size);
void* reallocate_memory(void* ptr, size_t new_size);
void free_memory(void* ptr);