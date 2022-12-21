// TerOled.h    ref 190326 
// TerOled need before TwiOledh/BcXxOled.h et PixOled.h ou MapOled.h
// dans pp  #include "TerOled.h"  pas de setup spécifique
// Compatible to TerSer.h in the default mode Do not have Compact and 
// Replace TerNum.h    Big/double size) not available
// differences with TerSer  TetGet TerEcho TabCar

byte moz=0;
#define HideZ() moz=0  // Spaces mode of TerSer
#define ShowZ() moz=1  // Zeros mode of TerSer

// Car() Text() defined in PixOled.h 

void TtBin8 (byte bb) {
  for (byte i=0;i<8;i++) {
     if (bb&0x80) Car('1');
     else Car('0');
     bb <<= 1;
  }
}
void Bin8 (byte bb) {
  TtBin8 (bb); Car(' ');
}
void Bin16 (uint16_t bb) {
 TtBin8 (bb>>8); Car('.');
 TtBin8 (bb&0xFF); Car(' ');
}
char ConvNibble (byte nn) { // converti 4 bit hexa en Ascii
  byte cc;
  if (nn<10) {cc = nn + '0';}
  else {cc = nn-10 + 'A';}
  return cc;
}
void Hex8 (byte hh) {
  byte cc;
  cc = ConvNibble (hh >> 4);  Car(cc);
  cc = ConvNibble (hh & 0x0F) ; Car(cc);
  Car(' '); // space
}
void Hex16 (uint16_t hh) {
  volatile uint8_t cc;
  cc = ConvNibble (hh >> 12) ;  Car(cc);
  cc = ConvNibble ((hh >> 8)&0x0F) ; Car(cc);
  cc=hh&0xFF; Hex8 (cc);
  Car(' ');
}

void Hex32 (uint32_t hh) {
  volatile uint8_t cc;
  cc = ConvNibble (hh >> 28) ; Car(cc);
  cc = ConvNibble ((hh >> 24)&0x0F) ; Car(cc);
  cc = ConvNibble ((hh >> 20)&0x0F) ; Car(cc);
  cc = ConvNibble ((hh >> 16)&0x0F) ; Car(cc);
  hh = hh&0xFFFF; Hex16 (hh);
  Car(' ');
}  

// espace pour zéros non sig // moz=1 affiche zero et pas espace
#define Afzs if(moz){cc='0';}else{cc=' ';}

volatile byte uni,diz,cen,mil,dim;

void Af3Dig () {
   byte cc; byte afz=0;
  cc=(cen+'0');
    if (cc>48) {afz=1;} else {Afzs;}  Car(cc);
  cc=(diz+'0');
  if ((cc>48)|(afz)){afz=1;} else {Afzs;} Car(cc);
  cc=(uni+'0'); Car(cc);
     Car(' '); 
}
void Af5Dig () {
   byte cc; byte afz=0;
  cc=(dim+'0');
    if (cc>48) {afz=1;} else {Afzs;}  Car(cc);
  cc=(mil+'0'); 
  if ((cc>48)|(afz)){afz=1;} else {Afzs;} Car(cc); 
  cc=(cen+'0');
  if ((cc>48)|(afz)){afz=1;} else {Afzs;} Car(cc);
  cc=(diz+'0');
  if ((cc>48)|(afz)){afz=1;} else {Afzs;} Car(cc);
  cc=(uni+'0'); Car(cc);
     Car(' ');
}

void BinDec8 (uint8_t bb) { //21us
 uni=bb%10; bb=bb/10;
 diz=bb%10; bb=bb/10;
 cen=bb;
}
#define Dec8(v)                 \
    do { typeof(v) _v = -1;        \
      if (_v < 0) { Dec8s(v); } \
       else { Dec8u(v); }       \
    } while(0);

void Dec8u (uint8_t hh) {
   BinDec8 (hh); Af3Dig();
}

void Dec8s (int8_t hh) {
  if (hh<0) {  // négatif
    Car('-'); hh = -hh;
  } else {
    Car('+');
  }
  BinDec8 (hh); Af3Dig();
}
void BinDec16 (uint16_t bb) { //21us
 uni=bb%10; bb=bb/10;
 diz=bb%10; bb=bb/10;
 cen=bb%10; bb=bb/10;
 mil=bb%10; bb=bb/10;
 dim=bb;
}

#define Dec16(v)                 \
    do { typeof(v) _v = -1;         \
      if (_v < 0) { Dec16s(v); } \
       else { Dec16u(v); }       \
    } while(0);
    
void Dec16u (uint32_t hh) {  // 
  BinDec16 (hh); Af5Dig();
}

void Dec16s (int16_t hh) {  // signé
  if(hh<0) {  // negatif
    Car('-'); hh = -hh;
  } else {
    Car('+'); 
  }
  BinDec16 (hh); Af5Dig();
}



