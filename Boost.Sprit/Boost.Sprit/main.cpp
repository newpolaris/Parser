#include "mqo.h"

int main()
{
    client::mqo mqo;
    client::parse_mqo("torus.mqo", mqo);
    return 0;
}