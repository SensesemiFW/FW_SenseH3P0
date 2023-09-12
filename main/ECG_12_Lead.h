/*
 * ECG_12_Lead.h
 *
 *  Created on: 06-Jan-2023
 *      Author: madhusudhan
 */

#ifndef MAIN_ECG_12_LEAD_H_
#define MAIN_ECG_12_LEAD_H_

#include <stdbool.h>
#include "API_utility.h"

typedef enum
{
  LEAD6,
  VLEAD1,
  VLEAD2,
  VLEAD3,
  VLEAD4,
  VLEAD5,
  VLEAD6
}VLEAD_TYPE_t;


bool Lead12_Test(void);

void Select_Vlead(VLEAD_TYPE_t vlead_type);


#endif /* MAIN_ECG_12_LEAD_H_ */
