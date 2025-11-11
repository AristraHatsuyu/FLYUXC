#include "flyuxc/mapper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

IdentifierMapper* mapper_create(void) {
    IdentifierMapper *m = malloc(sizeof(*m));
    if (!m) return NULL;
    m->capacity = 64;
    m->count = 0;
    m->entries = malloc(m->capacity * sizeof(MapEntry));
    if (!m->entries) { free(m); return NULL; }
    return m;
}

void mapper_free(IdentifierMapper *mapper) {
    if (!mapper) return;
    for (size_t i = 0; i < mapper->count; ++i) {
        free(mapper->entries[i].original);
        free(mapper->entries[i].mapped);
    }
    free(mapper->entries);
    free(mapper);
}

// Helper to check if identifier needs mapping (contains non-ASCII or special chars)
static bool needs_mapping(const char *ident) {
    for (size_t i = 0; ident[i]; ++i) {
        unsigned char c = (unsigned char)ident[i];
        // Keep only ASCII letters, digits, underscores; rest need mapping
        if (!isalnum(c) && c != '_') return true;
        if (c >= 128) return true; // non-ASCII UTF-8 byte
    }
    return false;
}

const char* mapper_get_or_alloc(IdentifierMapper *mapper, const char *original) {
    if (!original || !needs_mapping(original)) {
        // Return original if it's already valid ASCII
        return original;
    }

    // Search for existing mapping
    for (size_t i = 0; i < mapper->count; ++i) {
        if (strcmp(mapper->entries[i].original, original) == 0) {
            return mapper->entries[i].mapped;
        }
    }

    // Allocate new mapping
    if (mapper->count >= mapper->capacity) {
        mapper->capacity *= 2;
        MapEntry *new_entries = realloc(mapper->entries, mapper->capacity * sizeof(MapEntry));
        if (!new_entries) return NULL;
        mapper->entries = new_entries;
    }

    // Generate new mapped identifier: _XXXXX (5 chars uppercase/digits after underscore)
    char mapped_buf[16];
    snprintf(mapped_buf, sizeof(mapped_buf), "_%05lX", (unsigned long)mapper->count);
    // Convert to all uppercase
    for (int i = 1; i < 6; ++i) {
        if (mapped_buf[i] >= 'a' && mapped_buf[i] <= 'f') {
            mapped_buf[i] = 'A' + (mapped_buf[i] - 'a');
        }
    }

    mapper->entries[mapper->count].original = strdup(original);
    mapper->entries[mapper->count].mapped = strdup(mapped_buf);

    if (!mapper->entries[mapper->count].original || !mapper->entries[mapper->count].mapped) {
        free(mapper->entries[mapper->count].original);
        free(mapper->entries[mapper->count].mapped);
        return NULL;
    }

    const char *result = mapper->entries[mapper->count].mapped;
    mapper->count++;
    return result;
}

void mapper_output_json(IdentifierMapper *mapper) {
    printf("{\n");
    printf("  \"mappings\": [\n");
    for (size_t i = 0; i < mapper->count; ++i) {
        printf("    {\"original\": \"");
        // Escape JSON string (simple version: escape quotes and backslashes)
        for (size_t j = 0; mapper->entries[i].original[j]; ++j) {
            unsigned char c = (unsigned char)mapper->entries[i].original[j];
            if (c == '"') printf("\\\"");
            else if (c == '\\') printf("\\\\");
            else printf("%c", c);
        }
        printf("\", \"mapped\": \"%s\"}", mapper->entries[i].mapped);
        if (i < mapper->count - 1) printf(",");
        printf("\n");
    }
    printf("  ]\n");
    printf("}\n");
}
