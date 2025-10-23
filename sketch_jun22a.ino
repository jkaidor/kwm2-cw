#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Encoder.h>

#define INPUT_FREQ 1700.0
#define OUTPUT_FREQ  600.0                 //really 600!
#define OUTPUT_LOWPASS 800.0
#define INJECTION_FREQ (INPUT_FREQ-OUTPUT_FREQ)
//#define INPUT_FILTER_Q 0.707
#define INPUT_FILTER_Q   10.0
#define OUTPUT_FILTER_Q  10.0 //0.707
#define CW_ENABLE_PIN 0
bool cw_enabled;
#define CW_OUTPUT_PORT 0
#define SSB_OUTPUT_PORT 1
#define ZERO_GAIN 0.0f
#define UNITY_GAIN 1.0f

// Is this an external box, or a card INSIDE the KWM-2?
#define INTERNAL_CARD
AudioOutputUSB           output_usb;           //
AudioInputAnalog         input_adc;
AudioSynthWaveformSine   injection_oscillator;          //xy=124.00001525878906,327.0333251953125
AudioFilterBiquad        input_filter;        //xy=270.0333251953125,193.03333282470703
AudioEffectMultiply      modulator;      //xy=445.0333251953125,227.0333251953125
AudioFilterBiquad        output_filter;        //xy=588.0333251953125,227.0333251953125
AudioMixer4              output_switch;         //xy=604.0333404541016,385.03334045410156
AudioOutputMQS           output_mqs;           //xy=609.0333251953125,472.0333251953125


int input_filter_q = INPUT_FILTER_Q;

// GUItool: begin automatically generated code
//AudioInputAnalog         input_adc;           //xy=75.03334045410156,192.0333251953125
AudioSynthToneSweep test_oscillator;

AudioConnection          patchCord1(input_adc, input_filter);
AudioConnection          patchCord2(input_adc, 0, output_switch, 1);
AudioConnection          patchCord3(injection_oscillator, 0, modulator, 1);
AudioConnection          patchCord4(input_filter, 0, modulator, 0);
AudioConnection          patchCord5(modulator, output_filter);
AudioConnection          patchCord6(output_filter, 0, output_switch, 0);
AudioConnection          patchCord7(output_switch, 0, output_mqs, 0);
AudioConnection          patchCord8(output_switch, 0, output_mqs, 1);
//AudioConnection          patchCord9(output_switch, 0, output_usb, 0);
//AudioConnection          patchCord10(output_switch, 0, output_usb, 1);

#define NUM_SELECTIVITIES 23

#ifdef INTERNAL_CARD
unsigned long cw_input_debounce_delay = 50;
bool raw_cw_input;
unsigned long cw_input_changed_time;
bool cw_input = false;         //don't really know what it is... but don't want it random.
bool last_cw_input = false;
#endif


//Actually "Q" values to set up the input bandpass filter.
float selectivities[ NUM_SELECTIVITIES ]
  = {
     0.5,
     0.6,
     0.7,
     0.8,
     0.9,
     1.0,
    2.0,
    3.0,
    4.0,
    5.0,
    6.0,
    7.0,
    8.0,
    9.0,
    10.0,
    15.0,
    20.0,
    25.0,
    30.0,
    35.0,
    40.0,
    45.0,
    50.0
    };

int selectivity_index = 0;


Encoder selectivity_encoder( 34, 33 );

#ifndef INTERNAL_CARD
void set_raw_selectivity( long );
int32_t raw_selectivity = 0L;
int32_t new_raw_selectivity = 0L;
#endif  //! INTERNAL_CARD

uint32_t when_changed;

void setup()
  {
  Serial.begin( 115200L );
  pinMode( CW_ENABLE_PIN, INPUT_PULLUP );
  AudioMemory( 16 );
  set_input_q( input_filter_q );

  output_filter.setLowpass( 0, OUTPUT_LOWPASS, OUTPUT_FILTER_Q );
  output_filter.setLowpass( 0, OUTPUT_LOWPASS, OUTPUT_FILTER_Q );
  output_filter.setLowpass( 0, OUTPUT_LOWPASS, OUTPUT_FILTER_Q );
  output_filter.setLowpass( 0, OUTPUT_LOWPASS, OUTPUT_FILTER_Q );

  injection_oscillator.frequency( INJECTION_FREQ );
  injection_oscillator.amplitude( 1.0f );

  pinMode( 32, INPUT_PULLUP );
  pinMode( 33, INPUT_PULLUP );
  selectivity_encoder.write( 0 );
  disable_cw();
  cw_enabled = false;
  cw_input_changed_time = millis();

  }

