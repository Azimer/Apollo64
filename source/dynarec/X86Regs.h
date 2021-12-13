/*  Ideas for RegCaching...

  Bind ESP to R4K Register 29
  Bind EAX to Temporary Work Register
  Bind EBX, ECX, EDX, ESI, EBP to all other registers
  

  "R0", "AT", "V0", "V1", "A0", "A1", "A2", "A3", "T0", "T1", "T2",
  "T3", "T4", "T5", "T6", "T7", "S0", "S1", "S2", "S3", "S4", "S5",
  "S6", "S7", "T8", "T9", "K0", "K1", "GP", "SP", "S8", "RA"

  At start of Function:
  A0 = EAX
  A1 = EBX
  A2 = ECX
  A3 = EDX
  SP = ESP

  At end of Function:
  V0 = EAX
  V1 = EBX
  */


#define EAX
#define EBX
#define ECX
#define EDX
#define EDI
#define ESI
#define EBP
#define ESP