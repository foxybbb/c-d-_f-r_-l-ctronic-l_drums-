#include <Arduino.h>


#define NUMBERS_PIEZ 10// количество доступных аналоговых входов  8 из 16 доступных на плате 
// Значения при котором будут считываться сигнал, требуется для исключения паразитных связей
#define SNARE_THOLD 20  
#define LEFTTOM_THOLD 50
#define RIGHTTOM_THOLD 50
#define LEFTCYM_THOLD 100
#define RIGHTCYM_THOLD 100
#define TOM_THOLD 20
#define HIHAT_THOLD 80
#define KICK_TOLD 20
#define PEDAL 0    

#define PEDAL_NOTE 0 // Включение педали хай-хета
// значение переменных  - нот, которые будут передаваться по последовательному порту
#define SNARE_NOTE 38
#define LEFTTOM_NOTE 48
#define RIGHTTOM_NOTE 47
#define LEFTCYM_NOTE 49
#define RIGHTCYM_NOTE 51
#define TOM_NOTE 43
#define HIHAT_NOTA 42
#define KICK_NOTE 36
//#define HIHAT2_NOTA 46

#define NOTE_ON 0x90  // Сообщение NOTE ON отправляется, когда нажимается клавиша. Оно содержит параметры, указывающие интенсивность ноты при ударе.     
#define NOTE_OFF 0x80  //Когда синтезатор получает это сообщение, он начинает играть с правильным шагом и уровнем силы. При получении сообщения NOTE OFF примечание отключается синтезатором.
#define MAX_MIDI_VELOCITY 127  // максимально возможное значений нот

#define SERIAL_SPEED 31250

#define BUFFER_SIZE_SIGNAL 100 // скорость порта 
#define BUFFER_SIZE_PEAK 30
#define MAX_TIME_BETWEEN_PEAKS 20
#define MIN_TIME_BETWEEN_NOTES 50
// --------------------------------------------------------
// Настройка программы 
// переменные дял кректной работы
int pinLDR = 9;
int Ledpin = 13;
int thresholdLDR = 350;
int valorLDR = 0;

// Время задерки и хранения аналогового сигнала и пиков
unsigned short slotMP[NUMBERS_PIEZ];  // Максимально доступное количество датчиков

unsigned short noteMP[NUMBERS_PIEZ]; // область определения нот

unsigned short thresholdMP[NUMBERS_PIEZ]; // область определения чувствительности

// Буферные буферы для хранения аналогового сигнала и пиков
short currect_Signal_Index[NUMBERS_PIEZ];
short currectPeak_Index[NUMBERS_PIEZ];
unsigned short Buffer_Signal[NUMBERS_PIEZ][BUFFER_SIZE_SIGNAL];// сигнал 
unsigned short peak_Buffer[NUMBERS_PIEZ][BUFFER_SIZE_PEAK];// пики

boolean note_Ready[NUMBERS_PIEZ];
unsigned short note_Ready_Velocity[NUMBERS_PIEZ]; // готовность ноты
boolean LastPeakZeroed[NUMBERS_PIEZ]; // обнуление последнего пика 

unsigned long last_Peak_Time[NUMBERS_PIEZ]; // время последнего пика
unsigned long last_Note_Time[NUMBERS_PIEZ]; // время последней ноты
 
void setup()// код выполянется перед запуском 
{
  Serial.begin(SERIAL_SPEED);
  

  for(short i=0; i<NUMBERS_PIEZ; ++i)
  {
    currect_Signal_Index[i] = 0;
    currectPeak_Index[i] = 0;
    memset(Buffer_Signal[i],0,sizeof(Buffer_Signal[i]));
    memset(peak_Buffer[i],0,sizeof(peak_Buffer[i]));
    note_Ready[i] = false;
    note_Ready_Velocity[i] = 0;
    LastPeakZeroed[i] = true;
    last_Peak_Time[i] = 0;
    last_Note_Time[i] = 0;    
    slotMP[i] = PEDAL + i;
  }
  
  thresholdMP[0] = SNARE_THOLD; 
  thresholdMP[1] = RIGHTTOM_THOLD;
  thresholdMP[2] = LEFTTOM_THOLD;
  thresholdMP[3] = RIGHTCYM_THOLD;
  thresholdMP[4] = LEFTCYM_THOLD; 
  thresholdMP[5] = TOM_THOLD; 
  thresholdMP[6] = HIHAT_THOLD;
  thresholdMP[7] = KICK_TOLD;

  noteMP[0] = SNARE_NOTE;  
  noteMP[1] = RIGHTTOM_NOTE;
  noteMP[2] = LEFTTOM_NOTE;
  noteMP[3] = RIGHTCYM_NOTE;
  noteMP[4] = LEFTCYM_NOTE;
  noteMP[5] = TOM_NOTE;  
  noteMP[6] = HIHAT_NOTA;
  noteMP[7] = KICK_NOTE;
 
 {// Включение светодиода, после коррекктного инцилизации
  pinMode (pinLDR, INPUT);
  pinMode (Ledpin, OUTPUT);
 }
   
}
 

