
#define PROD_LEN 2

unsigned short products[PROD_LEN] = {
    0x633E,
    0x6860
};

int is_android_device(unsigned short product_id)
{
    int i;
    for (i = 0; i < PROD_LEN; ++i) {
        if (products[i] == product_id)
            return 1;
    }

    return 0;
}
