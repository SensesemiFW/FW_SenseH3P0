#include "Error_Handling.h"
#include "API_Display.h"
#include <stdio.h>
#include <stdint.h>

void Catch_RunTime_Error(ERROR_CODES_t Error_code)
{
 printf("\nError Code :%d\n",Error_code);
 //API_DISP_Error_Code(Error_code);
}
