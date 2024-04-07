#include "object.h"
#include <stdlib.h>
#include <string.h>

#define ALLOC(pname, ptype, psize)                     \
    if (psize) {                                       \
        pname = (ptype*)malloc(psize * sizeof(ptype)); \
        if (pname)                                     \
            memset(pname, 0, psize);                   \
        else                                           \
            goto alloc_err;                            \
    }

static void __dealloc_object_info(const struct object_info* obj_info)
{
    if (obj_info->Filename)
        free(obj_info->Filename);

    if (obj_info->CaptureDate)
        free(obj_info->CaptureDate);

    if (obj_info->ModificationDate)
        free(obj_info->ModificationDate);

    if (obj_info->Keywords)
        free(obj_info->Keywords);

    free((struct object_info*)obj_info);
}

struct object_info* alloc_object_info(uint32_t filename_size, uint32_t capture_date_size, uint32_t modif_date_size, uint32_t keywords_size)
{
    struct object_info* __obj_info = (struct object_info*)malloc(sizeof(struct object_info));
    if (!__obj_info)
        return NULL;

    memset(__obj_info, 0, sizeof(struct object_info));

    ALLOC(__obj_info->Filename, char, filename_size);
    ALLOC(__obj_info->CaptureDate, char, capture_date_size);
    ALLOC(__obj_info->ModificationDate, char, modif_date_size);
    ALLOC(__obj_info->Keywords, char, keywords_size);

    return __obj_info;

alloc_err:

    __dealloc_object_info(__obj_info);
    return NULL;
}

void destroy_object_info(const struct object_info* obj_info)
{
    __dealloc_object_info(obj_info);
}