void loop()
  {
  // Debounce the CW enable input.  We do this by not accepting a change unless it has
  // been stable for a certain amount of time.
  raw_cw_input = digitalRead( CW_ENABLE_PIN );

  if( raw_cw_input != last_cw_input )
    {
    // It's changed!
    last_cw_input = raw_cw_input;
    cw_input_changed_time = millis();
    }
  // Subsequent swings after the initial change don't hit the above.
  if(( millis() - cw_input_changed_time ) > 10 )
    {
    cw_input = raw_cw_input;
    }

  if( !cw_input )
    {
    // The rig is in CW mode.
    if( !cw_enabled )
      {
      Serial.print( "Enabling CW mode\n");
      enable_cw();
      cw_enabled = true;
      // Toggle selectivity
      change_input_q();
      }
    }
  else
    {
    // The CW enable pin says "not CW"

    if( cw_enabled )
      {
      Serial.print( "Disabling CW mode\n");
      disable_cw();
      cw_enabled = false;
      }
    }

  //if( !test_oscillator.isPlaying() )    test_oscillator.play( 0.5, 1720, 1780, 1 );
  #ifndef INTERNAL_CARD
  uint32_t now = millis();
  new_raw_selectivity = selectivity_encoder.read();
  if( new_raw_selectivity != raw_selectivity )
    {
    if( now > (when_changed+100))
      {      
      set_raw_selectivity( new_raw_selectivity, raw_selectivity );
      raw_selectivity = new_raw_selectivity;
      when_changed = now;
      }
    }
  #endif //! INTERNAL_CARD
  }

#ifndef INTERNAL_CARD
void set_raw_selectivity( int32_t new_raw_selectivity, int32_t old_raw_selectivity )
  {
  if( new_raw_selectivity == old_raw_selectivity ) return;
  Serial.printf( "New raw selectivity is %d, old raw selectivity is %d\n", new_raw_selectivity, old_raw_selectivity );
  if( new_raw_selectivity < old_raw_selectivity )
    {
    selectivity_index--;
    // Don't let it decrement forever...
    if( selectivity_index < 0) selectivity_index = 0;
    }
  else
    {
    // raw selectivity is bigger
    selectivity_index++;
    // Don't let it increment forever
    if( selectivity_index >= NUM_SELECTIVITIES)
      {
      selectivity_index = NUM_SELECTIVITIES-1 ;
      }
    }
  Serial.printf( "   Setting selectivity index %d, filter Q %f\n", selectivity_index, selectivities[ selectivity_index ]);
  set_input_q( selectivities[ selectivity_index ] );
  }
#endif //! INTERNAL_CARD

void enable_cw()
  {
  // We use two inputs on the output switch.  input 0 goes to the output CW filter.  Input 1 goes to the input ADC for
  // undisturbed audio
  output_switch.gain( CW_OUTPUT_PORT, UNITY_GAIN );
  output_switch.gain( SSB_OUTPUT_PORT, ZERO_GAIN );
  }

void disable_cw()
  {
  output_switch.gain( CW_OUTPUT_PORT, ZERO_GAIN );
  output_switch.gain( SSB_OUTPUT_PORT, UNITY_GAIN );
  }

void set_input_q( float Q )
  {
  input_filter.setBandpass( 0, INPUT_FREQ, Q );  
  input_filter.setBandpass( 1, INPUT_FREQ, Q ); 
  input_filter.setBandpass( 2, INPUT_FREQ, Q );  
  input_filter.setBandpass( 3, INPUT_FREQ, Q );    
  }

//Each time CW is enabled, we will choose a different Q
// ...because we don't have any more controls in the KWM2
void change_input_q()
  {
    if( input_filter_q == 10.0 )  //tight
      {
      input_filter_q = 1.0;        //loose  
      }
    else
      {
      input_filter_q = 10.0;
      }
    set_input_q( input_filter_q );
  }


