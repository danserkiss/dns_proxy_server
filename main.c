#include "func.h"

int main()
{
    config *conf= import_config();   
    start(conf);
    free_config(conf);

    return 0;
}