void loop()
{
  unsigned long Tempo_Actual = millis();
   
  for(short i=0; i<NUMBERS_PIEZ; ++i)
  {
    // получает новый сигнал от аналогового порта
    unsigned short new_signal = analogRead(slotMP[i]); 
    Buffer_Signal[i][currect_Signal_Index[i]] = new_signal;
    
    // если новый сигнал равен 0
    if(new_signal < thresholdMP[i])
    {
      if(!LastPeakZero[i] && (Tempo_Actual - last_Peak_Time[i]) > MAX_TIME_BETWEEN_PEAKS)
      {
        record_New_Peak(i,0);
      }
      else
      {
        // возвращает предыдущий сигнал
        short Signal_prev_index = currect_Signal_Index[i]-1;
        if(Signal_prev_index < 0) Signal_prev_index = BUFFER_SIZE_SIGNAL-1;        
        unsigned short Signal_prev = Buffer_Signal[i][Signal_prev_index];
        
        unsigned short n_Peak = 0;
        
       // поиск пика волны, если предыдущий сигнал не был 0
        while(Signal_prev >= thresholdMP[i])
        {
          if(Buffer_Signal[i][Signal_prev_index] > n_Peak)
          {
            n_Peak = Buffer_Signal[i][Signal_prev_index];        
          }
          
          // уменьшаем предыдущий сигнал и возвращаем его
          Signal_prev_index--;
          if(Signal_prev_index < 0) Signal_prev_index = BUFFER_SIZE_SIGNAL-1;
          Signal_prev = Buffer_Signal[i][Signal_prev_index];
        }
        
        if(n_Peak > 0)
        {
          record_New_Peak(i, n_Peak);
        }
      }
  
    }
        
    currect_Signal_Index[i]++;
    if(currect_Signal_Index[i] == BUFFER_SIZE_SIGNAL) currect_Signal_Index[i] = 0;
   
  }
  int input = analogRead(pinLDR);
  if (input > thresholdLDR) {
    digitalWrite(Ledpin, LOW);
  }
  else {
    digitalWrite(Ledpin, HIGH);
  }
}

void record_New_Peak(short slot, short n_Peak)
{
  LastPeakZero[slot] = (n_Peak == 0);
  
  unsigned long Tempo_Actual = millis();
  last_Peak_Time[slot] = Tempo_Actual;
  
  // запись нового пика (newPeak)
  peak_Buffer[slot][currectPeak_Index[slot]] = n_Peak;
  
	// 1 из 3 случаев может случиться:
   // 1) ноты готовы - новый пик> = предыдущий пик
   // 2) отмечает - новый пик < предыдущий пик
   // 3) нет примечания - новый пик
  
   // возвращвем предыдущий пик
  short Peak_prev_index = currectPeak_Index[slot]-1;
  if(Peak_prev_index < 0) Peak_prev_index = BUFFER_SIZE_PEAK-1;        
  unsigned short Peak_prev = peak_Buffer[slot][Peak_prev_index];
   
  if(n_Peak > Peak_prev && (Tempo_Actual - last_Note_Time[slot]) > MIN_TIME_BETWEEN_NOTES)
  {
    note_Ready[slot] = true;
    if(n_Peak > note_Ready_Velocity[slot])
      note_Ready_Velocity[slot] = n_Peak;
  }
  else if(n_Peak < Peak_prev && note_Ready[slot])
    {
    EnviaNota(noteMP[slot], note_Ready_Velocity[slot]);
    note_Ready[slot] = false;
    note_Ready_Velocity[slot] = 0;
    last_Note_Time[slot] = Tempo_Actual;
    
  }
  
  currectPeak_Index[slot]++;
  if(currectPeak_Index[slot] == BUFFER_SIZE_PEAK) currectPeak_Index[slot] = 0;  
}

void EnviaNota(unsigned short note, unsigned short midiVelocity)
{
  if(midiVelocity > MAX_MIDI_VELOCITY)
    midiVelocity = MAX_MIDI_VELOCITY;
  
  midiNotaOn(note, midiVelocity);
  midiNotaOff(note, midiVelocity);
}

void midiNotaOn(byte note, byte velocity)
{
  if (note == 42 && analogRead(pinLDR) < thresholdLDR)
  { 
    
  Serial.write(NOTE_ON);
  Serial.write(46);
  Serial.write(velocity);
   }
     else
   {
    
  Serial.write(NOTE_ON);
  Serial.write(note);
  Serial.write(velocity);
    
    }
  
}

void midiNotaOff(byte note, byte velocity)
{
  Serial.write(NOTE_OFF);
  Serial.write(note);
  Serial.write(velocity);
}




